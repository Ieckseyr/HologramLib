# 1.16 IViewOverride · 视图覆盖

只给某些玩家看一个「改过的世界」：换实体外观、换皮肤、隐藏实体、换方块 —— 服务端、存档、碰撞和其他插件看到的都是真实内容。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().viewOverrides()`
- 粒度：覆盖表按**玩家名**存一份、外加一份**全局表**（`playerName` 传空串）；实体用服务端 `uniqueId` 指定，方块用坐标
- 纪律：**只拦不发** —— 拦截引擎本来要发的包，只决定放行 / 丢弃，从不修改引擎包字段
- 生效：设完立刻下发；另有每 tick 心跳兜底（实体重新进入视野、玩家换区块时把覆盖重推一遍）
- 存储：只在内存里，重启失效；按玩家名的覆盖在该玩家重进后仍然有效

!!! info "只拦不发：要「改」就丢原包 + 自己补发"

    库拦截挂在 `NetworkSystem::send` / `sendToMultiple`（BDS 按收件人发结构化包的汇合点；
    实体出生包不走 `LoopbackPacketSender`，那是早期失效的原因）。它**从不修改引擎包字段**：

    - 换类型 = 丢掉原出生包，用**同一个 `runtimeId` / `uniqueId`** 自己发一只（其余字段照抄原包）
    - 名字牌 = 原包照发，库自己的 `SetActorData` 紧随其后压过去
    - 方块覆盖 = 丢掉原 `UpdateBlock`，库自己发一条带覆盖网络 id 的

    所以服务端世界、存档、碰撞、其他插件看到的全是真实内容。

!!! note "自己不看自己"

    覆盖目标是玩家时，**该玩家本人的客户端不参与覆盖** —— 不发 / 不吃关于他自己实体的包
    （`RemoveActor` / 同 runtimeId 的 `AddActor` / 位移打回本地玩家会让客户端错乱甚至卡死，2026-10-09 实证）。

    例外：换肤类（`asPlayer`）含本人 —— `PlayerList(Add)` 发给本人是「自己看自己被换肤」的正路
    （第三人称 / 第一人称自视都变；同 `IPlayerNpc::injectSkin` 的约定）。

## 实体覆盖

### 覆盖一只实体的外观

`bool overrideEntity(std::string const& playerName, std::int64_t entityUniqueId, EntityView const& view)`

- 参数：
  - `playerName` : `std::string const&`<br>只看这个玩家；**空串 = 所有玩家**（全局表）
  - `entityUniqueId` : `std::int64_t`<br>服务端实体的 `uniqueId`（不是 `runtimeId`）
  - `view` : `EntityView const&`<br>要改成什么样；字段按「设了才改」合并，全空 = 撤销
- 返回值：覆盖已登记 `true`；`false` = 该功能被关闭，或「字段全空 = 撤销」时本来就没有覆盖
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& view = hologramlib::IHologramLib::getInstance().viewOverrides();

    // 换类型 + 换名字牌（只对 Steve）
    hologramlib::EntityView cow;
    cow.identifier        = "minecraft:cow";   // 短名会自动补 minecraft: 前缀
    cow.hasNametag        = true;
    cow.nametag           = "§c奶牛";          // 出现 hasNametag 才改名字牌；空串 = 清空名字
    cow.nametagAlwaysShow = true;
    view.overrideEntity("Steve", uniqueId, cow);

    // 让 Steve 看不到这只实体（阻断它的出生与更新包）
    hologramlib::EntityView gone;
    gone.hidden = true;
    view.overrideEntity("Steve", uniqueId, gone);
    ```

玩家不在线、实体还没出生时也能先设 —— 覆盖会先记下来，等那名玩家上线、那只实体出生时套用
（所以这里的 `true` 只表示「已登记」，不代表当下已经有人看得到）。

