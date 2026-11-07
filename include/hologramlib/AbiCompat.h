#pragma once

// ABI 兼容取域：运行时版本够才返回指针，否则 nullptr 让调用方降级。
// 规则只有一条：只追加不删；要下线就留空槽；布局一动抬 ABI stamp，消费方重编。

#include <cstdint>

#include <hologramlib/HologramLib.h>

namespace hologramlib::abi {

// 各尾部域引入的 ABI 版本（= IHologramLib::version() 的返回值）
inline constexpr std::uint32_t kAbiViewOverride       = 0x012900; // 1.35.0
inline constexpr std::uint32_t kAbiContainerMenuTail  = 0x012900; // 1.35.0
inline constexpr std::uint32_t kAbiCurrent            = HOLOGLIB_API_VERSION;

[[nodiscard]] inline bool atLeast(IHologramLib& lib, std::uint32_t required) noexcept {
    try {
        return static_cast<std::uint32_t>(lib.version()) >= required;
    } catch (...) {
        return false;
    }
}

// 取尾部域；运行时库太旧时返回 nullptr，调用方显式降级。
#define HOLOGLIB_ABI_ACCESSOR(Name, Type)                                                                    \
    [[nodiscard]] inline Type* Name(IHologramLib& lib) noexcept {                                            \
        if (!atLeast(lib, kAbiCurrent)) return nullptr;                                                      \
        try {                                                                                                \
            return &lib.Name();                                                                              \
        } catch (...) {                                                                                      \
            return nullptr;                                                                                  \
        }                                                                                                    \
    }

HOLOGLIB_ABI_ACCESSOR(playerVisibility, IPlayerVisibility)
HOLOGLIB_ABI_ACCESSOR(shapeExt, IShapeDrawerExt)
HOLOGLIB_ABI_ACCESSOR(bossBars, IBossBar)
HOLOGLIB_ABI_ACCESSOR(hud, IHud)
HOLOGLIB_ABI_ACCESSOR(actorEffects, IActorEffect)
HOLOGLIB_ABI_ACCESSOR(sidebar, ISidebar)
HOLOGLIB_ABI_ACCESSOR(playerRespawn, IPlayerRespawn)
HOLOGLIB_ABI_ACCESSOR(containerPreview, IContainerPreview)
HOLOGLIB_ABI_ACCESSOR(actorEffectFilter, IActorEffectFilter)

#undef HOLOGLIB_ABI_ACCESSOR

} // namespace hologramlib::abi
