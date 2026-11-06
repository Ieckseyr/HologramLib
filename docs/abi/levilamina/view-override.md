# 1.16 IViewOverride · 视图覆盖

完整签名（`#include "HologramLib/HologramLib.h"`）：

> **1.29.0（26.40.10，26.51 线随 `26.51.3`）补齐: `AddItemActor` 也纳入拦截。** 掉落物（item 实体）的出生包走的是
> `AddItemActor`(id 15) 而不是 `AddActor`(id 13)，此前只拦后者 → "把掉落物换类型/隐藏"只做一半:
> 设覆盖那一刻能把已经在看的玩家换掉，但引擎随后自己发的 `AddItemActor` 会把物品图标又塞回客户端
> （2026-10-16 抓包实证: `RemoveActor` + `AddActor(minecraft:cod)` 之后 13ms 就来一条同 uniqueId 的
> `AddItemActor`）。现在:
> * `hidden = true` → **丢掉**该掉落物的 `AddItemActor`（客户端从头到尾不会看到它）；
> * `identifier = "<生物>"` → 丢掉原包 + 用**同一个 runtimeId/uniqueId** 手写一只 `AddActor`（属性用
>   `health 20 / scale 1` 的已验证配方 —— **不照抄 ItemActor 的 `maxHealth = 0`**，那是另一处会渲染异常的点）；
> * 探测能力: LSE 侧新增 `viewItemSpawnHooked() -> bool`（老库没有这个名字，脚本 `ll.hasExported` 判 false）。
>
> 对 mob 的行为一字未改；对 item 实体是**修复**（此前半残）。

```cpp
auto& view = lib.viewOverrides();

// 实体: 换类型 / 换皮肤（我们的模型）/ 换名字牌 / 对该玩家隐藏（playerName 空串 = 所有玩家）
hologramlib::EntityView cow;
cow.identifier = "minecraft:cow";   // 短名会自动补 minecraft: 前缀
cow.hasNametag = true;
cow.nametag    = "§c奶牛";          // 出现 hasNametag 才改名字牌（值给空串 = 清空名字）
cow.nametagAlwaysShow = true;
view.overrideEntity("Steve", uniqueId, cow);

hologramlib::EntityView gone;
gone.hidden = true;                 // 让 Steve 看不到这只实体（阻断它的出生与更新包）
view.overrideEntity("Steve", uniqueId, gone);

// 非玩家实体: 换成"我们的模型"（已注册皮肤, 含自定义几何）—— **同一个 runtimeId/uniqueId**
hologramlib::EntityView skinned;
skinned.skinId = "ms_demo";         // IPlayerNpc 注册表里的皮肤 id
skinned.scale  = 0.8f;              // 可选: 模型缩放（客户端碰撞箱等比; 0 = 不缩放）
view.overrideEntity("", mobUniqueId, skinned);
// 客户端看到的是我们的模型; 服务端那边还是那只生物 —— 点它/打它/瞄准都是它本人。
// 撤销: view.clearEntity("", mobUniqueId);（库会重发真实生物的出生包, 立刻恢复原样）

// 玩家实体专用的两种"千人千面"（由 Steve 一个人看到）
hologramlib::EntityView skin;
skin.asPlayer = "Alex";             // 让 Steve 看到这只玩家变成 Alex 的样子（皮肤）
view.overrideEntity("Steve", playerUniqueId, skin);

hologramlib::EntityView asMob;
asMob.identifier = "minecraft:cow"; // 玩家模型渲染不了生物 → 库走"替换": 同一 runtimeId/uniqueId 发一只牛
view.overrideEntity("Steve", playerUniqueId, asMob);

// 方块: 把 (100,64,100) 在他客户端显示成钻石块
hologramlib::BlockView block;
block.type = "minecraft:diamond_block";
view.overrideBlock("Steve", 100, 64, 100, block);

view.clearEntity("Steve", uniqueId);   // 撤销
view.clearBlock("Steve", 100, 64, 100);
view.clearAll("Steve");                // 撤销这名玩家的全部覆盖（空串 = 全部玩家）
std::string s = view.describeFor("Steve"); // "entities=1 blocks=2"
```

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `overrideEntity(playerName, entityUniqueId, view)` | bool | false = 玩家不在线。实体还没出生也能先设（库在它出生时套用）。五项（`identifier` / `asPlayer` / `skinId` / `hidden` / `hasNametag`）都没设 = 当作撤销 |
| `overrideBlock(playerName, x, y, z, view)` | bool | false = 玩家不在线 / 类型名不在方块注册表里。type 给空串 = 撤销 |
| `clearEntity(playerName, entityUniqueId)` | bool | 撤销单个实体（返回是否原来有覆盖） |
| `clearBlock(playerName, x, y, z)` | bool | 撤销单个方块，并把**真实方块**推给该玩家 |
| `clearAll(playerName)` | void | 清掉该玩家（空串 = 全部）的覆盖表 |
| `describeFor(playerName)` | string | 该玩家当前生效的覆盖条数摘要（含全局那一份）；带拦截钩子计数与 `sendFail` |
| `startControl(controllerName, targetName, follow)` | bool | 让 controller 的移动/转向输入**驱动 target 的真实移动**（服务端位移）; `follow=true` 时 controller 被锁到 target 上一起走（附身观感） |
| `stopControl(controllerName)` | bool | 停止操控（任一方下线也会自动停） |