!!! note "多次调用是「按字段合并」，不是整体替换"

    只改姿态时给 `pose` + `hasPose` 就行，不会把 `identifier` / `skinId` / 名字牌一起清掉。
    `hidden` 只能设成 `true`（给 `false` 不会撤销），要撤销用 `clearEntity`。
    `scale` / `yOffset` 单独设置不算「有效果」，必须跟 `identifier` / `skinId` / `asPlayer` 之类的某一项一起用。

!!! warning "换类型 / 隐藏 = 重建一次客户端实体"

    换类型、隐藏、撤销全部**立刻生效**：库对这只实体做一次「移除 + 按目标状态重发一只」
    （同一个 `runtimeId` 与 `uniqueId`，服务端后续的移动 / 状态更新照常作用在它身上）。
    代价是客户端那只实体会被重建 —— 只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步才回来。

#### 换成「我们的模型」（`skinId`，只对非玩家实体）

```cpp
hologramlib::EntityView skinned;
skinned.skinId = "ms_demo";   // IPlayerNpc 注册表里的皮肤 id
skinned.scale  = 0.8f;        // 可选：模型缩放（客户端碰撞箱等比；0 = 不缩放）
view.overrideEntity("", mobUniqueId, skinned);   // 空串 = 所有玩家
```

库吃掉这只实体的出生包，用**同一个 `runtimeId` / `uniqueId`** 发 `PlayerList(Add)`（携带该皮肤）+ `AddPlayer` ——
客户端把它当玩家实体渲染（自定义几何 / 贴图都走玩家皮肤通道，不需要资源包）。因为 **id 没变**，
服务端仍认它是那只生物：点它 / 打它 / 瞄准都是它本人（伤害、掉落、其他插件看到的全是真身）。

配套两件事：① 该实体的 `MoveActor*`(18/111) 对这些观看者被吃掉，位置 / 朝向由库每 tick 用
`MoveActorAbsolute` 推（只在动了才发）；② `PlayerList` 条目 ~20 tick 后摘掉（不会一直挂在 Tab 里，实体保留）。
撤销 = `clearEntity`（重发真实生物的出生包）。皮肤必须先在 `IPlayerNpc` 注册表里
（`registerSkin` / `captureSkin` / `registerSkinFromBlob`）。

#### 只对玩家实体：换皮肤 / 变生物

```cpp
// 让 Steve 看到这只玩家变成 Alex 的样子
hologramlib::EntityView skin;
skin.asPlayer = "Alex";
view.overrideEntity("Steve", playerUniqueId, skin);

// 玩家模型渲染不了生物 → 库走「替换」：同一 runtimeId/uniqueId 发一只牛
hologramlib::EntityView asMob;
asMob.identifier = "minecraft:cow";
view.overrideEntity("Steve", playerUniqueId, asMob);
```

- `asPlayer = "<在线玩家名>"` → 给该观看者重发一条 `PlayerList(Add)`（用该实体自己的 UUID + 源玩家的皮肤）。
  与原版「皮肤更新」同机制，**只发给该观看者**；撤销 = 用他自己的皮肤再发一次，立刻恢复。
  对*任何*实体只有玩家实体有效（皮肤按 UUID 认）。
- `identifier = "<生物>"` → 吃掉他的 `AddPlayer`，用**同一个 `runtimeId` / `uniqueId`** 发一只 `AddActor`；
  他的 `MovePlayer` 对这名观看者也吃掉，位置 / 朝向由库按输入包用 `MoveActorAbsolute` 推。
  **id 没变 → 服务端仍认他是那个真玩家，打他就是打他本人。**
  撤销时把替代实体移走，**并把真身补发回该观看者**（`RemoveActor` → `PlayerList(Add)` → `AddPlayer`，
  位置 / 朝向取引擎当前值、游戏模式按输入包），所以撤销是**立刻**生效的，不用重进 / 换维度。
  皮肤数据读不到时库不动客户端（宁可先维持现状，也不做「先删后补不回来」）。

### 撤销一只实体的覆盖

`bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId)`

