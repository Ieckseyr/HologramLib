#pragma once

// HologramLib ABI 兼容层（供 C++ 消费方使用）
//
// 为什么需要它
// ------------
// 消费方是通过 `IHologramLib::getInstance().xxx()` **按虚表槽位**取域接口的。
// 库一旦增删中间某个域，后面所有槽位就整体平移，而库自带的
//   abiCompatible(runtime) => runtime >= HOLOGLIB_API_VERSION
// 只是**单向**检查：它能拦住「库比消费方旧」，拦不住「库更新但删了槽位」。
// 26.60.2 删掉 ILocatorBar / ILocatorBarExt 两域就是这种情况——
// 校验通过、槽位整体前移、调用落到别的域上，然后静默失效（不发包、不报错）。
//
// 兼容层做法
// ----------
// 按运行时 version() 选槽：认识的 ABI 走对应槽位表，不认识就返回 nullptr，
// 让消费方显式降级，而不是把调用打歪到别的域上。
//
//   槽号   0x012900（locatorBar 域存在）   0x012A00（域已移除）
//   27     locatorBar()                    playerVisibility()
//   28     playerVisibility()              shapeExt()
//   29     shapeExt()                      bossBars()
//   30     bossBars()                      hud()
//   31     hud()                          actorEffects()
//   32     locatorBarExt()                 sidebar()
//   33     actorEffects()                  playerRespawn()
//   34     sidebar()                       containerPreview()
//   35     playerRespawn()                 actorEffectFilter()
//   36     containerPreview()
//   37     actorEffectFilter()
//
// 27 号槽之前的访问器（shapes/holograms/customEntities/playerNpcs/viewOverrides/
// tradeMenus/npcDialogs/containerMenus/...）两版布局完全一致，直接调用即可，
// 不必经过这里。
//
// 用法
// ----
//   auto* hud = hologramlib::abi::hud(hologramlib::IHologramLib::getInstance());
//   if (!hud) { /* 库版本不认识：本功能降级 */ }
//
// 新版本发布时：若 IHologramLib 再增删中间的域，只需在 slotsOf() 里补一张表，
// 并在 kAbiKnown* 里登记版本号；所有消费方不用重编逻辑。

#include <cstddef>
#include <cstdint>

#include <hologramlib/HologramLib.h>

namespace hologramlib::abi {

enum class Layout : std::uint8_t {
    Unknown = 0,
    LocatorBarPresent, // 0x012900：带 locatorBar()/locatorBarExt() 两槽
    LocatorBarRemoved, // 0x012A00：两槽已删除
};

// 已登记的运行时 ABI（IHologramLib::version() 的返回值）
inline constexpr std::uint32_t kAbiLocatorBarPresent = 0x012900;
inline constexpr std::uint32_t kAbiLocatorBarRemoved = 0x012A00;

// 编译期护栏：头文件 ABI 变了就必须回来补槽位表，否则不让编译
static_assert(
    HOLOGLIB_API_VERSION == kAbiLocatorBarRemoved,
    "HologramLib 头文件 ABI 已变化 —— 请在 AbiCompat.h 里为新的 ABI 补一张槽位表"
);

[[nodiscard]] constexpr Layout detect(std::uint32_t abi) noexcept {
    switch (abi) {
    case kAbiLocatorBarPresent:
        return Layout::LocatorBarPresent;
    case kAbiLocatorBarRemoved:
        return Layout::LocatorBarRemoved;
    default:
        return Layout::Unknown;
    }
}

[[nodiscard]] constexpr char const* layoutName(Layout layout) noexcept {
    switch (layout) {
    case Layout::LocatorBarPresent:
        return "locatorBar 域存在";
    case Layout::LocatorBarRemoved:
        return "locatorBar 域已移除";
    default:
        return "未知布局";
    }
}

// 各布局下「27 号槽及之后」的访问器槽号
struct Slots {
    std::size_t playerVisibility;
    std::size_t shapeExt;
    std::size_t bossBars;
    std::size_t hud;
    std::size_t actorEffects;
    std::size_t sidebar;
    std::size_t playerRespawn;
    std::size_t containerPreview;
    std::size_t actorEffectFilter;
};

[[nodiscard]] constexpr Slots slotsOf(Layout layout) noexcept {
    switch (layout) {
    case Layout::LocatorBarPresent:
        return Slots{28, 29, 30, 31, 33, 34, 35, 36, 37};
    case Layout::LocatorBarRemoved:
        return Slots{27, 28, 29, 30, 31, 32, 33, 34, 35};
    default:
        return Slots{0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
}

// 运行时布局。version() 在 5 号槽，两版布局一致，任何时候取都安全；
// 不做 static 缓存 —— 有的消费方会在库就绪前先探测一次，缓存会把"未就绪"钉死。
[[nodiscard]] inline Layout runtimeLayout(IHologramLib& lib) noexcept {
    try {
        return detect(static_cast<std::uint32_t>(lib.version()));
    } catch (...) {
        return Layout::Unknown;
    }
}

// 按槽号取接口指针；槽号 0 视为「该布局没有这个域」
// MSVC/clang-cl x64 下，无参成员函数（this 走 RCX、返回引用走 RAX）与
// `T& (*)(void*)` 同构，所以可以直接按槽号调用，不必为新旧布局各复刻一份接口。
template <class T>
[[nodiscard]] T* at(IHologramLib& lib, std::size_t slot) noexcept {
    if (slot == 0) return nullptr;
    auto self   = static_cast<void*>(&lib);
    auto vtable = *static_cast<void***>(self);
    using Getter = T& (*)(void*);
    return &reinterpret_cast<Getter>(vtable[slot])(self);
}

#define HOLOGLIB_ABI_ACCESSOR(Name, Field, TYPE)                                                                       \
    [[nodiscard]] inline TYPE* Name(IHologramLib& lib) noexcept {                                                      \
        auto const layout = runtimeLayout(lib);                                                                        \
        if (layout == Layout::Unknown) return nullptr;                                                                 \
        return at<TYPE>(lib, slotsOf(layout).Field);                                                                   \
    }

HOLOGLIB_ABI_ACCESSOR(playerVisibility, playerVisibility, IPlayerVisibility)
HOLOGLIB_ABI_ACCESSOR(shapeExt, shapeExt, IShapeDrawerExt)
HOLOGLIB_ABI_ACCESSOR(bossBars, bossBars, IBossBar)
HOLOGLIB_ABI_ACCESSOR(hud, hud, IHud)
HOLOGLIB_ABI_ACCESSOR(actorEffects, actorEffects, IActorEffect)
HOLOGLIB_ABI_ACCESSOR(sidebar, sidebar, ISidebar)
HOLOGLIB_ABI_ACCESSOR(playerRespawn, playerRespawn, IPlayerRespawn)
HOLOGLIB_ABI_ACCESSOR(containerPreview, containerPreview, IContainerPreview)
HOLOGLIB_ABI_ACCESSOR(actorEffectFilter, actorEffectFilter, IActorEffectFilter)

#undef HOLOGLIB_ABI_ACCESSOR

} // namespace hologramlib::abi
