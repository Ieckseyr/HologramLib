# LeviLamina 的 C++ 包装

LeviLamina 版把这些 ABI 能力包成**冻结的纯虚接口**：消费方 `enable()` 时 `getInstance()` 拿到它，
不需要声明依赖，也不用管实现类怎么变。

!!! tip "先看这个"

    各个接口的完整签名在下面分页里；本页只讲入口、通用约定、线程模型和一条最小可跑的例子。

## 30 秒上手

```cpp
#include "HologramLib/HologramLib.h"

void onEnable() {
    auto& lib = hologramlib::IHologramLib::getInstance();
    if (auto* shapes = &lib.shapes(); shapes != nullptr) {
        shapes->createText({0, 100, 0}, "hello");
    }
}
```

## 接口清单

| 接口 | 取法 | 能力 |
|---|---|---|
| `IShapeDrawer` | `lib.shapes()` | 线框、球、圆、箭头、文字形状 |
| `IHologramText` | `lib.holograms()` | 多行悬浮字、轮播、逐玩家变量 |
| `IItemDetail` | `lib.itemDetails()` | 「物品 + 名称 + Lore」悬浮详情 |
| `IItemDisplay` | `lib.itemDisplays()` | 3D 物品悬浮（FMBE） |
| `ICustomEntity` | `lib.customEntities()` | 协议层自定义实体 |
| `IParticleShape` | `lib.particleShapes()` | 粒子拼形状 |
| `IPlayerNpc` | `lib.playerNpcs()` | 假玩家 NPC |
| `ITradeMenu` | `lib.tradeMenus()` | 村民交易菜单 |
| `INpcDialogue` | `lib.npcDialogs()` | NPC 对话框 |
| `IContainerMenu` | `lib.containerMenus()` | 虚拟容器 / 列表界面 |
| `IPlayerSensing` | `lib.playerSensing()` | 客户端感知、千人千面 |
| `IFakeInventory` | `lib.fakeInventories()` | 假背包 |
| `ISulfurDisplay` | `lib.sulfurDisplays()` | 硫磺方块展示 |
| `IViewOverride` | `lib.viewOverrides()` | 客户端视图覆盖 |
| `IItemActor` | `lib.itemActors()` | 协议层掉落物 |
| `IFormControl` | `lib.formControls()` | 强制关闭表单 |
| `ILocatorBar` | `lib.locatorBars()` | 方位条 |

## 分页

- [1.2 IShapeDrawer · 形状渲染](shape-drawer.md)
- [1.3 IHologramText · 悬浮字](hologram-text.md)
- [1.4 IItemDetail · 物品详情](item-detail.md)
- [1.5 IItemDisplay · 物品悬浮展示](item-display.md)
- [1.6 ICustomEntity · 自定义实体](custom-entity.md)
- [1.7 GhostInteractEvent · 交互事件](ghost-interact.md)
- [1.8 IParticleShape · 粒子形状](particle-shape.md)
- [1.9 IPlayerNpc · 假玩家 NPC](player-npc.md)
- [1.10 ITradeMenu · 交易菜单](trade-menu.md)
- [1.11 INpcDialogue · NPC 对话](npc-dialogue.md)
- [1.12 IContainerMenu · 虚拟容器](container-menu.md)
- [1.13 感知域 · 逐玩家变量](sensing.md)
- [1.14 IFakeInventory · 假背包](fake-inventory.md)
- [1.15 ISulfurDisplay · 硫磺方块](sulfur-display.md)
- [1.16 IViewOverride · 视图覆盖](view-override.md)
- [1.17 IItemActor / IFormControl / ILocatorBar](item-actor-form-locator.md)

## 入口与通用约定

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // 唯一 dllexport 工厂
        IShapeDrawer&   shapes();          // 形状渲染
        IHologramText&  holograms();       // 悬浮字全息
        IItemDetail&    itemDetails();     // 物品详情
        IItemDisplay&    itemDisplays();   // FMBE 物品悬浮（1.6.0）
        ICustomEntity&  customEntities();  // 自定义实体（1.10.0）
        IParticleShape& particleShapes();  // 通用粒子形状（1.14.0）
        IPlayerNpc&     playerNpcs();      // 假玩家 NPC（1.16.0）
        ITradeMenu&     tradeMenus();      // 村民交易菜单（1.21.0）
        INpcDialogue&   npcDialogs();      // NPC 对话框（1.21.0）
        IContainerMenu& containerMenus();  // 虚拟容器 / 列表界面（1.21.0）
        IPlayerSensing& playerSensing();   // 感知域（1.21.0）
        IFakeInventory& fakeInventories(); // 背包虚容器（1.23.0）
        ISulfurDisplay& sulfurDisplays();  // 硫磺立方体展示（1.23.0）
        IViewOverride&  viewOverrides();   // 客户端视图覆盖（1.25.0）
        bool     isLseAvailable();        // LSE 兼容层是否已挂载
        uint32_t version();               // 0x012300 = 1.29.0
        int64_t  findNearestItemDisplay(float x, float y, float z, int dim, double maxDist); // 1.6.0
        void     setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.12.0
        void     clearGhostInteractListener();                                             // 1.12.0
        std::vector<std::string> pollGhostInteractions();                                  // 1.12.0 LSE 轮询版
        uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.19.1 多播
        bool     removeGhostInteractListener(uint64_t token);                               // 1.19.1
        uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener); // 1.27.0 真实实体交互（多播）
        bool     removeActorInteractListener(uint64_t token);                               // 1.27.0
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）；坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；发包在调用线程执行，建议主线程调用
