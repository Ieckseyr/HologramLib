// DiagLog.h - 库内诊断日志的唯一入口（编译期开关）
//
// HologramLib 是**纯前置库**: 正式构建不携带任何诊断日志 —— 日志字符串根本不进产物。
// 排障时再打开: xmake f --holo_diag=y && xmake build（或设 HOLOGLIB_DIAG_LOG=1）
//
// 用 if constexpr 而不是直接删掉参数: 关闭时参数仍会被**编译检查**（格式串与类型不匹配会
// 报错、只用于日志的变量不会变成"未使用"），但不会生成任何代码, 字符串也不会进二进制。
#pragma once

#include <ll/api/io/Logger.h>

// 开关: 默认 0 = 正式产物不含任何诊断日志。
//    排障打开: xmake f --holo_diag=y && xmake build（走 xmake.lua 的 add_defines, 不经这里）,
//    或临时把它改成 1 / 传 -DHOLOGLIB_DIAG_LOG=1 —— 但**发布前必须回到 0**。
//    这个不变量由 tests/check-no-diagnostics.bat 直接扫产物里的日志串把关。
#ifndef HOLOGLIB_DIAG_LOG
#    define HOLOGLIB_DIAG_LOG 0
#endif

namespace debugshape_export {

// 诊断日志器（"HologramLib" 通道）。关闭诊断时不会被调用, 因此也不会注册该通道。
ll::io::Logger& diagLogger();

} // namespace debugshape_export

#define HOLOG_LIB_LOG(level, ...)                                                                                      \
    do {                                                                                                               \
        if constexpr (HOLOGLIB_DIAG_LOG) {                                                                             \
            ::debugshape_export::diagLogger().level(__VA_ARGS__);                                                       \
        }                                                                                                              \
    } while (0)

#define HLIB_LOG_INFO(...) HOLOG_LIB_LOG(info, __VA_ARGS__)
#define HLIB_LOG_WARN(...) HOLOG_LIB_LOG(warn, __VA_ARGS__)
#define HLIB_LOG_ERROR(...) HOLOG_LIB_LOG(error, __VA_ARGS__)
#define HLIB_LOG_DEBUG(...) HOLOG_LIB_LOG(debug, __VA_ARGS__)
