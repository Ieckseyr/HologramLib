#pragma once
// 26.60 才有 ServerLevelTickEvent，1.9.7 只有 LevelTickEvent。
#include "ll/api/event/world/LevelTickEvent.h"

namespace ll::event {
using ServerLevelTickEvent = LevelTickEvent;
} // namespace ll::event
