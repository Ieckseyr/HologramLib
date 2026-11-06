#pragma once

#include <ll/api/io/Logger.h>

#ifndef HOLOGLIB_DIAG_LOG
#define HOLOGLIB_DIAG_LOG 0
#endif

namespace debugshape_export {

ll::io::Logger& diagLogger();

}

#define HOLOG_LIB_LOG(level, ...)                                 \
    do {                                                          \
        if constexpr (HOLOGLIB_DIAG_LOG) {                        \
            ::debugshape_export::diagLogger().level(__VA_ARGS__); \
        }                                                         \
    } while (0)

#define HLIB_LOG_INFO(...) HOLOG_LIB_LOG(info, __VA_ARGS__)
#define HLIB_LOG_WARN(...) HOLOG_LIB_LOG(warn, __VA_ARGS__)
#define HLIB_LOG_ERROR(...) HOLOG_LIB_LOG(error, __VA_ARGS__)
#define HLIB_LOG_DEBUG(...) HOLOG_LIB_LOG(debug, __VA_ARGS__)
