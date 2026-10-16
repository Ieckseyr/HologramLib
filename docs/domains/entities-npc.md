# 能力域指南 · 实体与 NPC

覆盖 4 个域：**自定义实体**、**Ghost 交互**、**假玩家 NPC**、**硫磺立方体展示**。

这一组的共同点是**协议层实体**：它们只存在于客户端，服务端没有对应的真实实体（没有 AI、不落存档、不参与服务端逻辑）。想「让已有的真生物长得不一样」请走 [视图覆盖](view-sensing.md)，不要用这一组。

| 域 | C++ | LSE | 一句话 |
|---|---|---|---|
| 自定义实体 | `ICustomEntity` | `entity*` · 37 | 协议层生成原版实体：姿态 / 装备 / 动画 / 骑乘 |
| Ghost 交互 | 监听器 + 轮询 | `ghost*` · 2 | 非真实实体的交互事件路由 |
| 假玩家 NPC | `IPlayerNpc` | `playerNpc*` · 29 | 纯协议假玩家，皮肤可注册 / 采集 / 注入 |
| 硫磺立方体展示 | `ISulfurDisplay` | `sulfur*` · 10 | 把方块「吞」在立方体上；默认只看到方块 |

---

## 自定义实体

协议层生成一只原版实体，支持姿态、装备槽、动画、ActorLink 骑乘。

### 逐客户端朝向（`1.20.0`）

`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations`（实体与 NPC 通用）让**不同玩家看到不同的朝向** —— 即「千人千面」的朝向版。

`setRotationLight` 是一条**轻脏通道**：只发朝向、不重建实体，同一 tick 内多次调用会合并成一条包。

### 千人千面：逐观看者外观覆盖（`1.21.0`）

| 方法 | 作用 |
|---|---|
| `setPlayerNametag` | 该观看者看到的名字牌 |
| `setPlayerScale` | 该观看者看到的缩放 |
| `setPlayerEquipmentSlot` | 该观看者看到的装备 |
| `clearPlayerAppearance` | 清掉该观看者的全部覆盖 |

出生包与增量包都按观看者覆盖值下发，**装备变更即时单发、无闪烁**。
LSE 侧是 `entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`（`entity*` 尾部 4 个）。

### 属性同步

`setMobProperty`（`26.40.5` 起对外开放）用于下发 `client_sync` 类属性 —— 硫磺立方体的外观档位就走这条路。

> **为什么属性必须「出生之后」再发**：`AddActor` 的 `PropertySyncData` 只装得下按索引的 int / float，**装不下 enum**。所以 enum 属性（如 `sulfur_cube_archetype`）必须在客户端已经认识这只实体之后才能发。库里会在 spawn 之后自动补发、并在 respawn 后重放 —— 这属于「送达保障」，不是替消费方做业务。

---

## Ghost 交互

Ghost = **非真实实体**的交互路由。玩家的 `InteractPacket` 打在一个不是真实体的目标上（比如自定义实体、假玩家 NPC），库把它路由成事件给消费方。

```cpp
auto token = lib.addGhostInteractListener([](hologramlib::GhostInteractEvent const& e) {
    // e.action 是 InteractPacket 的原始 Action 值
});
lib.removeGhostInteractListener(token);
```

- C++ 侧支持**多播监听**（`addGhostInteractListener` / `removeGhostInteractListener` 返回 token，`1.19.1` 起）；旧的单监听 `setGhostInteractListener` 仍在。
- `GhostInteractEvent::action` 是 **`InteractPacket` 的原始 Action 值**：

  | 值 | 含义 |
  |---|---|
  | 1 | Interact（右键） |
  | 2 | Attack（左键攻击） |
  | 3 | StopRiding |
  | 4 | InteractUpdate |
  | 5 | NpcOpen |
  | 6 | OpenInventory |

  处理前先判 `action == 1 || action == 2`，不要假设只有这两种取值。

