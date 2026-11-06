#pragma once

#include "RemoteCallTypes.h"

#include <string>

namespace hologramlib::lse {

bool attach();

bool isAttached();

bool exportFunc(
    std::string const&       nameSpace,
    std::string const&       funcName,
    RemoteCall::CallbackFn&& callback,
    void*                    handle);

bool                          hasFunc(std::string const& nameSpace, std::string const& funcName);
RemoteCall::CallbackFn const& importFunc(std::string const& nameSpace, std::string const& funcName);
bool                          removeFunc(std::string const& nameSpace, std::string const& funcName);
int                           removeNameSpace(std::string const& nameSpace);
void                          onCallError(std::string const& msg);

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
    RemoteCall::CallbackFn cb = [callback = std::move(callback), funcName](std::vector<RemoteCall::ValueType> args)
        -> RemoteCall::ValueType {
        try {
            if (sizeof...(Args) != args.size())
                return {};
            int index = sizeof...(Args);
            if constexpr (std::is_void_v<RTN>) {
                callback(RemoteCall::extract<Args>(expandArg(args, index))...);
                return {};
            } else {
                return RemoteCall::pack(callback(RemoteCall::extract<Args>(expandArg(args, index))...));
            }
        } catch (std::exception const& e) {
            onCallError(fmt::format("{}: {}", funcName, e.what()));
        }
        return {};
    };
    return exportFunc(nameSpace, funcName, std::move(cb), ll::sys_utils::getCurrentModuleHandle());
}

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
        using RTN = std::invoke_result_t<Func, decltype(args)...>;
        try {
            std::vector<RemoteCall::ValueType> params = {RemoteCall::pack(std::forward<decltype(args)>(args))...};
            RemoteCall::ValueType&&            res    = rawFunc(std::move(params));
            return RemoteCall::extract<RTN>(std::move(res));
        } catch (std::exception const& e) {
            onCallError(fmt::format("{}::{}: {}", nameSpace, funcName, e.what()));
        }
        return RTN();
    };
    return callback;
}

} // namespace hologramlib::lse
