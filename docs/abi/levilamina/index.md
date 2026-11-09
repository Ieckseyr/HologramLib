# LeviLamina 的 C++ 包装

LeviLamina 版把这些 ABI 能力包成一组**冻结的纯虚接口**：消费方在 `enable()` 里
`getInstance()` 拿到入口，通过访问器取各个域，不需要声明前置依赖，也不用关心实现类怎么变。

- 头文件：`#include "hologramlib/HologramLib.h"`（跨版本取尾部域时另加 `hologramlib/AbiCompat.h`）
- 入口：`hologramlib::IHologramLib::getInstance()`（唯一的 `dllexport` 工厂）
- 版本：`HOLOGLIB_API_VERSION` = `0x012B00` = `1.37.0`（ABI stamp = `2`）
- 平台：LeviLamina 26.60 / BDS 1.26.60（协议 2225）

!!! tip "先跑通一条最小流程"

    下面这条流程能直接抄进插件 `enable()`：校验版本 → 造一个悬浮字 → 画一条线 → 挂一行物品名。
    跑通之后再按「接口清单」查各域的分页。

## 30 秒上手

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x012B00, "需要 HologramLib 1.37.0 (0x012B00) 及以上");

void onEnable() {
    auto& lib = hologramlib::IHologramLib::getInstance();
    if (!hologramlib::abiCompatible(lib.version())) {
        return; // 服务器上的库比头文件旧：后面的域不能调
    }

    // 一行悬浮字
    int64_t holo = lib.holograms().create(100.5f, 66.0f, 100.5f);
    lib.holograms().addLine(holo, "§e主城");
    lib.holograms().draw(holo);

    // 一条红色的线
    int64_t line = lib.shapes().createLine(100.0f, 65.0f, 100.0f, 110.0f, 65.0f, 110.0f);
    lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);
    lib.shapes().draw(line);

    // 一行「Diamond x64」
    lib.itemDetails().show(0, 100.5f, 65.0f, 100.5f, "minecraft:diamond", 0, 64, "");
}
```

## 接口清单

| 接口 | 中文名 | 职责 | 页面链接 |
|---|---|---|---|
| `IShapeDrawer` | 形状渲染 | 线、方框、圆、球、箭头、文本等调试形状 | [1.2 形状渲染](shape-drawer.md) |
| `IShapeDrawerExt` | 形状扩展 | 圆柱、金字塔、椭球、圆锥；最远渲染距离、挂接实体 | [1.2 形状渲染](shape-drawer.md) |
| `IHologramText` | 悬浮字 | 多行悬浮字、内容池轮播、逐玩家变量、跟随玩家 | [1.3 悬浮字](hologram-text.md) |
| `IItemDetail` | 物品详情 | 把物品堆翻译成「物品名 xN」的单行悬浮字 | [1.4 物品详情](item-detail.md) |
| `IItemDisplay` | 物品悬浮展示 | FMBE 狐狸载体的 3D 物品 / 方块悬浮展示 | [1.5 物品悬浮展示](item-display.md) |
| `ICustomEntity` | 自定义实体 | 协议层纯视觉实体（模型、装备、动画、骑乘） | [1.6 自定义实体](custom-entity.md) |
| `IParticleShape` | 粒子形状 | 用原版粒子摆点、线、面、盒、多边形 | [1.8 粒子形状](particle-shape.md) |
| `IPlayerNpc` | 假玩家 NPC | 协议层假玩家（皮肤、姿势、装备、骑乘） | [1.9 假玩家 NPC](player-npc.md) |
| `ITradeMenu` | 交易菜单 | 村民交易界面 | [1.10 交易菜单](trade-menu.md) |
| `INpcDialogue` | NPC 对话 | NPC 对话框与按钮 | [1.11 NPC 对话](npc-dialogue.md) |
| `IContainerMenu` | 虚拟容器 | 任意输入设备都能点的容器 / 列表界面 | [1.12 虚拟容器](container-menu.md) |
| `IPlayerSensing` | 客户端感知 | 查询玩家当前输入设备（键鼠 / 触屏 / 手柄） | [1.13 感知域](sensing.md) |
| `IFakeInventory` | 假背包 | 整包替换客户端背包 | [1.14 假背包](fake-inventory.md) |
| `ISulfurDisplay` | 硫磺方块展示 | 硫磺立方体的外观与档位 | [1.15 硫磺方块](sulfur-display.md) |
| `IViewOverride` | 客户端视图覆盖 | 只改某个玩家看到的模型、皮肤、隐藏状态 | [1.16 视图覆盖](view-override.md) |
| `IItemActor` | 协议层掉落物 | `AddItemActor` 造虚拟掉落物 | [1.17 掉落物 / 表单 / 定位条](item-actor-form-locator.md) |
| `IFormControl` | 表单控制 | 强制关闭客户端表单 | [1.17 掉落物 / 表单 / 定位条](item-actor-form-locator.md) |
| `IPlayerVisibility` | 玩家出站包屏蔽 | 屏蔽出生 / Tab / 移动 / 状态 / 皮肤包 | [1.18 其它接口](misc-interfaces.md) |
| `IBossBar` | Boss 血量条 | 逐玩家的 Boss 条 | [1.18 其它接口](misc-interfaces.md) |
| `IHud` | HUD 元素显隐 | 隐藏准星、血条、热键栏等 HUD | [1.18 其它接口](misc-interfaces.md) |
| `IActorEffect` | 实体表现事件 | 播受伤 / 死亡 / 任意 `ActorEvent` | [1.18 其它接口](misc-interfaces.md) |
| `ISidebar` | 侧边栏计分板 | 逐玩家的侧边栏记分板 | [1.18 其它接口](misc-interfaces.md) |
| `IPlayerRespawn` | 复活包 | 死亡动画后补发 `RespawnPacket`，把客户端拉回游戏 | [1.18 其它接口](misc-interfaces.md) |
| `IContainerPreview` | 容器预览 | 按坐标只读预览一个真实容器 | [1.18 其它接口](misc-interfaces.md) |
| `IActorEffectFilter` | 表现事件阻断 | 按事件 / 玩家 / 实体拦住受伤、死亡等表现包 | [1.18 其它接口](misc-interfaces.md) |

`IContainerPreview` 与 `IActorEffectFilter` 是 1.35.0 追加的尾部域，目前在
[更新日志](../../versions/changelog.md) 的 26.60.1 一节里有说明，还没有单独的分页。

## 怎么拿到实例

`getInstance()` 返回进程级单例的引用，取到之后**不要** `delete`，也不要缓存成裸指针跨模块销毁。
在插件 `enable()` 里取一次、存成成员或就地用完即可：

```cpp
auto& lib = hologramlib::IHologramLib::getInstance();
```

各域访问器（都是无参、返回引用；域取不到只会在 ABI 不匹配时发生，见下文版本校验）：

```cpp
lib.shapes();            // IShapeDrawer      形状渲染
lib.shapeExt();          // IShapeDrawerExt   形状扩展（1.33.0）
lib.holograms();         // IHologramText     悬浮字
lib.itemDetails();       // IItemDetail       物品详情
lib.itemDisplays();      // IItemDisplay      物品悬浮展示（1.6.0）
lib.customEntities();    // ICustomEntity     自定义实体（1.10.0）
lib.particleShapes();    // IParticleShape    粒子形状（1.14.0）
lib.playerNpcs();        // IPlayerNpc        假玩家 NPC（1.16.0）
lib.tradeMenus();        // ITradeMenu        交易菜单（1.21.0）
lib.npcDialogs();        // INpcDialogue      NPC 对话（1.21.0）
lib.containerMenus();    // IContainerMenu    虚拟容器（1.21.0）
lib.playerSensing();     // IPlayerSensing    客户端感知（1.21.0）
lib.fakeInventories();   // IFakeInventory    假背包（1.23.0）
lib.sulfurDisplays();    // ISulfurDisplay    硫磺方块展示（1.23.0）
lib.viewOverrides();     // IViewOverride     客户端视图覆盖（1.25.0）
lib.itemActors();        // IItemActor        协议层掉落物
lib.formControl();       // IFormControl      强制关闭表单
lib.playerVisibility();  // IPlayerVisibility 玩家出站包屏蔽
lib.bossBars();          // IBossBar          Boss 血量条
lib.hud();               // IHud              HUD 元素显隐
lib.actorEffects();      // IActorEffect      实体表现事件
lib.sidebar();           // ISidebar          侧边栏计分板
lib.playerRespawn();     // IPlayerRespawn    复活包
lib.containerPreview();  // IContainerPreview 容器预览
lib.actorEffectFilter(); // IActorEffectFilter 表现事件阻断
```

入口自己还有几个方法：`version()`（返回 `HOLOGLIB_API_VERSION`）、`isLseAvailable()`
（LSE 兼容层是否已挂载）、`findNearestItemDisplay(x, y, z, dim, maxDist)`，以及
`addGhostInteractListener` / `addActorInteractListener` 这类多播监听器。

## 公共约定

### id 语义

- 所有 id 都是 `int64_t`，**每个域各自编号**，从 `1` 开始递增；一个域的 id 不能传给另一个域。
  唯一例外：`IItemDetail::show` 返回的就是悬浮字 id，可以直接交给 `holograms()`。
- `create` 返回 `< 0` 表示失败：一般是 `-1`（创建失败）。
- `createWithId(config, desiredId)` 在 `desiredId <= 0` 或该 id 已被占用时返回 `-2`。
- `createRandom` 在 `0x10000000 ~ 0x7FFFFFFF` 的随机段里取 id，与自增段长期隔离，不会撞车。
- 查询类接口用 `-1` 表示「没找到」：`findNearest` 无匹配返回 `-1`，`IShapeDrawer::type` 对未知 id 返回
  `static_cast<ShapeType>(-1)`。id 不存在的写操作一律返回 `false`，不会抛异常、也不会静默改别的对象。

### 颜色、单位与数值范围

- 颜色分量 `r` / `g` / `b` / `a` 都是 `float`，范围 `0.0 ~ 1.0`；库写入包时量化成 8 位通道，
  所以 `0.5` 与 `128/255` 等价。
- 坐标是**世界坐标**，单位方块；直接传玩家坐标、方块坐标都行，库不做取整。
- 角度（`pitch` / `yaw` / `roll`）单位是**度**，不是弧度；库不做换算。
- 时长 `setDuration(seconds)` 单位是**秒**；动画 / 轮播类参数用 **tick** 或**毫秒**，逐方法在分页里写清。
- 距离（视距、渲染距离）单位是方块；`<= 0` 一律表示「不限制」，不是「看不见」。

### 维度

- 维度 id：`0` 主世界、`1` 下界、`2` 末地。
- `setPosition` 这类同时带维度的接口，传 `dim < 0` 表示**只改坐标、维度不动**。
- `draw` 按全体在线玩家发送；`drawToDimension` 只发给该维度的玩家，并把形状的维度字段设为该值。

### 返回值约定

| 返回 | 含义 |
|---|---|
| `int64_t` | 创建成功是 `>= 1` 的 id；`-1` 创建失败 / 找不到；`-2` 指定 id 被占用 |
| `bool` | `true` = 这次操作生效；`false` = 前置条件不满足（id 不存在、玩家不在线、参数越界、当前没有观看者……逐方法写清） |
| `void` | 批量操作（`destroyAll`、`closeAll`），没有失败可报 |
| `std::string` | 诊断摘要；对象不存在时返回 `"not_found"` |

!!! note "库不落盘"

    形状、悬浮字、实体、菜单都只存在于内存和客户端：服务端重启后全部消失，需要自己存 id 再用
    `createWithId` 恢复；玩家重连由库自动补发，不用消费方处理。
    永久内容**不要**设 `setDuration` —— 不设才是无限期。

### 线程与时机

- 所有方法内部互斥，可以跨线程调用；但**发包在调用线程执行**，建议在主线程（tick 上下文）调用。
- 不要在静态初始化期调 `getInstance()`：此时 BDS 与库都还没就绪。放到插件 `enable()` 或更晚。
- 改动类接口大多只改服务端保存的值，**不会自动上屏**：形状改完要 `update`，悬浮字改完要 `refresh`，
  物品展示 / 自定义实体由库在下一 tick 合并重建。每页的提示块里会写明。

### 版本校验

```cpp
static_assert(HOLOGLIB_API_VERSION >= 0x012B00, "需要 HologramLib 1.37.0");

