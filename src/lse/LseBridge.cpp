// LseBridge.cpp - lrca 运行时挂载实现
//
// mangled 符号串与 LegacyRemoteCall.dll 导出表逐字一致（dumpbin /exports 校验）。
// 任一符号解析失败即判定 lrca 不可用，LSE 兼容层整体禁用（安全降级）。
#include "LseBridge.h"

#include <Windows.h>

#include <mutex>

namespace hologramlib::lse {

namespace {

// LegacyRemoteCall.dll 导出的 7 个 C++ 函数的 MSVC x64 mangled 符号
// （dumpbin /exports LegacyRemoteCall.dll 校验, 2026-08 快照）
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

std::mutex gAttachMutex;
bool       gAttached     = false;
bool       gAttachFailed = false;

template <typename Fn>
Fn resolve(HMODULE module, char const* symbol) {
    if (!module) return nullptr;
    return reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(module, symbol)));
}

} // namespace

bool attach() {
    std::lock_guard<std::mutex> lock(gAttachMutex);
    if (gAttached) return true;
    if (gAttachFailed) return false; // 每进程只尝试一轮, 避免反复扫导出表

    HMODULE lrca = ::GetModuleHandleW(L"LegacyRemoteCall.dll");
    if (!lrca) return false;

    detail::p_exportFunc      = resolve<detail::ExportFuncFn>(lrca, kSymbolExportFunc);
    detail::p_importFunc      = resolve<detail::ImportFuncFn>(lrca, kSymbolImportFunc);
    detail::p_hasFunc         = resolve<detail::HasFuncFn>(lrca, kSymbolHasFunc);
    detail::p_removeFunc      = resolve<detail::RemoveFuncFn>(lrca, kSymbolRemoveFunc);
    detail::p_removeNameSpace = resolve<detail::RemoveNameSpaceFn>(lrca, kSymbolRemoveNameSpace);
    detail::p_onCallError     = resolve<detail::OnCallErrorFn>(lrca, kSymbolOnCallError);

    // exportFunc / importFunc 是导出/导入闭环的最小集, 缺一即放弃
    if (!detail::p_exportFunc || !detail::p_importFunc) {
        detail::p_exportFunc      = nullptr;
        detail::p_importFunc      = nullptr;
        detail::p_hasFunc         = nullptr;
        detail::p_removeFunc      = nullptr;
        detail::p_removeNameSpace = nullptr;
        detail::p_onCallError     = nullptr;
        gAttachFailed             = true;
        return false;
    }

    gAttached = true;
    return true;
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
