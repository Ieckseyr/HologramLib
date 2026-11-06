#include "ViewOverrideManager.h"

#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace debugshape_export::view {
namespace {

constexpr std::size_t            kSpawnEventMax = 512;
std::mutex                       gSpawnEventMutex;
std::vector<std::string>         gSpawnEvents;
std::unordered_set<std::int64_t> gSpawnSeen;

} // namespace

void pushSpawnEvent(std::string event) {
    std::lock_guard lock(gSpawnEventMutex);
    if (gSpawnEvents.size() >= kSpawnEventMax)
        gSpawnEvents.erase(gSpawnEvents.begin());
    gSpawnEvents.push_back(std::move(event));
}

std::vector<std::string> drainSpawnEvents() {
    std::lock_guard lock(gSpawnEventMutex);
    auto            out = std::move(gSpawnEvents);
    gSpawnEvents.clear();
    return out;
}

bool markSpawnSeen(std::int64_t uniqueId) {
    std::lock_guard lock(gSpawnEventMutex);
    if (!gSpawnSeen.insert(uniqueId).second)
        return false;
    if (gSpawnSeen.size() > 8192)
        gSpawnSeen.clear();
    return true;
}

} // namespace debugshape_export::view