- 参数：
  - `playerName` : `std::string const&`<br>哪个玩家；空串 = 全局表
  - `entityUniqueId` : `std::int64_t`<br>服务端实体的 `uniqueId`
- 返回值：原来有覆盖并已撤销 `true`（库重发真实出生包，立刻恢复原样）；`false` = 本来就没有覆盖
- 返回值类型：`bool`
- 示例：

    ```cpp
    view.clearEntity("Steve", uniqueId);
    view.clearEntity("", mobUniqueId);   // 撤销全局那一份
    ```

### 读取真实实体信息

`EntityInfo queryEntity(std::int64_t entityUniqueId) const`

- 参数：
  - `entityUniqueId` : `std::int64_t`<br>服务端实体的 `uniqueId`
- 返回值：`EntityInfo`（`identifier` / `nameTag` / `hasNameTag` / `isPlayer` / `health`）；
  `identifier` 为空 = 实体不存在或区块未加载，其余字段也就没有意义
- 返回值类型：`EntityInfo`
- 示例：

    ```cpp
    hologramlib::EntityInfo info = view.queryEntity(uniqueId);
    if (!info.identifier.empty()) {
        // info.identifier = "minecraft:zombie"，info.health = 20.0f
    }
    ```

只读当前**已加载**的实体（`fetchEntity(..., false)`），不会为了查信息去加载区块。

### 把覆盖参数对齐到真实实体

`bool alignEntity(std::string const& playerName, std::int64_t entityUniqueId)`

- 参数：
  - `playerName` : `std::string const&`<br>哪个玩家；空串 = 全局表
  - `entityUniqueId` : `std::int64_t`<br>服务端实体的 `uniqueId`
- 返回值：已按真实实体下发覆盖 `true`；`false` = 实体查不到（没加载 / 不存在）
- 返回值类型：`bool`
- 示例：

    ```cpp
    view.alignEntity("Steve", uniqueId);   // 等价于用 queryEntity 的结果调 overrideEntity
    ```

内部就是「`queryEntity` 取 `identifier` + 名字牌，再 `overrideEntity` 一次」，
适合先拿一只真实生物当模板、再只改其中一两项的场景。

## 方块覆盖

### 覆盖一个方块的显示

`bool overrideBlock(std::string const& playerName, int x, int y, int z, BlockView const& view)`

- 参数：
  - `playerName` : `std::string const&`<br>只看这个玩家；空串 = 所有玩家
  - `x` / `y` / `z` : `int`<br>方块坐标（整格）
  - `view` : `BlockView const&`<br>`type` 填方块类型名，如 `minecraft:diamond_block`
- 返回值：覆盖已登记并下发 `true`；`false` = 类型名不在方块注册表（`type` 空串不算失败，等价于撤销）
- 返回值类型：`bool`
- 示例：

    ```cpp
    hologramlib::BlockView block;
    block.type = "minecraft:diamond_block";
    view.overrideBlock("Steve", 100, 64, 100, block);

    block.type = "diamond_block";      // 短名会自动补 minecraft: 前缀
    view.overrideBlock("", 100, 64, 100, block);   // 全局：所有玩家
    ```

类型名会先去掉首尾空白，不含 `:` 时自动补 `minecraft:` 前缀；**大小写敏感**。
方块的覆盖在区块重发后由库自动补发。

### 撤销一个方块覆盖

`bool clearBlock(std::string const& playerName, int x, int y, int z)`

- 参数：
  - `playerName` : `std::string const&`<br>哪个玩家；空串 = 全局表
  - `x` / `y` / `z` : `int`<br>方块坐标
- 返回值：原来有覆盖并已撤销 `true`（库把**真实方块**推给该玩家）；`false` = 本来就没有覆盖
- 返回值类型：`bool`
- 示例：

    ```cpp
    view.clearBlock("Steve", 100, 64, 100);
    ```

### 清空一名玩家的全部覆盖

