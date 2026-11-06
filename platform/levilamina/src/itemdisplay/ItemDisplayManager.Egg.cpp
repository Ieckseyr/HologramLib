#include "ItemDisplayManager.h"

#include "ItemDisplayRuntimeOps.h"

#include <format>
#include <string>

namespace debugshape_export {
using namespace ide_ops;

bool ItemDisplayManager::playCreeperSwelling(int64_t id, int durationTicks, bool rotate) {
    // 朋友的⑨
    // 展示方块本来就在播 animation.creeper.swelling（buildBlockSwellingExpr），
    // 这里只是把 stretch（必要时连 rotY 一起）换成带 q.life_time 的表达式，
    // 客户端自己逐帧算膨胀/旋转，服务端不跟着推。
    std::lock_guard lock(mMutex);
    auto*           data = configOf(id);
    if (data == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;

    static constexpr char const* kSin = "math.sin(q.life_time*720)";
    auto const                   xz   = std::format(
        "1.0-0.1*math.max({0},0)+0.15*math.max(-{0},0)",
        kSin
    );
    auto const y = std::format("1.0+0.1*math.max({0},0)-0.4*math.max(-{0},0)", kSin);

    // 旋转和膨胀共用同一个角度：q.life_time*720（度/秒），一次肿胀正好转一圈
    static constexpr char const* kSpin = "q.life_time*720";

    for (auto it = mEggs.begin(); it != mEggs.end();) {
        if (it->id == id) {
            it = mEggs.erase(it);
        } else {
            ++it;
        }
    }

    int const ticks = durationTicks > 0 ? durationTicks : 60;

    EggState state{};
    state.id      = id;
    state.endTick = currentTick() + static_cast<std::uint64_t>(ticks);
    state.oldXZ   = rit->stretchXZ;
    state.oldY    = rit->stretchY;
    state.oldRotY = data->rotY;
    state.rotate  = rotate;
    mEggs.push_back(state);

    rit->stretchXZ = xz;
    rit->stretchY  = y;
    if (rotate) {
        data->rotY = kSpin;
    }
    resendFmbeLocked(id, *data, *rit);
    return true;
}

void ItemDisplayManager::tickEasterEggs() {
    std::lock_guard lock(mMutex);
    if (mEggs.empty())
        return;

    auto const now = currentTick();
    for (auto it = mEggs.begin(); it != mEggs.end();) {
        if (it->endTick > now) {
            ++it;
            continue;
        }
        if (auto* rit = runtimeOf(it->id); rit != nullptr) {
            rit->stretchXZ = it->oldXZ;
            rit->stretchY  = it->oldY;
            if (it->rotate) {
                if (auto* data = configOf(it->id); data != nullptr) {
                    data->rotY = it->oldRotY;
                }
            }
            if (auto* data2 = configOf(it->id); data2 != nullptr) {
                resendFmbeLocked(it->id, *data2, *rit);
            }
        }
        it = mEggs.erase(it);
    }
}

} // namespace debugshape_export
