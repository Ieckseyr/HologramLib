# 假玩家 NPC

纯协议假玩家：PlayerList + AddPlayer，服务端没有这个玩家；支持皮肤、姿势、装备、骑乘与交互轮询。


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
## 新 API（1.31.0）

### 图片平面

`playerNpcRegisterImageSkin(pngPath, skinId, sizeBlocks, transparent)`

- 128×128 PNG；`sizeBlocks=1` 为 1×1 格（128 px/格）；
- `transparent=true`：alpha ≥ 128 不透明，其余挖空；
- `transparent=false`：透明区域铺白；
- 几何 identifier 按尺寸唯一；拼接/摆放由调用方负责。

### 姿势 / 装备 / 骑乘

- `setPose(id, pose)`：`standing` / `sneaking` / `sleeping` / `swimming` / `crawling` 等；
- `setEquipment(id, slot, item, aux, nbt)`；
- `setRideUniqueId(id, entityUniqueId)` / `setRidePlayer(id, playerName)` / `setRideEntity(id, entityId)`；
- `clearRide(id)`。

### 交互

假玩家不在点击回调里直接返回；用 `ghostPollInteractions()` 轮询，条目形如：

```text
player=Steve action=2 domain=npc id=7
```

`action=1` 右键交互，`action=2` 攻击。实体与假玩家两条路径都走 ghost 域。