> **脚本侧拿事件**：`ghostPollInteractions()` 取走并清空待处理队列（返回字符串数组，格式见 [实体、粒子与交互](../lse/entities.md)），`ghostClearInteractions()` 直接丢弃队列。C++ 侧另有 `addGhostInteractListener` 多播监听器。

---

## 假玩家 NPC

纯协议假玩家：客户端看到一个玩家模型，服务端**没有这个玩家**。皮肤可以 PNG 注册、在线采集、目录导入、blob 持久化，还可以注入给指定观看者。

### 皮肤的四条来路

| 来路 | 方法 | 用途 |
|---|---|---|
| PNG 注册 | `registerSkin` | 从 PNG + 可选 `.json` 模型直接注册 |
| 在线采集 | `captureSkin` | 采集在线玩家的皮肤 |
| 目录导入 | `importSkins` | 一个子文件夹 = PNG + 可选 `.json`；**单次导入，库不落盘** |
| blob 持久化 | `getSkinBlob` / `registerSkinFromBlob` | 导出/恢复快照 —— **存盘由消费方负责** |

`PlayerNpcSkin.geometryData` 支持自定义几何模型（`26.10.7` 起）。缩放走 `PlayerNpcConfig.scale` / `playerNpcSetScale`（**0.0625 ~ 10**，碰撞箱等比）。

> **库不落盘。** `26.10.7` 起移除了磁盘存储，改为纯 API —— 皮肤快照经 `getSkinBlob` 导出 → **消费方存盘** → `registerSkinFromBlob` 恢复。
> LSE 侧有 base64 配对：`playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64`（重启后源玩家不在线也能恢复）。

### 皮肤注入：让玩家看到自己被换肤（`26.40.8`）

```cpp
lib.playerNpcs().injectSkin("Steve", "Alex", skinId);   // 只给 Steve 看
lib.playerNpcs().injectSkinAll("Alex", skinId);         // 给所有在线玩家看
```

按 **target 自己的 UUID / uniqueId** 发一条 `PlayerList(Add)`，就地更新皮肤条目。几个关键细节：

- `viewerName` 传**空串 = 所有在线玩家，含 target 本人** —— 这是「**让玩家自己看到自己被换肤**」的正路（Geyser 对 session 玩家自身也是走 PlayerList 而不是 `PlayerSkinPacket`）；
- 条目带真实 xuid、`TrustedSkinFlag = true`；皮肤对象自带 `OverridesPlayerAppearance = true`（**缺它客户端会拒绝覆盖**）；
- **不自动重发** —— 客户端偶发丢帧时由消费方自己再调一次（Geyser 的做法是 ~100ms 后重发）。

### 轻量位置更新（`26.40.8`）

```cpp
if (!lib.playerNpcs().setPositionLight(id, x, y, z, dim)) {
    lib.playerNpcs().setPosition(id, x, y, z, dim);   // 跨维度只能走重建
}
```

只发 `MoveActorAbsolute`，**不重建实体、不重发皮肤、无闪烁**；与 `setRotationLight` 共用同一条轻脏通道，同一 tick 内合并为一条包。
典型用途是「每 tick 跟随一个会移动的东西」（把自定义模型挂在移动中的生物身上）。**`dim` 与当前维度不同时返回 `false`** —— 跨维度必须走 `setPosition` 重建。

### 按名播放动画（`26.40.8`）

```cpp
lib.playerNpcs().playAnimation(id, "animation.npc.wave", "query.any_animation", 40);
lib.playerNpcs().playAnimationTo(id, "Steve", "animation.npc.wave", "", 0);
```

对已见过该 NPC 的玩家发 `AnimateEntityPacket`：

- `animation` = 资源包里的动画标识符；
- `controller` 名由库内**按 NPC id 自动唯一化** —— **不需要资源包里预先存在同名控制器**；
- `stopExpression` 空 = 常驻；`"query.any_animation"` = 立刻停；
- `durationTicks > 0` 到期**自动补发停止包**。

