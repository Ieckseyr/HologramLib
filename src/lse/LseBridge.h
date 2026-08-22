// LseBridge.h - LegacyRemoteCall 运行时可选挂载桥
//
// HologramLib 不再以 LegacyRemoteCall / MeowPAPI 为前置依赖（无链接时符号、
// 无 manifest 声明），而是在运行时通过 GetModuleHandle + GetProcAddress
// 直接解析 lrca 导出的 7 个 C++ 函数（MSVC mangled 符号，与导出表逐字一致）。
//
// 行为契约:
// - lrca 未加载或符号缺失时, attach() 返回 false, 所有导出/导入调用安全空操作
// - lrca 存在时, LSE 脚本 (ll.import / ll.hasExported) 调用方式与旧版完全一致
// - 原生 C++ 接口 (HologramLib.h) 不经过本桥, 不受 lrca 存在与否影响
#pragma once

#include "RemoteCallTypes.h"

#include <string>

namespace hologramlib::lse {

// 尝试挂载 lrca（幂等，重复调用安全）
// 返回: true = 已挂载（本次或之前）；false = lrca 不可用
bool attach();

// 当前是否已挂载
bool isAttached();

// —— 以下函数未挂载时均为安全空操作 ——

bool exportFunc(
    std::string const& nameSpace,
    std::string const& funcName,
    RemoteCall::CallbackFn&& callback,
    void*              handle
);

bool              hasFunc(std::string const& nameSpace, std::string const& funcName);
RemoteCall::CallbackFn const& importFunc(std::string const& nameSpace, std::string const& funcName);
bool              removeFunc(std::string const& nameSpace, std::string const& funcName);
int               removeNameSpace(std::string const& nameSpace);
void              onCallError(std::string const& msg);

// PAPI 占位符翻译（经 lrca 调 "MeowPAPI" 命名空间, 由 MeowSidebar 侧提供）
// lrca / MeowPAPI 不可用时原样返回文本（内置变量兜底由调用方处理）
std::string translateString(std::string const& text);
std::string translateStringWithPlayer(std::string const& text, std::string const& playerName);

namespace detail {

using ExportFuncFn      = bool (*)(std::string const&, std::string const&, RemoteCall::CallbackFn&&, void*);
using ImportFuncFn      = RemoteCall::CallbackFn const& (*)(std::string const&, std::string const&);
using HasFuncFn         = bool (*)(std::string const&, std::string const&);
using RemoveFuncFn      = bool (*)(std::string const&, std::string const&);
using RemoveNameSpaceFn = int (*)(std::string const&);
using OnCallErrorFn     = void (*)(std::string const&, void*);

inline ExportFuncFn      p_exportFunc      = nullptr;
inline ImportFuncFn      p_importFunc      = nullptr;
inline HasFuncFn         p_hasFunc         = nullptr;
inline RemoveFuncFn      p_removeFunc      = nullptr;
inline RemoveNameSpaceFn p_removeNameSpace = nullptr;
inline OnCallErrorFn     p_onCallError     = nullptr;

inline RemoteCall::CallbackFn const EMPTY_FUNC{};

} // namespace detail

inline RemoteCall::ValueType expandArg(std::vector<RemoteCall::ValueType>& args, int& index) {
    return std::move(args[--index]);
}

template <typename RTN, typename... Args>
inline bool
exportAs(std::string const& nameSpace, std::string const& funcName, std::function<RTN(Args...)>&& callback) {
    RemoteCall::CallbackFn cb = [callback = std::move(callback)](std::vector<RemoteCall::ValueType> args)
                                    -> RemoteCall::ValueType {
        if (sizeof...(Args) != args.size()) return {};
        int index = sizeof...(Args);
        if constexpr (std::is_void_v<RTN>) {
            callback(RemoteCall::extract<Args>(expandArg(args, index))...);
            return {};
        } else {
            return RemoteCall::pack(callback(RemoteCall::extract<Args>(expandArg(args, index))...));
        }
    };
    return exportFunc(nameSpace, funcName, std::move(cb), ll::sys_utils::getCurrentModuleHandle());
}

// 泛型转发重载: 允许直接传 lambda（经 std::function CTAD 推导签名, C++23 P2547）
template <typename CB>
inline bool exportAs(std::string const& nameSpace, std::string const& funcName, CB&& callback) {
    return exportAs(nameSpace, funcName, std::function(std::forward<CB>(callback)));
}

template <typename CB, typename Func = std::conditional_t<std::is_function_v<CB>, std::function<CB>, CB>>
inline Func importAs(std::string const& nameSpace, std::string const& funcName) {
    Func callback{};
    callback = [nameSpace, funcName](auto&&... args) {
        auto& rawFunc = importFunc(nameSpace, funcName);
        if (!rawFunc) {
            onCallError(fmt::format("Fail to import! Function [{}::{}] has not been exported", nameSpace, funcName));
            using RTN = std::invoke_result_t<Func, decltype(args)...>;
            return RTN();
        }
        std::vector<RemoteCall::ValueType> params = {RemoteCall::pack(std::forward<decltype(args)>(args))...};
        RemoteCall::ValueType&&            res    = rawFunc(std::move(params));
        using RTN                                 = std::invoke_result_t<Func, decltype(args)...>;
        return RemoteCall::extract<RTN>(std::move(res));
    };
    return callback;
}

} // namespace hologramlib::lse
