// LseBridge.cpp - lrca 运行时挂载实现（精确 + 导出表模糊两级符号解析）
//
// 背景（2026-08-25 本机实证）：本机 LegacyRemoteCall.dll 与源内符号快照
// 不匹配（不同编译版本的 MSVC STL mangling 差异），GetProcAddress 精确
// 匹配全失败 → 旧实现 gAttachFailed 置位后永久放弃，LSE 桥形同虚设，
// 所有 ll.import("HologramLib",...) 报 "has not been exported"。
//
// 修复（与 MeowPAPI LseBridge 同源方案，2026-08-25 实测模糊匹配可用）：
// 精确失败后手动解析 PE 导出表按前缀模糊匹配（"?exportFunc@RemoteCall@@YA"
// 等 6 个前缀）。MSVC ABI 自 2015 起 std::function/std::string 布局稳定，
// 前缀命中的函数二进制兼容。
#include "LseBridge.h"

#include <Windows.h>

#include <cstdint>
#include <cstring>
#include <mutex>

namespace hologramlib::lse {

namespace {

// lrca 导出函数的完整 mangled 符号（精确匹配用，dumpbin 快照）
constexpr char const* kSymbolExportFunc =
    "?exportFunc@RemoteCall@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0$$QEAV?$function@"
    "$$A6A?AUValueType@RemoteCall@@V?$vector@UValueType@RemoteCall@@V?$allocator@UValueType@RemoteCall@@@std@@@std@@@"
    "Z@std@@@Z@3@PEAX@Z";
constexpr char const* kSymbolImportFunc =
    "?importFunc@RemoteCall@@YAAEBV?$function@$$A6A?AUValueType@RemoteCall@@V?$vector@UValueType@RemoteCall@@V?"
    "allocator@UValueType@RemoteCall@@@std@@@std@@@Z@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@"
    "D@2@@std@@3@0@Z";
constexpr char const* kSymbolHasFunc =
    "?hasFunc@RemoteCall@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z";
constexpr char const* kSymbolRemoveFunc =
    "?removeFunc@RemoteCall@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z";
constexpr char const* kSymbolRemoveNameSpace =
    "?removeNameSpace@RemoteCall@@YAHAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z";
constexpr char const* kSymbolOnCallError =
    "?_onCallError@RemoteCall@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEAX@Z";

// 模糊匹配前缀（导出表扫描用）：截断到函数名+调用约定为止，
// 忽略后续参数类型的 STL mangling 差异
constexpr char const* kPrefixExportFunc      = "?exportFunc@RemoteCall@@YA";
constexpr char const* kPrefixImportFunc      = "?importFunc@RemoteCall@@YA";
constexpr char const* kPrefixHasFunc         = "?hasFunc@RemoteCall@@YA";
constexpr char const* kPrefixRemoveFunc      = "?removeFunc@RemoteCall@@YA";
constexpr char const* kPrefixRemoveNameSpace = "?removeNameSpace@RemoteCall@@YA";
constexpr char const* kPrefixOnCallError     = "?_onCallError@RemoteCall@@YA";

std::mutex gAttachMutex;
bool       gAttached     = false;
bool       gAttachFailed = false;

template <typename Fn>
Fn resolve(HMODULE module, char const* symbol) {
    if (!module) return nullptr;
    return reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(module, symbol)));
}

// PE 导出表前缀模糊解析：返回首个以 prefix 开头的导出函数地址。
// 手动解析 IMAGE_EXPORT_DIRECTORY（模块已加载，RVA + 基址即可）
void* fuzzyResolve(HMODULE module, char const* prefix) {
    if (!module) return nullptr;
    auto base = reinterpret_cast<uint8_t*>(module);
    auto dos  = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS const*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;
    auto const& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (dir.VirtualAddress == 0 || dir.Size == 0) return nullptr;
    auto exp   = reinterpret_cast<IMAGE_EXPORT_DIRECTORY const*>(base + dir.VirtualAddress);
    auto names = reinterpret_cast<uint32_t const*>(base + exp->AddressOfNames);
    auto funcs = reinterpret_cast<uint32_t const*>(base + exp->AddressOfFunctions);
    auto ords  = reinterpret_cast<uint16_t const*>(base + exp->AddressOfNameOrdinals);
    size_t prefixLen = std::strlen(prefix);
    for (DWORD i = 0; i < exp->NumberOfNames; i++) {
        char const* name = reinterpret_cast<char const*>(base + names[i]);
        if (std::strncmp(name, prefix, prefixLen) == 0) {
            return base + funcs[ords[i]];
        }
    }
    return nullptr;
}

template <typename Fn>
Fn fuzzyResolveFn(HMODULE module, char const* prefix) {
    return reinterpret_cast<Fn>(fuzzyResolve(module, prefix));
}

// 候选模块：独立 lrca 优先，LSE 引擎 DLL 兜底（若引擎内置 RemoteCall）
wchar_t const* kCandidates[] = {
    L"LegacyRemoteCall.dll",
    L"legacy-script-engine-quickjs.dll",
    L"legacy-script-engine-nodejs.dll",
};