auto& lib = hologramlib::IHologramLib::getInstance();
if (!hologramlib::abiCompatible(lib.version())) {
    return; // 库比头文件旧：尾部槽位不存在，继续调会打歪
}
```

`abiCompatible(runtime)` 只做**单向**检查（`runtime >= HOLOGLIB_API_VERSION`），能拦住
「库比消费方旧」，拦不住「库更新但删了槽位」。1.36.0 曾删掉 `ILocatorBar` / `ILocatorBarExt`
两个域，`IHologramLib` 虚表中部缩短；**1.37.0 已把 27 / 32 两个槽位原样放回**（空的
`IReservedDomain` 占位，见下），虚表恢复「只追加、不删除」的单调布局。

### ABI 策略（只有一条）

- **只追加**：`IHologramLib` 的域访问器只允许尾部追加，不允许中间插入；
- **要删就留空槽**：下线某个域时，方法保留为 `[[deprecated]]` 非纯虚 + 内联空实现，
  槽位不动（`IHologramText` 里现有 8 个这样的空槽），只抬 `HOLOGLIB_API_VERSION`；
- **换库要重编**：消费方在 `enable()` 里比一次 `abiCompatible(runtime)`，
  升级库后按新头文件重编；`HOLOGLIB_ABI_STAMP` 与 consumer stamp 在加载期挡住忘记重编的旧二进制。

尾部域（`playerVisibility()` / `shapeExt()` / `bossBars()` / `hud()` / `actorEffects()` /
`sidebar()` / `playerRespawn()` / `containerPreview()` / `actorEffectFilter()`）可以直接调；
需要「运行时库太旧就降级」时，用 `AbiCompat.h` 的薄封装（只做版本检查，不猜槽位）：

```cpp
#include "hologramlib/AbiCompat.h"

