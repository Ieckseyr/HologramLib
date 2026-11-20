#pragma once

#include <cstdint>
#include <functional>

namespace debugshape_export::core {

using TickToken = std::uint64_t;

// LL 1.9.x 的 EventBus 里没有 level tick 事件（LeviLamina.dll 里连 event id 字符串都没有），
// 所以这里用 Level::$tick 钩子自己凑一个「每帧一次」的 tick 源。
TickToken addTick(std::function<void()> callback);
void      removeTick(TickToken token);

} // namespace debugshape_export::core