**机制（只拦不发）**：库拦截服务端**本来要发给这名玩家的包**，但**只决定放行 / 丢弃** ——
**从不修改引擎包字段**。需要"改"的内容一律：**丢掉原包 + 库自己手写协议包**补发
（sculk 构造 → 回读校验 → 原始字节发送）。换类型 = 丢掉原出生包、用同一 `runtimeId`/`uniqueId`
自己发一只（其余字段照抄原包）; 名字牌 = 原包照发、库自己的 `SetActorData` 紧随其后压过去;
方块覆盖 = 丢掉原 `UpdateBlock`、库自己发一条带覆盖网络 id 的。服务端世界 / 存档 / 碰撞 /
其他插件看到的都是真实内容。拦截挂在 `NetworkSystem::send` / `sendToMultiple`（BDS 按收件人发结构化包的
汇合点；实体出生包不走 `LoopbackPacketSender`，那是原先失效的原因），**外加一层心跳**：服务器每 tick
自查，实体"重新进入视野"、玩家换区块时把覆盖重新推一遍 —— 拦截漏掉的场合由它兜住。

**换"我们的模型"（`skinId`, 1.27.0; 只对非玩家实体）**：库吃掉这只实体的出生包, 用**同一个
`runtimeId`/`uniqueId`** 发 `PlayerList(Add)`（携带该皮肤）+ `AddPlayer` —— 客户端把它当玩家实体渲染
（自定义几何/贴图都走玩家皮肤通道, 不需要资源包）; 因为 **id 没变**, 服务端仍认它是那只生物,
**点它 / 打它 / 瞄准都是它本人**（伤害、掉落、其他插件看到的全是真身）。配套两件事: ①该实体的
`MoveActor*`(18/111) 对这些观看者被吃掉, 位置/朝向由库每 tick 用 `MoveActorAbsolute` 推（与
`IPlayerNpc::setPositionLight` 同一条轻推通道, 只在动了才发）; ②`PlayerList` 条目 ~20 tick 后摘掉
（不会一直挂在 Tab 里, 实体保留）。撤销 = `clearEntity`（重发真实生物的出生包）。
皮肤必须先在 `IPlayerNpc` 注册表里（`registerSkin` / `captureSkin` / `registerSkinFromBlob`）。

**自己不看自己**：覆盖目标是玩家时，**该玩家本人的客户端不参与覆盖**——不发/不吃关于他自己实体的包
（`RemoveActor` / 同 runtimeId 的 `AddActor` / 位移打回本地玩家会让客户端错乱甚至卡死，2026-10-09 实证）。
**例外：换肤类（`asPlayer`）含本人**——`PlayerList(Add)` 发给本人是"自己看自己被换肤"的正路
（第三人称/第一人称自视都变；同 `IPlayerNpc::injectSkin` 的约定）。