配套 `setEntitySpawnCallback`：NPC 对某玩家出生完成时回调，供消费方补发动画 / 状态（**库发通知，消费方决策**）。与 customentity 域同款队列（+2 tick 发出）。

`setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` 目前**只有 C++ 接口，暂无 LSE 导出**；皮肤一族已补 LSE（`playerNpcInjectSkin` / `playerNpcInjectSkinAll`）。

### 三个实战坑（都是实测踩出来的）

1. **同一 tick 内多次下发会让客户端断线** —— 客户端会收到密集的「新玩家」通知。所以 NPC 的创建与脏刷新**合并到 tick 末尾统一发包**。
2. **不要下发 PlayerList 移除条目** —— 客户端在皮肤条目仍活跃时移除该条目会**崩**。实体照常消失，所以库不下发。
3. **皮肤不渲染的根因在身份证字段**（`26.40.2` 修复）：`PlayerList` 皮肤条目的 `Id` / `FullId` 为空、或残留了原玩家的缓存键，客户端就回退默认模型。现在 PNG 注册 / 在线采集 / blob 恢复统一走 `finalizeSkinIds` 补全 `Id` 并重建 `FullId`，且在 Persona 改写完成后才设置。

---

## 硫磺立方体展示

第二种「摆一个方块」的方式（与 FMBE 的「狐狸 + 物品」并列）：生成一只 `minecraft:sulfur_cube`，把方块**装进它的主手**。

```cpp
hologramlib::SulfurDisplaySpec spec;
spec.block = "minecraft:bookshelf";   // 它「吞」下去的东西
spec.invisible = true;                // 默认就是 true
auto id = lib.sulfurDisplays().create(spec);
```

**这不是我们发明的花招**：行为包里 `entities/sulfur_cube.json` 就是靠 `slot.weapon.mainhand` + `behavior.equip_item` 拿「吞下去」的方块，客户端按装备渲染。所以看起来就是方块被吞在立方体上。放**方块类物品**观感最正。

### 隐身是一个参数，而且默认就是开

**实测（26.40 本机客户端）：立方体隐身时，主手里「吞下去的方块」照常渲染。**
所以 `invisible = true`（默认）的观感就是**一个方块浮在那里**，立方体本体看不见。要连立方体一起看就设 `false`。

> 对比：**NPC 载体的头像是会被隐身一起抹掉的** —— 两个实体行为不同，别套用。

### 外观档位

走**手写的 `ChangeMobProperty`(182)** 下发 `minecraft:sulfur_cube_archetype`（行为包里 `client_sync: true` 的 enum，13 档）：
`none` / `regular` / `bouncy` / `slow_bouncy` / `slow_flat` / `fast_flat` / `light` / `fast_sliding` / `slow_sliding` / `sticky` / `high_resistance` / `explosive` / `hot`。

因为它是 **enum**，装不进出生包的 `PropertySyncData`，所以必须在实体已被客户端认识之后再发 —— 库里会在 spawn 之后自动补发、respawn 后重放。

### 本域完全委托给 `ICustomEntity`

立方体就是一只自定义实体，所以**缩放 / 视距 / 可见玩家白名单 / 逐客户端朝向全都直接继承**。本域新增的协议内容只有 `ChangeMobProperty`。

> **「吞生物」没有做，也不打算做**：协议层实体没有 AI，真正的吞并 / 消化做不到。试过「让另一个实体骑在立方体上」做近似，骑乘位置与碰撞都调不出「被吞进去」的观感（实测不成立），已整体移除。本域只做**方块 + 隐身**。

---

## 相关

- 完整签名：[C++ 接口参考](../reference/cpp-api.md)（§1.6 自定义实体 / §1.7 Ghost / §1.9 假玩家 NPC / §1.15 硫磺立方体）
- LSE 函数清单：[实体、粒子与交互](../lse/entities.md) · [假玩家 NPC](../lse/player-npc.md)
- 想「给真生物换模型」而不是造新实体：[视图覆盖与感知](view-sensing.md)