void clearResolved() {
    detail::p_exportFunc      = nullptr;
    detail::p_importFunc      = nullptr;
    detail::p_hasFunc         = nullptr;
    detail::p_removeFunc      = nullptr;
    detail::p_removeNameSpace = nullptr;
    detail::p_onCallError     = nullptr;
}

// 两级符号解析；exportFunc/importFunc 是闭环最小集，缺一即失败
bool tryResolveAll(HMODULE module) {
    // 第一级：精确 mangled 符号
    detail::p_exportFunc      = resolve<detail::ExportFuncFn>(module, kSymbolExportFunc);
    detail::p_importFunc      = resolve<detail::ImportFuncFn>(module, kSymbolImportFunc);
    detail::p_hasFunc         = resolve<detail::HasFuncFn>(module, kSymbolHasFunc);
    detail::p_removeFunc      = resolve<detail::RemoveFuncFn>(module, kSymbolRemoveFunc);
    detail::p_removeNameSpace = resolve<detail::RemoveNameSpaceFn>(module, kSymbolRemoveNameSpace);
    detail::p_onCallError     = resolve<detail::OnCallErrorFn>(module, kSymbolOnCallError);
    if (detail::p_exportFunc && detail::p_importFunc) return true;

    // 第二级：导出表前缀模糊匹配（跨 lrca 编译版本的 STL mangling 差异）
    clearResolved();
    detail::p_exportFunc      = fuzzyResolveFn<detail::ExportFuncFn>(module, kPrefixExportFunc);
    detail::p_importFunc      = fuzzyResolveFn<detail::ImportFuncFn>(module, kPrefixImportFunc);
    detail::p_hasFunc         = fuzzyResolveFn<detail::HasFuncFn>(module, kPrefixHasFunc);
    detail::p_removeFunc      = fuzzyResolveFn<detail::RemoveFuncFn>(module, kPrefixRemoveFunc);
    detail::p_removeNameSpace = fuzzyResolveFn<detail::RemoveNameSpaceFn>(module, kPrefixRemoveNameSpace);
    detail::p_onCallError     = fuzzyResolveFn<detail::OnCallErrorFn>(module, kPrefixOnCallError);
    return detail::p_exportFunc && detail::p_importFunc;
}

} // namespace

bool attach() {
    std::lock_guard<std::mutex> lock(gAttachMutex);
    if (gAttached) return true;
    if (gAttachFailed) return false; // 每进程只尝试一轮, 避免反复扫导出表

    for (auto const* cand : kCandidates) {
        HMODULE mod = ::GetModuleHandleW(cand);
        if (!mod) continue;
        if (tryResolveAll(mod)) {
            gAttached = true;
            return true;
        }
        // 模块在但符号两级都解析失败 → 判定不可用
        clearResolved();
        gAttachFailed = true;
        return false;
    }
    return false; // 模块都未加载，ServerStarted 兜底重试仍有机会
}

bool isAttached() { return gAttached; }

bool exportFunc(
    std::string const&       nameSpace,
    std::string const&       funcName,
    RemoteCall::CallbackFn&& callback,
    void*                    handle
) {
    if (!detail::p_exportFunc) return false;
    return detail::p_exportFunc(nameSpace, funcName, std::move(callback), handle);
}

bool hasFunc(std::string const& nameSpace, std::string const& funcName) {
    return detail::p_hasFunc ? detail::p_hasFunc(nameSpace, funcName) : false;
}

RemoteCall::CallbackFn const& importFunc(std::string const& nameSpace, std::string const& funcName) {
    if (detail::p_importFunc) return detail::p_importFunc(nameSpace, funcName);
    return detail::EMPTY_FUNC;
}

bool removeFunc(std::string const& nameSpace, std::string const& funcName) {
    return detail::p_removeFunc ? detail::p_removeFunc(nameSpace, funcName) : false;
}

int removeNameSpace(std::string const& nameSpace) {
    return detail::p_removeNameSpace ? detail::p_removeNameSpace(nameSpace) : 0;
}

void onCallError(std::string const& msg) {
    if (detail::p_onCallError) detail::p_onCallError(msg, ll::sys_utils::getCurrentModuleHandle());
}

std::string translateString(std::string const& text) {
    if (!isAttached() || !hasFunc("MeowPAPI", "translateString")) return text;
    auto fn = importAs<std::string(std::string const&)>("MeowPAPI", "translateString");
    return fn(text);
}

std::string translateStringWithPlayer(std::string const& text, std::string const& playerName) {
    if (!isAttached() || !hasFunc("MeowPAPI", "translateStringWithPlayer")) return text;
    auto fn = importAs<std::string(std::string const&, std::string const&)>(
        "MeowPAPI",
        "translateStringWithPlayer"
    );
    return fn(text, playerName);
}

} // namespace hologramlib::lse