**逐玩家隔离与幂等**：覆盖表按玩家名 + 一份全局表；`sendToClients` / 广播这类"同一包发给多人"的路径
会展开成逐收件人判定 —— 只为让"放行 / 丢弃"逐人生效；因为从不改字段，不存在"后一个收件人看到前一个
覆盖"的串味。同一条包被多条发包路径各判一次，结果与判一次相同。库内没有任何覆盖时，钩子不做任何解析
就直接放行。

**玩家实体（1.25.0 支持）**：
- `asPlayer = "<在线玩家名>"` → 给该观看者重发一条 `PlayerList(Add)`（用该实体自己的 UUID + 源玩家的皮肤）
  —— 与原版"皮肤更新"同机制，**只发给该观看者**；撤销 = 用他自己的皮肤再发一次，立刻恢复。
  对*任何*实体只有玩家实体有效（皮肤按 UUID 认）。
- `identifier = "<生物>"` → 玩家模型渲染不了生物，所以库走**替换**（不是代理）：吃掉他的 `AddPlayer`，用**同一个 `runtimeId`/`uniqueId`** 发一只 `AddActor`；他的 `MovePlayer` 对这名观看者也吃掉，位置/朝向由库按他的输入包用 `MoveActorAbsolute` 推。**id 没变 → 服务端仍认他是那个真玩家，打他就是打他本人。** 撤销时把替代实体移走，**并把真身补发回该观看者**：`RemoveActor` → 用他自己身份的 `PlayerList(Add)`（皮肤走玩家皮肤通道）→ `AddPlayer`（位置/朝向取引擎当前值、游戏模式按他输入包里的 PlayMode）。所以撤销是**立刻**生效的，不用重进 / 换维度。皮肤数据读不到时库不动客户端（宁可先维持现状，也不做"先删后补不回来"）。

**26.40 做不到的**：逐玩家的**缩放 / 发光 / 隐身**没进 API —— 元数据表里没有 `scale`，
也没有 `glowing` / `invisible` 旗标（基岩版隐身是效果，走 `MobEffectPacket`）。
自定义实体的逐客户端缩放 / 装备槽请用 `ICustomEntity` 的 per-viewer 覆盖（1.21.0）。

**限制（写清楚免得误用）**：
- **换类型**只对非玩家生物生效（玩家出生包 `AddPlayer` 里没有类型字段；玩家实体的名字牌 / 隐藏照常可覆盖）。
- **换类型 / 隐藏 / 撤销全部立刻生效**：库里对这只实体做一次"移除 + 按目标状态重发一只"（同一个
  `runtimeId` 与 `uniqueId`，所以服务端后续的移动/状态更新照常作用在它身上）。代价是客户端那只实体
  会被重建 —— 只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步才回来。
- 撤销时离得太远（> 64 格）或已消失的实体不重建（客户端上本来也没有它）。玩家实体走"补发真身"那条路（见上）；补发出来的真身**手持物品是空的、能力是默认档**，引擎随后自己会纠正（切换物品会推 `MobEquipment`，其余状态变更会推 `SetActorData`）。
- 方块覆盖在区块重发后由库自动补发。
- 覆盖只存在内存里（重启服务器即失效）；按玩家名的覆盖在该玩家重进后仍然有效。
- 名字牌常显状态没有读口：撤销名字牌覆盖时按"不常显"推回真实名字，服务端下次因别的原因重发元数据会带回真实值。

**诊断**：`describeFor()` 的字符串里带拦截钩子的计数（`hooks(calls=… spawn=… meta=… block=… chunk=…)`）
与 sculk 发送失败数（`sendFail=`）——现象是"设了没反应"时先看它：`calls` 一直是 0 说明汇合点没被走到
（功能此时由心跳兜底，仍然生效，只是不即时）；`sendFail` 大于 0 说明有包构造不合法（应上报）。

判定逻辑（坐标打包 / 区块归属 / 幂等判断 / 名字牌计划 / 类型名清洗）独立在
`src/view/ViewOverrideLogic.h`，离线可测：`tests\check-view-override.bat`（59 项）。

---

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