auto* ext = hologramlib::abi::shapeExt(hologramlib::IHologramLib::getInstance());
if (ext == nullptr) {
    // 运行时库版本低于本编译单元所用版本：这部分功能降级
}
```

## ABI 版本

C++ 接口用 `HOLOGLIB_API_VERSION` 标识，当前 `0x012B00`（1.37.0），配套 `HOLOGLIB_ABI_STAMP = 2`。
只要 `include/hologramlib/*.h` 的虚表布局变化（增删虚函数、调整槽位），这两个数就一起变，
按旧头文件编译的插件必须重新编译。

读取与校验：

```cpp
static_assert(HOLOGLIB_API_VERSION >= 0x012B00, "需要 HologramLib 1.37.0 或更高");

auto& lib = hologramlib::IHologramLib::getInstance();
if (!hologramlib::abiCompatible(lib.version())) {
    // 运行中的库比插件旧，虚表槽位可能不存在：降级或拒绝加载
    return false;
}
```

脚本侧和 Python 侧各有自己的版本号，与 ABI 版本分开变化：LSE 用 `holoLseVersion()`，
endstone 用 `hologramlib_endstone.__lse_version__`。全部历史对照见
[API 版本对照](../../versions/api-version-matrix.md)。

## 完整示例

下面这段把入口、版本校验和三个域串起来，可以直接放进插件的 `enable()`。
跑起来能在主世界 (100.5, 66, 100.5) 看到一行字、一条红线、一行物品名：

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x012B00, "需要 HologramLib 1.37.0 (0x012B00) 及以上");

void spawn_demo() {
    auto& lib = hologramlib::IHologramLib::getInstance();
    if (!hologramlib::abiCompatible(lib.version())) {
        return;
    }

    // 1. 悬浮字：创建 → 加行 → 画出去
    int64_t holo = lib.holograms().create(100.5f, 66.0f, 100.5f);
    if (holo > 0) {
        lib.holograms().addLine(holo, "§6§l主城");
        lib.holograms().addLine(holo, "在线: §a{online}");
        lib.holograms().setDepthTest(holo, false); // false = 穿墙可见（默认）
        lib.holograms().draw(holo);
    }

    // 2. 形状：线 + 颜色 + 显示
    int64_t line = lib.shapes().createLine(100.0f, 65.0f, 100.0f, 110.0f, 65.0f, 110.0f);
    lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);
    lib.shapes().draw(line);

    // 3. 物品详情：show 直接返回悬浮字 id，可继续用 holograms() 精修
    int64_t tip = lib.itemDetails().show(0, 100.5f, 65.0f, 100.5f, "minecraft:diamond", 0, 64, "");
    if (tip > 0) {
        lib.holograms().setScale(tip, 1.2f);
        lib.holograms().refresh(tip);
    }
}
```

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
- [1.18 其它接口](misc-interfaces.md)

---

[← 返回 ABI 总览](../index.md)
