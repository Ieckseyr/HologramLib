#include "DiagLog.h"

#include <ll/api/io/LoggerRegistry.h>

namespace debugshape_export {

ll::io::Logger& diagLogger() {
    static auto logger = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *logger;
}

} // namespace debugshape_export