`void clearAll(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>哪个玩家；**空串 = 全部玩家**（连全局表一起清）
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    view.clearAll("Steve");
    ```

清完会把真实方块 / 实体推回给受影响的玩家。传具体玩家名时只清他自己那一份，
全局表要靠空串清 —— 否则全局覆盖会在下一次心跳里重新套到他身上。

## 诊断

### 查看当前生效的覆盖

`std::string describeFor(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：一条摘要字符串，形如
  `entities=1 blocks=2 hooks(calls=31 spawn=3 meta=1 block=2 chunk=0) sendFail=0 ctrlSteps=0 skinnedSpawn=1`；
  统计数含全局那一份。玩家名不存在时只有全局的计数
- 返回值类型：`std::string`
- 示例：

    ```cpp
    std::string s = view.describeFor("Steve");
    // entities=1 blocks=2 hooks(calls=31 spawn=3 meta=1 block=2 chunk=0) sendFail=0 ...
    ```

怎么读：`calls` 一直是 `0` 说明出站汇合点没被走到（功能此时由心跳兜底，仍然生效，只是不即时）；
`sendFail` 大于 `0` 说明有包构造 / 发送失败（应上报）。`ctrlSteps` 是历史遗留计数器，正常情况下一直是 `0`。

## 数据结构

`EntityView` —— 一项覆盖（`hasXxx = false` 表示该项不动）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `identifier` | `std::string` | 换成这个实体类型；短名自动补 `minecraft:`；空 = 不改 |
| `asPlayer` | `std::string` | 只对玩家实体：让观看者看到这名玩家的皮肤 |
| `skinId` | `std::string` | 只对非玩家实体：换成 `IPlayerNpc` 注册表里的皮肤（玩家模型通道） |
| `scale` | `float` | 换皮模型的缩放；`0` = 不缩放 |
| `yOffset` | `float` | 换皮模型的 Y 偏移；`0` = 不加偏移 |
| `hidden` | `bool` | `true` = 对该观看者隐藏 |
| `hasNametag` | `bool` | 是否改名字牌（`true` 才看后两项） |
| `nametag` | `std::string` | 新名字（支持 `§`）；空串 = 清空名字 |
| `nametagAlwaysShow` | `bool` | 名字牌是否常显 |
| `hasPose` / `pose` | `bool` / `int` | 姿态，`0..13`（超出钳到边界） |
| `hasMetaScale` / `metaScale` | `bool` / `float` | 元数据缩放（`Reserved38` + `Reserved53·54`，客户端碰撞箱等比） |
| `hasVariant` / `variant` | `bool` / `int` | 变种（`Variant(2)`） |
| `hasMarkVariant` / `markVariant` | `bool` / `int` | 副变种（`MarkVariant(43)`） |
| `hasColorIndex` / `colorIndex` | `bool` / `int` | 颜色（`ColorIndex(3)`） |
| `hasColor2` / `color2` | `bool` / `int` | 第二颜色（`Color2Index(82)`） |

`BlockView` —— 一个方块的覆盖：

| 字段 | 类型 | 说明 |
|---|---|---|
| `type` | `std::string` | 方块类型名（如 `minecraft:diamond_block`）；空串 = 撤销 |

`EntityInfo` —— `queryEntity` 的结果：

| 字段 | 类型 | 说明 |
|---|---|---|
| `identifier` | `std::string` | 真实类型名（`minecraft:zombie`）；空 = 查不到 |
| `nameTag` | `std::string` | 玩家看到的显示名（可能带 `§`） |
| `hasNameTag` | `bool` | 是否有名字牌 |
| `isPlayer` | `bool` | 是不是玩家实体 |
| `health` | `float` | 当前血量 |
| `variant` / `markVariant` | `std::int32_t` | 结构体里的预留位，当前恒为 `0` |

## 机制与限制

- **逐玩家隔离与幂等**：覆盖表按玩家名 + 一份全局表；`sendToClients` / 广播这类「同一包发给多人」的路径
  会展开成逐收件人判定 —— 只为让「放行 / 丢弃」逐人生效；因为从不改字段，不存在「后一个收件人看到前一个覆盖」
  的串味。同一条包被多条发包路径各判一次，结果与判一次相同。库内没有任何覆盖时，钩子不做任何解析就直接放行。
- **换类型只对非玩家生物生效**（玩家出生包 `AddPlayer` 里没有类型字段；玩家实体的名字牌 / 隐藏照常可覆盖）。
- 撤销时离得太远（> 64 格）或已消失的实体不重建（客户端上本来也没有它）。玩家实体走「补发真身」那条路，
  补发出来的真身**手持物品是空的、能力是默认档**，引擎随后自己会纠正（切物品推 `MobEquipment`，
  其余状态变更推 `SetActorData`）。
- **逐玩家「效果类」能力没有全做**：`glowing`（发光）没进 API —— 元数据表里没有这个旗标；
  原版隐身（药水效果，走 `MobEffectPacket`）也没做，要「看不见」用 `hidden`（整只不显示）。
  逐玩家缩放用 `EntityView::metaScale`（1.29.0 起，客户端碰撞箱等比）；
  自定义实体的逐玩家名字牌 / 缩放 / 装备槽请用 `ICustomEntity` 的 per-viewer 覆盖（1.21.0）。
- 方块覆盖在区块重发后由库自动补发。
- 覆盖只存在内存里（重启服务器即失效）；按玩家名的覆盖在该玩家重进后仍然有效。
- 名字牌常显状态没有读口：撤销名字牌覆盖时按「不常显」推回真实名字，
  服务端下次因别的原因重发元数据会带回真实值。

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.25.0 · 26.40.7（26.51 线 26.51.2） | `IViewOverride` 首次发布：实体换类型 / 换名字牌 / 隐藏 / 方块换外观，出站拦截 + 每 tick 心跳 |
| 1.27.0 · 26.40.9 | `EntityView::skinId`（非玩家实体换「我们的模型」，同 id 换皮） |
| 1.29.0 · 26.40.10（26.51 线 26.51.3） | `AddItemActor`(15) 也纳入拦截：`hidden` 丢掉掉落物出生包，`identifier` 用同 id 手写 `AddActor`；`EntityView` 追加 `pose` / `metaScale` / `variant` / `markVariant` / `colorIndex` / `color2`（各带 `hasXxx`） |
| 1.36.0 · 26.60.3 | 尾部追加 `queryEntity` / `alignEntity`；同版删除了 `ILocatorBar` 两个访问器，**旧头文件编译的消费方必须重新编译** |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <cstdint>

void show_as_cow(std::int64_t zombieUniqueId) {
    auto& view = hologramlib::IHologramLib::getInstance().viewOverrides();

    // 1) 先看一眼真实实体（可选）
    hologramlib::EntityInfo info = view.queryEntity(zombieUniqueId);
    if (info.identifier.empty()) {
        return;   // 实体没加载 / 不存在
    }

    // 2) 只对 Steve 换成牛，并改名字牌
    hologramlib::EntityView cow;
    cow.identifier        = "minecraft:cow";
    cow.scale             = 1.0f;
    cow.hasNametag        = true;
    cow.nametag           = "§c奶牛";
    cow.nametagAlwaysShow = true;
    view.overrideEntity("Steve", zombieUniqueId, cow);

    // 3) 只对 Alex 隐藏它
    hologramlib::EntityView gone;
    gone.hidden = true;
    view.overrideEntity("Alex", zombieUniqueId, gone);

    // 4) 把 (100, 64, 100) 在所有客户端显示成钻石块
    hologramlib::BlockView block;
    block.type = "minecraft:diamond_block";
    view.overrideBlock("", 100, 64, 100, block);

    // 5) 撤销
    view.clearEntity("Steve", zombieUniqueId);
    view.clearEntity("Alex", zombieUniqueId);
    view.clearBlock("", 100, 64, 100);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
