# LSE 接口参考

LegacyRemoteCall（lrca）在场时自动导出。**单命名空间 `HologramLib`**，前缀区分能力域：

| 前缀域 | 能力 | 对应 C++ 接口 | 函数数 |
|--------|------|---------------|--------|
| `shape*` | 形状渲染 | IShapeDrawer | 39 |
| `holo*` | 悬浮字全息 | IHologramText | 26 |
| `gradient*` | 渐变线 | GradientLineManager | 11 |
| `itemDetail*` | 物品详情 | IItemDetail | 2 |
| `itemDisplay*` | FMBE 物品悬浮 | IItemDisplay | 33 |
| `entity*` | 自定义实体 | ICustomEntity | 37 |
| `ghost*` | 交互事件轮询 | IHologramLib | 2 |
| `particle*` | 通用粒子形状系统 | ParticleShapeManager | 22 |
| `playerNpc*` | 假玩家 NPC（含皮肤注册/采集/目录导入/注入/blob 持久化） | IPlayerNpc | 29 |
| `fakeInv*` | 背包虚容器（1.23.0） | IFakeInventory | 9 |
| `sulfur*` | 硫磺立方体展示（1.23.0） | ISulfurDisplay | 10 |
| `trade*` | 村民交易菜单 | ITradeMenu | 10 |
| `container*` | 虚拟容器 / 列表界面 | IContainerMenu | 10 |
| `npcDialog*` | NPC 对话框 | INpcDialogue | 9 |
| `sensing*` | 感知域（客户端设备） | IPlayerSensing | 3 |
| `view*` | 客户端视图覆盖（1.25.0） | IViewOverride | 18 |

上表合计 **270** 个 `exportAs` 调用，运行期**全部注册**。

缺席时安全降级（`ll.import` 得 null）——**这是唯一且显式的失败方式**，不会出现「调用成功但没生效」。

**类型记法**：

| 签名里的类型 | 引擎数据类型 | JavaScript | 说明 |
|---|---|---|---|
| `Float` | `Float` | `Number` | 浮点数 |
| `Integer` | `Integer` | `Number` | 整数；本库的 id / runtimeId / uniqueId 一律为 int64 |
| `String` | `String` | `String` | 字符串 |
| `Boolean` | `Boolean` | `Boolean` | 布尔型 |
| `Null` | `Null` | `null` / `undefined` | 无返回值 |
| `Array<T>` | `Array` | `Array` | 数组；坐标 / 颜色数组按各函数说明为 `[x, y, z]` 或 `[r, g, b, a]` |

可选参数用方括号括起：`foo(name : String[, count : Integer])`。

> 本页只列 HologramLib 自己的 270 个导出。`mc` / `ll` / `logger` / 事件 / NBT / 表单 / 数据库等标准接口见 <https://lse.levimc.org/zh/apis/>。

## 2.1 shape*（形状，39 函数）

**创建**

| 函数 | 签名 |
|------|------|
| shapeCreateText | `(x : Float, y : Float, z : Float, text : String) -> Integer` |
| shapeCreateLine | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |
| shapeCreateBox | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |
| shapeCreateCircle | `(x : Float, y : Float, z : Float, scale : Float) -> Integer` |
| shapeCreateSphere | `(x : Float, y : Float, z : Float, scale : Float) -> Integer` |
| shapeCreateArrow | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |

**属性**

| 函数 | 签名 |
|------|------|
| shapeSetText | `(id, text : String) -> Boolean` |
| shapeGetText | `(id) -> String` |
| shapeSetLocation | `(id, x : Float, y : Float, z : Float) -> Boolean` |
| shapeGetLocation | `(id) -> Array<Float> ([x, y, z])` |
| shapeSetColor | `(id, r : Float, g : Float, b : Float, a : Float) -> Boolean` |
| shapeGetColor | `(id) -> Array<Float> ([r, g, b, a])` |
| shapeSetScale | `(id, scale : Float) -> Boolean` |
| shapeSetDuration | `(id, seconds : Float) -> Boolean` |
| shapeSetDimension | `(id, dimId : Integer) -> Boolean` |
| shapeSetRotation | `(id, pitch : Float, yaw : Float, roll : Float) -> Boolean` |
| shapeClearRotation | `(id) -> Boolean` |
| shapeGetRotation | `(id) -> Array<Float> ([x, y, z])` |
| shapeGetShapeType | `(id) -> Integer`（0..5，见 ShapeType） |
| shapeSetBackgroundColor | `(id, r : Float, g : Float, b : Float, a : Float) -> Boolean`（1.26.0; 仅文本形状, 背景框颜色） |
| shapeClearBackgroundColor | `(id) -> Boolean`（1.26.0; 回客户端默认背景色） |
| shapeSetDepthTest | `(id, enabled : Boolean) -> Boolean`（1.26.0; 仅文本形状, true = 被方块/实体遮挡） |

**显示**

| 函数 | 签名 |
|------|------|
| shapeDraw | `(id) -> Boolean` |
| shapeDrawToPlayer | `(id, playerName : String) -> Boolean` |
| shapeDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| shapeDrawBatch | `(ids: Array<Integer>) -> Boolean` |
| shapeRemove | `(id) -> Boolean` |
| shapeRemoveToPlayer | `(id, playerName : String) -> Boolean` |
| shapeRemoveToDimension | `(id, dimId : Integer) -> Boolean` |
| shapeUpdate | `(id) -> Boolean` |
| shapeUpdateToPlayer | `(id, playerName : String) -> Boolean` |
| shapeUpdateToDimension | `(id, dimId : Integer) -> Boolean` |

**生命周期与查询**

| 函数 | 签名 |
|------|------|
| shapeDestroy | `(id) -> Boolean` |
| shapeDestroyAll | `() -> Null` |
| shapeDestroyBatch | `(ids: Array<Integer>) -> Boolean` |
| shapeFindTextByLocation | `(x : Float, y : Float, z : Float, radius : Float) -> Array<Integer>` |
| shapeFindTextByLocationAndContent | `(x : Float, y : Float, z : Float, radius : Float, text : String) -> Integer` |
| shapeGetAllShapeIds | `() -> Array<Integer>` |
| shapeExists | `(id) -> Boolean` |

## 2.2 holo*（悬浮字，26 函数）

| 函数 | 签名 |
|------|------|
| holoCreate | `(x : Float, y : Float, z : Float) -> Integer` |
| holoDestroy | `(id) -> Boolean` |
| holoDestroyAll | `() -> Null` |
| holoAddLine | `(id, text : String) -> Boolean` |
| holoSetLineText | `(id, lineIndex : Integer, text : String) -> Boolean` |
| holoSetLinePool | `(id, lineIndex : Integer, content: Array<String>, intervalMs : Integer) -> Boolean`（1.26.0; 动态行: 内容池按间隔**时间取模轮播**; 池 ≤1 项 / 间隔≤0 = 不轮播; 空数组 = 清除池） |
| holoSetLineParseVariables | `(id, lineIndex : Integer, enabled : Boolean) -> Boolean`（1.26.0; 该行是否解析变量, 默认开; 关 = `{}` 按字面量显示） |
| holoRemoveLine | `(id, lineIndex : Integer) -> Boolean` |
| holoClearLines | `(id) -> Boolean` |
| holoGetLineCount | `(id) -> Integer` |
| holoSetColor | `(id, r,g,b,a : Float) -> Boolean`（整块文字颜色） |
| holoSetScale | `(id, scale : Float) -> Boolean`（整块缩放; 替代原 holoSetLineScale） |
| holoSetBackgroundColor | `(id, r,g,b,a : Float) -> Boolean`（背景框颜色; 不设 = 客户端默认色） |
| holoClearBackgroundColor | `(id) -> Boolean`（回客户端默认背景色） |
| holoSetDepthTest | `(id, enabled : Boolean) -> Boolean`（true = 被方块/实体遮挡; false = 始终渲染/穿墙可见, 默认） |
| holoSetRotation | `(id, pitch : Float, yaw : Float, roll : Float) -> Boolean`（三轴 Euler, **单位 = 度**; 设置后不再面向相机） |
| holoClearRotation | `(id) -> Boolean`（恢复面向相机） |
| holoSetLocation | `(id, x : Float, y : Float, z : Float) -> Boolean` |
| holoSetDimension | `(id, dimId : Integer) -> Boolean`（迁移维度; 已绘制时原地重发, 无闪烁） |
| holoSetFollowPlayer | `(id, playerName : String, offsetY : Float) -> Boolean`（位置在 setFollowPlayer/draw/refresh 时就地解析） |
| holoClearFollowPlayer | `(id) -> Boolean` |
| holoDraw | `(id) -> Boolean` |
| holoDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| holoDrawToPlayer | `(id, playerName : String) -> Boolean` |
| holoRemove | `(id) -> Boolean` |
| holoRefresh | `(id) -> Boolean`（重解析变量/跟随坐标并原地重发） |

> **1.26.0 收缩**：原 `holoSetLineScale` / `holoSetLineColor` / `holoSetLineGradient` / `holoSetLineRainbow` /
> `holoSetLineScroll` / `holoSetVerticalAnimation` / `holoSetLineSpacing` / `holoTick` 已移除（这些在"整块
> 单形状"模型下不可实现或空转）。行级颜色请改用文本内嵌 § 颜色代码; 行级缩放改 `holoSetScale`。

## 2.3 gradient*（渐变线，11 函数）

| 函数 | 签名 |
|------|------|
| gradientCreate | `(x1,y1,z1 : Float, x2,y2,z2 : Float, segments : Integer) -> Integer` |
| gradientSetGradient | `(id, r1,g1,b1,r2,g2,b2 : Float) -> Boolean` |
| gradientSetRainbow | `(id, speed : Float) -> Boolean` |
| gradientSetColor | `(id, r,g,b,a : Float) -> Boolean` |
| gradientSetEndpoints | `(id, x1,y1,z1 : Float, x2,y2,z2 : Float) -> Boolean` |
| gradientDraw | `(id) -> Boolean` |
| gradientDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| gradientRemove | `(id) -> Boolean` |
| gradientDestroy | `(id) -> Boolean` |
| gradientDestroyAll | `() -> Null` |
| gradientTick | `(deltaTime : Float) -> Null`（彩虹动画驱动） |

## 2.4 itemDetail*（物品详情，2 函数）

| 函数 | 签名 |
|------|------|
| itemDetailShow | `(dimId : Integer, x,y,z : Float, itemId : String, aux : Integer, count : Integer, customText : String) -> Integer` |
| itemDetailHide | `(id) -> Boolean` |

`itemDetailShow`：在 (x,y,z) 显示"本地化物品名 xN"（count<=1 无数量后缀）；`customText` 传 `""` 用自动文本，非空则完全替代（支持 § 颜色码与 `{变量}`）。返回悬浮字 ID，可继续用 `holo*` 精修。

## 2.5 itemDisplay*（FMBE 物品悬浮显示，33 函数）

FMBE（狐狸+发包）技术：隐形狐狸手持物品渲染任意物品/方块的悬浮展示。三轴旋转/函数平移/缩放全部支持 **Molang 表达式**（如 `"math.sin(query.life_time*90)*360"`）。

| 函数 | 签名 |
|------|------|
| itemDisplayCreate | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer) -> Integer` |
| itemDisplayCreateAdvanced | `(x,y,z : Float, dim : Integer, itemId : String, aux : Integer, offX,offY,offZ : String, rotX,rotY,rotZ : String, scale : String) -> Integer` |
| itemDisplayCreateSeamless | `(x,y,z : Float, dim : Integer, itemId : String, aux : Integer, offX,offY,offZ : String, rotX,rotY,rotZ : String, scale : String, mode : Integer, viewDistance : Float, visiblePlayer : String) -> Integer`（无感创建: mode/视距/白名单在首次 spawn 之前写入, 全程只发一次 Add 序列; 创建后无需 setMode/setViewDistance/setVisiblePlayer。mode: -1=默认 auto 1=item 2=block; viewDistance: -1=默认 0=不限; visiblePlayer 空串=全员） |
| itemDisplayCreateRandom | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer) -> Integer`（随机段 `[0x10000000,0x7FFFFFFF)` 不重复 ID） |
| itemDisplayCreateWithId | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer, desiredId : Integer) -> Integer`（指定 ID 创建, 持久化恢复用; <=0 或已占用返回 -2） |
| itemDisplayDestroy | `(id) -> Boolean` |
| itemDisplayDestroyAll | `() -> Null` |
| itemDisplayExists | `(id) -> Boolean` |
| itemDisplayIsIdUsed | `(id) -> Boolean` |
| itemDisplayScaleBy | `(id, factor : Float) -> Boolean`（相对缩放: factor>1 放大, 0<factor<1 缩小） |
| itemDisplaySetItemWithNbt | `(id, itemId : String, aux : Integer, nbt : String) -> Boolean`（换物品带附加数据: nbt 为 SNBT; 空串清除） |
| itemDisplaySetGlint | `(id, on : Boolean) -> Boolean`（附魔光效开关: BDS 原生路径注入 1 级锋利, 客户端紫色光效） |
| itemDisplaySetVisiblePlayers | `(id, playerNames: Array<String>) -> Boolean`（可见玩家白名单: 按玩家名/LSE `player.realName` 匹配; 空列表 = 全员可见; 维度/视距条件仍叠加） |
| itemDisplayClearVisiblePlayers | `(id) -> Boolean`（清除白名单, 恢复全员可见） |
| itemDisplaySetVisiblePlayer | `(id, playerName : String) -> Boolean`（标量版单玩家白名单; JS 侧推荐） |
| itemDisplayGetInfo | `(id) -> String`（诊断探针: 返回运行态摘要; 未找到返回 `not_found`） |
| itemDisplayScaleTo | `(id, targetScale : Float) -> Boolean`（单次跳变缩放: 以新 scale 常量重发完整方块动画序列, 无 respawn、无 Remove/Add; 渐变动画由消费者步进驱动（如 5 步 easeOutCubic）。仅方块模式; scale<0.01 钳制为 0.01） |
| itemDisplayGetAllIds | `() -> Array<Integer>` |
| itemDisplaySetItem | `(id, itemId : String, aux : Integer) -> Boolean`（换物品; 清除附加数据） |
| itemDisplaySetPosition | `(id, x,y,z : Float, dim : Integer) -> Boolean`（dim<0 仅改坐标） |
| itemDisplaySetOffset | `(id, ox,oy,oz : String) -> Boolean`（模型单位平移; Molang） |
| itemDisplaySetBaseOffset | `(id, ox,oy,oz : String) -> Boolean`（渲染像素基础偏移; Molang） |
| itemDisplaySetRotation | `(id, rx,ry,rz : String) -> Boolean`（三轴旋转/度; Molang） |
| itemDisplaySetScale | `(id, scale : String) -> Boolean`（Molang） |
| itemDisplaySetExtend | `(id, scale,rx,ry : String) -> Boolean`（方块模式二段变换） |
| itemDisplaySetMode | `(id, mode : Integer) -> Boolean`（0=auto 1=item 2=block） |
| itemDisplaySetEnabled | `(id, enabled : Boolean) -> Boolean` |
| itemDisplaySetViewDistance | `(id, dist : Float) -> Boolean`（<=0 无限制） |
| itemDisplayRotateY | `(id, delta : Float) -> Boolean`（偏航叠加增量） |
| itemDisplayFindNearest | `(x,y,z : Float, dim : Integer, maxDist : Float) -> Integer`（最近查找; 无匹配 -1） |
| itemDisplayFollow | `(id, playerName : String, offX,offY,offZ : Float) -> Boolean`（1.17.0 展示跟随玩家: 每 tick 同步目标实时坐标（含跨维度自动 respawn）, 对已见玩家发 MoveActorAbsolute（非 teleport, 客户端插值）平滑位移, 全程无 respawn 无闪烁; 玩家下线自动解除跟随（方块原地保留）; `itemDisplaySetPosition` 手动设位解除跟随） |
| itemDisplayUnfollow | `(id) -> Boolean`（1.17.0 解除跟随; 无跟随关系返回 true） |
| itemDisplaySetHitbox | `(id, width : Float, height : Float) -> Boolean`（1.17.0 AABB 判定体积: 对已见玩家广播 SetActorDataPacket(R53=Width R54=Height), 即时无 respawn; 数值持久入配置, respawn/新观察者自动按当前值发包; 0/0 = 恢复不可命中。开启后客户端射线可命中 → 攻击经 `ghost*` 路由回库内 id） |

渲染模式：`auto`（默认）按物品 3D/2D 自动选择；`item` 平面物品渲染（rotY 内部自动 +205 补偿狐狸头朝向）；`block` 3D 方块渲染（完整旋转矩阵路径 + 二段扩展变换）。可见性由库内 Level tick hook 自动同步。

## 2.6 entity*（自定义实体，37 函数）

AddActorPacket 直发客户端生成纯视觉实体（NPC 壳/装饰生物/盔甲架布景; 无碰撞、不可真实交互——点击经 `ghost*` 路由）。属性变更经 tick 脏刷新合并为单次 respawn（无闪烁）。

**创建与生命周期**

| 函数 | 签名 |
|------|------|
| entityCreate | `(identifier : String, x,y,z : Float, dim : Integer) -> Integer` |
| entityCreateAdvanced | `(identifier : String, x,y,z : Float, dim : Integer, yaw : Float, pitch : Float, scale : Float, nametag : String) -> Integer` |
| entityCreateRandom | `(identifier : String, x,y,z : Float, dim : Integer) -> Integer`（随机段 ID） |
| entityCreateWithId | `(identifier : String, x,y,z : Float, dim : Integer, desiredId : Integer) -> Integer`（指定 ID 创建; <=0 或占用返回 -2） |
| entityDestroy | `(id) -> Boolean` |
| entityDestroyAll | `() -> Null` |
| entityExists | `(id) -> Boolean` |
| entityIsIdUsed | `(id) -> Boolean` |
| entityGetAllIds | `() -> Array<Integer>` |

**属性**

| 函数 | 签名 |
|------|------|
| entitySetIdentifier | `(id, identifier : String) -> Boolean`（换实体类型; 短名自动补 `minecraft:`） |
| entitySetPosition | `(id, x,y,z : Float, dim : Integer) -> Boolean`（dim<0 仅改坐标） |
| entitySetRotation | `(id, yaw : Float, pitch : Float) -> Boolean`（度） |
| entitySetNametag | `(id, text : String) -> Boolean`（支持 § 颜色码; 空串清除） |
| entitySetScale | `(id, scale : Float) -> Boolean`（<=0 拒绝; 自动钳制 0.0625~10） |
| entitySetVariant | `(id, variant : Integer) -> Boolean` |
| entitySetMarkVariant | `(id, markVariant : Integer) -> Boolean` |
| entitySetColorIndex | `(id, colorIndex : Integer) -> Boolean`（羊/项圈等染色实体） |
| entitySetFlags | `(id, flags : Integer) -> Boolean`（原始位掩码 0x01=着火 0x20=隐身 等） |
| entitySetInvisible | `(id, on : Boolean) -> Boolean`（隐身便捷开关, 幂等） |
| entitySetEnabled | `(id, enabled : Boolean) -> Boolean` |
| entitySetViewDistance | `(id, dist : Float) -> Boolean`（<=0 无限制） |
| entitySetPose | `(id, pose : Integer) -> Boolean`（PoseIndex 0..13; 盔甲架坐姿/睡姿/跳舞等） |
| entitySetEquipmentSlot | `(id, slot : Integer, name : String, aux : Integer, nbt : String) -> Boolean`（槽位 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; name 空清空; nbt 为 SNBT） |
| entityFindNearest | `(x,y,z : Float, dim : Integer, maxDist : Float) -> Integer`（最近查找; 无匹配 -1） |

**扩展能力**

| 函数 | 签名 |
|------|------|
| entityScaleBy | `(id, factor : Float) -> Boolean`（相对缩放: 现有 scale × factor, 自动钳制 0.0625~10） |
| entitySetVisiblePlayers | `(id, playerNames: Array<String>) -> Boolean`（可见玩家白名单; 空列表 = 全员可见） |
| entitySetVisiblePlayer | `(id, playerName : String) -> Boolean`（单玩家白名单标量版; JS 侧推荐） |
| entityClearVisiblePlayers | `(id) -> Boolean`（恢复全员可见） |
| entityGetInfo | `(id) -> String`（诊断探针; 未找到返回 `not_found`） |
| entitySetRidePlayer | `(id, playerName : String) -> Boolean`（实体骑到指定玩家头上; 空名清除; 玩家须在线） |
| entitySetRideEntity | `(id, vehicleEntityId : Integer) -> Boolean`（骑到另一自定义实体上; 0 清除） |
| entityClearRide | `(id) -> Boolean`（解除骑乘链接） |
| entityPlayAnimation | `(id, animation : String, stopExpression : String, durationTicks : Integer) -> Boolean`（播放原版动画, 如 `animation.humanoid.base_pose`; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止） |
| entitySetPlayerNametag | `(id, playerName : String, text : String) -> Boolean`（1.22.0; 只改该玩家看到的名字牌; 空串 = 清除覆盖回到 config） |
| entitySetPlayerScale | `(id, playerName : String, scale : Float) -> Boolean`（1.22.0; 只改该玩家看到的缩放; <=0 = 清除覆盖） |
| entitySetPlayerEquipmentSlot | `(id, playerName : String, slot : Integer, name : String, aux : Integer, nbt : String) -> Boolean`（1.22.0; 0=主手 1=副手 2=头 3=胸 4=腿 5=脚; name 空 = 回退 config） |
| entityClearPlayerAppearance | `(id, playerName : String) -> Boolean`（1.22.0; 清除该玩家的名字/缩放/装备覆盖） |

## 2.7 ghost*（交互事件轮询，2 函数）

| 函数 | 签名 |
|------|------|
| ghostPollInteractions | `() -> Array<String>`（取走并清空待处理交互队列; 队列上限 256 条, 满时丢最旧） |
| ghostClearInteractions | `() -> Null`（丢弃队列中全部待处理事件） |

事件条目为可解析字符串，格式：

```
player=<realName> action=<1..6> domain=<entity|itemDisplay|npc> id=<库内id> pos=(x,y,z)
```

- `action`：1=Interact（右键交互）2=Attack（攻击/左键）3=StopRiding 4=InteractUpdate 5=NpcOpen 6=OpenInventory
- `pos` 仅在客户端携带坐标时出现（InteractUpdate 等）
- 示例：`player=Steve action=2 domain=entity id=42` —— 玩家 Steve 左键点击了库内 id=42 的自定义实体

LSE 轮询示例：

```js
const poll = ll.import("HologramLib", "ghostPollInteractions");
setInterval(() => {
    for (const line of poll()) {
        const m = line.match(/player=(\S+) action=(\d+) domain=(\S+) id=(-?\d+)/);
        if (!m) continue;
        const [, player, action, domain, id] = m;
        if (action === "2" && domain === "entity") { /* 左键点击 NPC */ }
    }
}, 200);
```

## 2.8 particle*（通用粒子形状系统，22 函数）

形状（点/线/矩形环/填充面/长方体框/六面/多面体）+ 变换（平移/平滑移动/旋转/自旋/缩放/跟随玩家）+ 多玩家白名单 + 逐玩家视距裁剪，全部由 C++ tick 自驱动。发送走 vanilla 通道，BDS tick flush 自动聚合压缩为单 Batch 数据报。

**核心概念**：
- **形状**：创建时按世界坐标定义，内部换算为局部坐标（锚点 = 线中点/矩形中心/盒中心/多面体质心），采样点缓存复用
- **变换**：`world = anchor + R(欧拉ZYX, 度) · (scale · local)`；`setPos` 即"粒子移动"（下次发射按新位置整批重发）；`moveTo` 为平滑点对点移动（锚点 easeOutCubic 插值逼近目标，单点/整面（plane/rect）/整体形状（box/poly）均随锚点整体移动）；`spin` 为度/tick 自旋；`follow` 时锚点每 tick = 玩家位置 + 偏移（跨维度自动跟随；moveTo/setPos 会解除跟随）
- **可见性**：白名单（uuid CSV）空 = 形状所在维度全员；逐玩家 3D 视距裁剪（`viewDistance` 0 = 不裁剪）
- **生命周期**：`lifetimeTicks` 0 = 永久；`setLifetime` 从现在起重新计时；粒子瞬态 → 每 `intervalTicks` 整批重发维持常驻视觉
- **平面轴**（rect/plane `axis` 参数）：0=XY（w 沿 X、h 沿 Y）、1=YZ（w 沿 Z、h 沿 Y）、2=XZ（w 沿 X、h 沿 Z）

| 函数 | 签名 |
|------|------|
| particleCreatePoint | `(owner : String, dimId : Integer, x : Float, y : Float, z : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` |
| particleCreateLine | `(owner : String, dimId : Integer, x1,y,z1 : Float, x2,y2,z2 : Float, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer`（锚点=中点, 旋转绕中点） |
| particleCreateRect | `(owner : String, dimId : Integer, cx,cy,cz : Float, w : Float, h : Float, axis : Integer, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer`（矩形环线） |
| particleCreatePlane | 同 rect 参数（填充平面网格） |
| particleCreateBox | `(owner : String, dimId : Integer, cx,cy,cz : Float, hx,hy,hz : Float, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer`（长方体 12 边线框, 半尺寸） |
| particleCreateBoxFaces | 同 box 参数（六面填充） |
| particleCreatePoly | `(owner : String, dimId : Integer, verts : String, edges : String, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer`（verts `"x,y,z;x,y,z"`世界坐标锚点=质心; edges `"i-j"`顶点索引对） |
| particleSetPos | `(id : Integer, x, y, z : Float) -> Boolean`（平移锚点; 解除跟随） |
| particleMoveBy | `(id : Integer, dx, dy, dz : Float) -> Boolean`（相对平移） |
| particleMoveTo | `(id : Integer, x, y, z : Float, durationTicks : Integer) -> Boolean`（平滑点对点移动, easeOutCubic; 解除跟随; duration<=0 = 立即到达） |
| particleSetRot | `(id : Integer, rx, ry, rz : Float) -> Boolean`（欧拉角, 度, ZYX 序, 绕锚点） |
| particleSpin | `(id : Integer, sx, sy, sz : Float) -> Boolean`（自旋速率, 度/tick; 0,0,0 停止） |
| particleSetScale | `(id : Integer, s : Float) -> Boolean` |
| particleFollow | `(id : Integer, uuid : String, offX, offY, offZ : Float) -> Boolean`（锚点跟随玩家+偏移, 每 tick） |
| particleUnfollow | `(id : Integer) -> Boolean` |
| particleSetEffect | `(id : Integer, effect : String) -> Boolean` |
| particleSetVisible | `(id : Integer, playersCsv : String) -> Boolean`（`"uuid1,uuid2"` 白名单; 空串 = 维度全员） |
| particleSetInterval | `(id : Integer, ticks : Integer) -> Boolean` |
| particleSetViewDistance | `(id : Integer, blocks : Integer) -> Boolean`（逐玩家 3D 裁剪; 0 = 不裁剪） |
| particleSetLifetime | `(id : Integer, ticks : Integer) -> Boolean`（从现在起; 0 = 永久） |
| particleDestroy | `(id : Integer) -> Boolean` |
| particleGetInfo | `(id : Integer) -> String`（诊断探针） |

LSE 示例（旋转自旋立方体 + 跟随玩家的光圈）：

```js
const create = ll.import("HologramLib", "particleCreateBox");
const spin   = ll.import("HologramLib", "particleSpin");
const follow = ll.import("HologramLib", "particleFollow");
const rot    = ll.import("HologramLib", "particleSetRot");
const vis    = ll.import("HologramLib", "particleSetVisible");

// 1. 自旋立方体线框（绕中心旋转）
const box = create("demo", 0, 100.5, 64.5, 100.5, 3, 3, 3, 1, "minecraft:endrod", 2, 0);
spin(box, 0, 3, 0);            // 每 tick 绕 Y 转 3 度
vis(box, player.uuid);          // 仅该玩家可见

// 2. 跟随玩家脚下的光圈（plane XZ + follow）
const ring = ll.import("HologramLib", "particleCreateRect");
const halo = ring("demo", 0, 0, 0, 0, 6, 6, 2, 1.5, "minecraft:endrod", 2, 0);
follow(halo, player.uuid, 0, 0.3, 0); // 锚点 = 玩家位置 + 偏移, 每 tick 更新

// 手动旋转 45 度（覆盖自旋累计值）
rot(box, 45, 0, 0);
```

## 2.9 playerNpc*（假玩家 NPC，29 函数）

> **NPC 皮肤已可正常渲染（26.40.2 修复）**。下列函数全部可用（注册/采集/导入/换肤/导出均正常），NPC 创建、移动、朝向、缩放、视距与显隐控制均正常渲染。

纯协议假玩家（不占服务端实体系统）：`PlayerList(Add, 皮肤) → AddPlayer → [20t] PlayerList(Remove)`；点击交互经 ghost 管线 `domain="npc"`（§2.7 轮询）。皮肤三来源：PNG 文件注册 / 目录批量导入（一个子文件夹 = PNG + 可选 `.json` 模型）/ 从在线玩家采集（全字段运行时快照，玩家之后换肤不影响）。库不落盘，LSE 侧如需跨重启保留请重新注册/采集。

`setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` 为 **C++ 接口，暂无 LSE 导出**（同 `setRotationLight`）。
皮肤一族（26.40.8 补导出）：`playerNpcInjectSkin` / `playerNpcInjectSkinAll` / `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64`（见下表）。

| 函数 | 签名 |
|------|------|
| playerNpcRegisterSkin | `(pngPath : String, skinId : String, geometry : String, armSize : String) -> Boolean`（skinId 空 = 文件名; geometry 空 = geometry.humanoid.custom; armSize: wide/slim） |
| playerNpcImportSkins | `(dirPath : String) -> Integer`（目录批量导入: 一个子文件夹 = 一套皮肤, PNG 必需 + `.json` 几何可选; skinId = 文件夹名; 返回导入数量, 目录无效 -1） |
| playerNpcCaptureSkin | `(skinId : String, playerName : String) -> Boolean`（从在线玩家采集当前皮肤, 全字段快照注册, 其后换肤不影响） |
| playerNpcHasSkin | `(skinId : String) -> Boolean` |
| playerNpcUnregisterSkin | `(skinId : String) -> Boolean`（有 NPC 引用时拒绝） |
| playerNpcGetSkinIds | `() -> Array<String>` |
| playerNpcCreate | `(x, y, z : Float, dim : Integer, name : String, skinId : String) -> Integer`（失败 <0: -2 id 占用, -3 皮肤未注册） |
| playerNpcCreateRandom | 同 create 参数（随机段 ID） |
| playerNpcCreateWithId | `(x, y, z : Float, dim : Integer, name : String, skinId : String, desiredId : Integer) -> Integer`（持久化恢复用） |
| playerNpcDestroy | `(id : Integer) -> Boolean` |
| playerNpcDestroyAll | `() -> Null` |
| playerNpcExists | `(id : Integer) -> Boolean` |
| playerNpcIsIdUsed | `(id : Integer) -> Boolean` |
| playerNpcGetAllIds | `() -> Array<Integer>` |
| playerNpcSetPos | `(id : Integer, x, y, z : Float, dim : Integer) -> Boolean`（dim<0 仅改坐标） |
| playerNpcSetRotation | `(id : Integer, yaw : Float) -> Boolean` |
| playerNpcSetNametag | `(id : Integer, text : String) -> Boolean`（空串清除） |
| playerNpcSetSkin | `(id : Integer, skinId : String) -> Boolean`（换肤 = respawn） |
| playerNpcSetScale | `(id : Integer, scale : Float) -> Boolean`（模型缩放; 客户端硬限 0.0625~10, 碰撞箱等比; 变更经 tick 脏刷新合并为单次 respawn） |
| playerNpcSetViewDistance | `(id : Integer, dist : Float) -> Boolean`（<=0 无限制） |
| playerNpcSetEnabled | `(id : Integer, enabled : Boolean) -> Boolean` |
| playerNpcSetVisiblePlayers | `(id : Integer, players: Array<String>) -> Boolean`（空列表 = 全员可见） |
| playerNpcClearVisiblePlayers | `(id : Integer) -> Boolean` |
| playerNpcSetVisiblePlayer | `(id : Integer, playerName : String) -> Boolean` |
| playerNpcGetDebugInfo | `(id : Integer) -> String`（诊断探针） |
| playerNpcInjectSkin | `(viewerName : String, targetName : String, skinId : String) -> Boolean`（1.26.0; 把注册表的皮肤发给观看者 —— viewerName 空串 = 所有在线玩家**含被换肤者本人**（"自己看自己被换肤"），非空 = 只发给该玩家; target 必须在线且 skinId 已注册; **不自动重发**） |
| playerNpcInjectSkinAll | `(targetName : String, skinId : String) -> Boolean`（1.26.0; 便捷: 等价 viewerName 空串） |
| playerNpcGetSkinBlobB64 | `(skinId : String) -> String`（1.26.0; 全字段二进制快照的 **base64**（LSE 字符串编组对二进制不安全）; 空串 = 未注册） |
| playerNpcRegisterSkinFromBlobB64 | `(b64 : String) -> Boolean`（1.26.0; 与上一条配对 —— 消费方落盘后重启恢复, 源玩家不在线也能用） |

LSE 示例（采集玩家皮肤并创建 NPC）：

```js
const capture = ll.import("HologramLib", "playerNpcCaptureSkin");
const create  = ll.import("HologramLib", "playerNpcCreate");
const pos     = ll.import("HologramLib", "playerNpcSetPos");

// 1. 采集在线玩家 Steve 的当前皮肤（含几何/披风/动画, 快照注册为 "steve_skin"）
capture("steve_skin", "Steve");

// 2. 用采集皮肤创建假玩家（主世界出生点旁）
const npc = create(100.5, 64, 100.5, 0, "Steve的分身", "steve_skin");
// 移动 / 换肤 / 白名单
pos(npc, 105.5, 64, 100.5, -1);
```

PNG 注册示例（自定义皮肤 + slim 手臂）：

```js
const reg = ll.import("HologramLib", "playerNpcRegisterSkin");
reg("E:/server/skins/girl.png", "girl", "", "slim");
```

---

## 2.10 trade*（村民交易菜单，10 函数）

打开界面 + 摆出交易表。默认纯展示；`settleLocally` 打开纯协议真结算（域契约见 1.10）。

| 函数 | 签名 | 说明 |
|------|------|------|
| tradeOpen | `(playerName : String, tradeType : String, tier : Integer, experience : Integer, realTrades : Boolean, carrierIdentifier : String) -> Integer` | 打开菜单。`realTrades = false`（推荐）= 纯协议层展示；`true` = 真实交易表（玩家真的能成交、物品真的消耗）。`carrierIdentifier` 空 = 村民, 可填 `minecraft:wandering_trader` |
| tradeAddOffer | `(id : Integer, buyAType : String, buyACount : Integer, buyBType : String, buyBCount : Integer, sellType : String, sellCount : Integer, sellName : String, tier : Integer, locked : Boolean) -> Boolean` | 追加一条并**就地重发**（不重开界面）；`buyBType` 空 = 无第二付费项 |
| tradeSetTier | `(id : Integer, tier : Integer, experience : Integer) -> Boolean` | 改显示栏值 / 经验条（同样就地重发） |
| tradeSetSettle | `(id : Integer, on : Boolean) -> Boolean` | **1.24.0** 打开后随时切换纯协议真结算（只有纯协议层路径有效；真实交易表路径归 BDS 结算）。也可打开前用 `TradeMenuSpec::settleLocally` 定初值 |
| tradeIsSettle | `(id : Integer) -> Boolean` | 查询当前是否真结算 |
| tradePollSettlements | `() -> Array<String>` | **取走并清空**结算回传；每条 `player=Steve menuId=3 offer=1 ok=1 reason=` |
| tradeClose / tradeCloseAll / tradeIsOpen / tradeGetIds | — | 生命周期与查询 |

> **本域没有点击回调。** 要「点条目就有回调」的列表界面用 `container*`（点击 = 一次物品拾取，任何输入设备都发包）。交易结果只能通过 `tradePollSettlements` 取。

```js
const tradeOpen     = ll.import("HologramLib", "tradeOpen");
const tradeAddOffer = ll.import("HologramLib", "tradeAddOffer");
const id = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
tradeAddOffer(id, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§b钻石", 1, false);
```

## 2.11 container*（虚拟容器 / 列表界面，1.21.0；1.22.0 补 LSE）

| 函数 | 签名 | 说明 |
|------|------|------|
| containerOpen | `(playerName : String, title : String, rows : Integer) -> Integer` | 打开一个空容器（`rows` 3 = 27 格, 6 = 54 格）。打开有固有延迟（默认 7 tick）, 之后才能填格 |
| containerSetItem | `(id : Integer, slot : Integer, type : String, count : Integer, damage : Integer, name : String, loreCsv : String) -> Boolean` | 单格刷新（一条 `InventorySlot`, 无延迟无闪烁）；`type` 空 = 清空该槽；`loreCsv` 用 `\n` 或 `\|` 分行, 空 = 无描述行 |
| containerSetTitle | `(id : Integer, title : String) -> Boolean` | 就地换标题（重发方块实体 + ContainerOpen, 不重摆方块） |
| containerSetInteractive | `(id : Integer, on : Boolean) -> Boolean` | **可交互模式**: 打开后客户端在容器**内部**拖动/交换物品会被库接住并回成功（物品真的留在新格子）; 默认关（只回传点击, 物品被服务端打回） |
| containerClose / containerCloseAll / containerIsOpen / containerGetIds | — | 生命周期与查询 |
| containerPollClicks | `() -> Array<String>` | **取走并清空**待处理的点击/关闭；每条 `player=X menuId=N slot=S closed=0\|1` |
| containerClearClicks | `() -> Null` | 丢弃队列里全部待处理点击 |

```js
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
const id = containerOpen("Steve", "§8任务列表", 3);
containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取|§8第二行");
// 定时器里取点击（也可用 ll.on 之类的守护循环）:
for (const line of containerPollClicks()) { /* 切分 player= / menuId= / slot= / closed= */ }
```

点击语义：点击就是一次物品拾取 —— 任何输入设备都会发包（实测 27 格与大容器第二半区都正常）。
服务端并没有这个容器，所以物品不会被真的拿走，库只回传槽位号（`slot` = 条目下标，`closed=1` 时 `slot=-1`）。

## 2.12 npcDialog*（NPC 对话框，1.21.0；1.22.0 补 LSE；26.40.8 补头像 1 个 → 9 函数）

| 函数 | 签名 | 说明 |
|------|------|------|
| npcDialogOpen | `(playerName : String, npcName : String, sceneName : String, dialogue : String, buttonsSpec : String, carrierIdentifier : String) -> Integer` | 打开对话（同一玩家重复调用先关上一层）；按钮串格式见下 |
| npcDialogUpdate | `(id : Integer, dialogue : String, buttonsSpec : String) -> Boolean` | 就地换正文与按钮（npcName / sceneName / 载体类型保留不变）；**只适合界面仍开着时的微调**，见下方实测限制 |
| npcDialogSetAvatar | `(id : Integer, skinVariant : Integer, avatarViewSpec : String) -> Boolean` | **26.40.8** 换聊天框内的头像 / 模型：`skinVariant` = NPC 内置皮肤变体 `0..59`（`-1` = 默认，不加 `SkinId` 项）；`avatarViewSpec` = 在载体上叠一层 `IViewOverride`（spec 语法同 `viewEntity`，如 `name=§6酒保`）。就地更新并重开（**0 闪烁**路径）；`false` = 该对话已不在 |
| npcDialogClose / npcDialogCloseAll / npcDialogIsOpen / npcDialogGetIds | — | 生命周期与查询 |
| npcDialogPollClicks | `() -> Array<String>` | 取走并清空；每条 `player=X dialogId=N scene=S button=I actionId=A closed=0\|1 commands=...` |
| npcDialogClearClicks | `() -> Null` | 丢弃队列 |

按钮串格式：按钮之间用 `;` 分隔（空段跳过），字段用 `|` 分隔 —— `label|actionId|mode|命令1,命令2`（后三项可省）。
`mode`：0=普通按钮 1=关闭 2=打开。

**实测限制**：客户端点任意按钮后界面会自行收起，而 `npcDialogUpdate` 只重发一次 `NpcDialoguePacket(Open)`,
客户端不会因此重新弹出 —— 换下一层对话必须重新 `npcDialogOpen`（内部删旧载体、建新载体、再发一次 Open）。

```js
const npcDialogOpen       = ll.import("HologramLib", "npcDialogOpen");
const npcDialogPollClicks = ll.import("HologramLib", "npcDialogPollClicks");
const id = npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？", "§a接受|main#0|0|say 你好;;§c离开|main#1|1|", "");
```

## 2.13 sensing*（感知域 / 客户端设备，1.21.0；1.22.0 补 LSE）

数据源是客户端每 tick 上报的 `PlayerAuthInput`（`InputMode`），库逐包捕获后在内存里记一份 —— 没有事件、不用轮询。

| 函数 | 签名 | 说明 |
|------|------|------|
| sensingDeviceOf | `(playerName : String) -> String` | `keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown` |
| sensingIsTouch | `(playerName : String) -> Boolean` | 是否触屏（最常用的那个） |
| sensingDeviceCode | `(playerName : String) -> Integer` | 0=Unknown 1=键鼠 2=触屏 3=手柄 4=体感（与 C++ 枚举同值） |

```js
const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
if (sensingIsTouch("Steve")) { /* 触屏: 用容器列表 */ } else { /* 键鼠: 用交易菜单 */ }
```

## 2.14 fakeInv*（背包虚容器，1.23.0）

| 函数 | 签名 | 说明 |
|------|------|------|
| fakeInvApply | `(playerName : String, itemsSpec : String, refreshIntervalTicks : Integer) -> Boolean` | 整份下发。`itemsSpec` 每条 `槽位\|物品id\|数量\|名字\|描述行~描述行`，条目之间用 `;` 或换行分隔；`refreshIntervalTicks` 0 = 关周期重发 |
| fakeInvSetSlot | `(playerName : String, slot : Integer, type : String, count : Integer, damage : Integer, name : String, loreCsv : String) -> Boolean` | 单格改动（`type` 空 = 清空该格） |
| fakeInvRefresh | `(playerName : String) -> Boolean` | 立即重发伪造内容（打开界面后 / 发现被覆盖时用） |
| fakeInvClear / fakeInvClearAll | `(playerName : String) -> Boolean` / `() -> Null` | 撤销伪造并恢复真实背包 |
| fakeInvIsActive / fakeInvGetPlayers | `(playerName : String) -> Boolean` / `() -> Array<String>` | 查询 |
| fakeInvPollClicks | `() -> Array<String>` | 取走并清空；每条 `player=X slot=S` |
| fakeInvClearClicks | `() -> Null` | 丢弃队列 |

```js
const fakeInvApply = ll.import("HologramLib", "fakeInvApply");
const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
// 快捷栏 0 号一把剑、1 号 64 个钻石；槽位 9 = 主背包第一格
fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§c虚容之剑|§7攻击力 +7;1|minecraft:diamond|64|§b虚容钻石", 20);
for (const line of fakeInvPollClicks()) { /* "player=Steve slot=0" */ }
```

## 2.15 sulfur*（硫磺立方体展示，1.23.0）

| 函数 | 签名 | 说明 |
|------|------|------|
| sulfurCreate | `(x : Float, y : Float, z : Float, dim : Integer, blockType : String, blockCount : Integer, blockDamage : Integer, blockName : String, archetype : String, invisible : Boolean) -> Integer` | 生成一只"吞着方块"的立方体; `archetype` 空串 = 不下发属性; `invisible` = 立方体本体是否隐身（默认 true → 只看到被吞的方块） |
| sulfurSetBlock | `(id : Integer, type : String, count : Integer, damage : Integer, name : String) -> Boolean` | 换吞下去的方块/物品（会重建一次实体） |
| sulfurSetArchetype | `(id : Integer, archetype : String) -> Boolean` | 换外观档位（空串 = 清掉属性; 不重建实体） |
| sulfurSetInvisible / sulfurSetScale | `(id : Integer, on : Boolean) / (id : Integer, scale : Float) -> Boolean` | 隐身 / 缩放 |
| sulfurDestroy / sulfurDestroyAll | — | 销毁（`sulfurDestroy` 单个 / `sulfurDestroyAll` 全部） |
| sulfurExists / sulfurGetIds / sulfurGetEntityId | — | 查询（`sulfurGetEntityId` 是背后的自定义实体 id） |

```js
const sulfurCreate      = ll.import("HologramLib", "sulfurCreate");
const sulfurSetArchetype = ll.import("HologramLib", "sulfurSetArchetype");
const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6书架", "regular");
sulfurSetArchetype(id, "sticky");   // 换外观档位（不重建实体）
```

## 2.16 view*（客户端视图覆盖，1.25.0；18 函数）

只拦不发: 库拦下**本来就该发**的包, 需要改内容时丢掉原包 + 手写协议包补发; 不改引擎对象、
不新增实体、不改服务端状态。数据只来自协议包本身（唯一例外: 玩家名字与快照, 见架构约定）。

| 函数 | 签名 | 说明 |
|------|------|------|
| viewEntity | `(playerName : String, entityUniqueId : Integer, spec : String) -> Boolean` | 按 spec 覆盖该玩家看到的这只实体; 玩家名空串 = 所有玩家; false = 该玩家不在线 / spec 没解析出内容 |
| viewBlock | `(playerName : String, x : Integer, y : Integer, z : Integer, type : String) -> Boolean` | 把 (x,y,z) 在该玩家客户端显示成 `type` 那种方块; type 空串 = 撤销; false = 玩家不在线 / 类型名无效 |
| viewClearEntity | `(playerName : String, entityUniqueId : Integer) -> Boolean` | 撤销单个实体 |
| viewClearBlock | `(playerName : String, x : Integer, y : Integer, z : Integer) -> Boolean` | 撤销单个方块（并把真实方块推回去） |
| viewClearAll | `(playerName : String) -> Null` | 撤销该玩家（空串 = 全部玩家）的全部覆盖 |
| viewDescribe | `(playerName : String) -> String` | 当前覆盖条数摘要, 如 `"entities=1 blocks=2"` |
| viewUniqueIdOf | `(playerName : String) -> Integer` | 按名字取该玩家实体的 uniqueId; **`-1` = 不在线**（脚本拿不到 id, 用这个） |
| viewEcho | `(text : String) -> String` | 原样回显（附长度与前 16 字节十六进制）; 排查"字符串到底传进来没有" |
| viewMetaProbe | `(spec : String) -> String` | 把 spec 折成的**元数据真序列化一遍**并回读成 items / bytes / hex。走的是与真正发包**完全同一条** `pushMetaItems`, 所以它绿 = 线上那一串 id/type 就是这个（**不需要客户端**就能验 spec） |
| viewItemSpawnHooked | `() -> Boolean` | **1.29.0**: 本库的视图覆盖是否连 `AddItemActor`(id 15) 一起拦。`true` = 掉落物换类型/隐藏一次到位; 老库没有这个名字（`ll.hasExported` 判 false）→ 脚本必须周期性重套覆盖 |

给"自己"套覆盖（写成所有玩家都看得见, 管理员自我伪装用）:

| 函数 | 签名 | 说明 |
|------|------|------|
| viewSelf | `(playerName : String, spec : String) -> Boolean` | 等价 `viewEntity("", viewUniqueIdOf(playerName), spec)` |
| viewSelfClear | `(playerName : String) -> Boolean` | 撤销自己的覆盖 |

**逐字段版**（一个字段一个参数, 不用拼 spec 串; 脚本侧推荐用这一组, 免去拼串与转义）:

| 函数 | 签名 | 说明 |
|------|------|------|
| viewSelfType | `(playerName : String, type : String) -> Boolean` | 自己变成该生物类型（玩家/生物都行） |
| viewSelfSkin | `(playerName : String, skinOfPlayer : String) -> Boolean` | 自己显示成另一名在线玩家的样子（借皮肤） |
| viewSelfName | `(playerName : String, nametag : String, alwaysShow : Boolean) -> Boolean` | 改自己头顶名字牌 |
| viewSelfHide | `(playerName : String, hidden : Boolean) -> Boolean` | 对所有人隐藏自己 |
| viewTargetType | `(entityUniqueId : Integer, type : String) -> Boolean` | 让**所有**玩家看到这只实体是该类型（不限于玩家实体; JS 侧拿不到别人 id 时的主力） |
| viewClearId | `(entityUniqueId : Integer) -> Boolean` | 撤销这只实体的覆盖（所有玩家） |

**uniqueId 是负数, 判在线只认 -1**:

> 实体 uniqueId 是 int64, 玩家与部分实体本身**就是负数**（实测某服玩家 = `-25769803775` = `-0x5FFFFFFFF`）。
> 所以判"在不在线"必须写 `viewUniqueIdOf(name) === -1`（`-1` 是库对外的"不在线"约定, 也是 BDS 的
> INVALID 常量, 真实在线实体不会是它）, **不要写 `< 0`** —— 1.25.0 首版内部就用 `-1` 当哨兵, 玩家明明
> 在线也直接返回 false, 而 C++ 结构体直传那条路不经过这个判断, 于是只有脚本侧翻车。
>
> LSE 的 `Entity` 对象上: `.id` 是**运行时 id**（不同东西）, 取唯一 id 要用 `.uniqueId`（**字符串**,
> 因为 int64 可能超出 JS 精确整数范围）, 用 `Number(e.uniqueId)` 转换。

`spec` 串（分号分隔，每项 `键=值`，键不分大小写；值两边的空白会被去掉）：

| 键 | 值 | 说明 |
|----|----|------|
| `type` | 实体类型名 | 换类型（`cow` 自动补成 `minecraft:cow`）; 对玩家实体 = 给他造代理生物（同一个 runtimeId/uniqueId） |
| `as` | 在线玩家名 | 换成那名玩家的样子（皮肤走原版皮肤更新那条路, 只对玩家实体有效） |
| `hidden` | `1` / `true` / `yes` / `on` | 对该玩家隐藏这只实体（阻断它的出生与更新包） |
| `name` | 名字牌文字 | 出现 `name` 才改名字牌；值给空串 = 清空名字 |
| `always` | `1` / `true` / `yes` / `on` | 名字牌一直显示（协议值本身是反的, 库内已处理） |
| `pose` | `0..13` | **1.29.0**: PoseIndex(79) —— 客户端**只对盔甲架与玩家**按它摆姿势; 越界自动钳进范围 |
| `scale` | `0.0625~10` | **1.29.0**: **元数据缩放**（客户端实体缩放 + 碰撞箱 53/54）; 越界整项忽略。⚠ 与 `skin=` 那档的"皮肤模型缩放"不是一回事 |
| `variant` | 整数 | **1.29.0**: `Variant`(2) —— 羊的颜色档 / 河豚的鼓胀档（0 常态 1 半鼓 2 全鼓）/ 热带鱼体型 0·1 … |
| `mark` | 整数 | **1.29.0**: `MarkVariant`(43) —— 热带鱼花纹 0..5、马的斑纹等 |
| `color` | `0..15` | **1.29.0**: `ColorIndex`(3) —— 主色（热带鱼等）; 越界整项忽略 |
| `color2` | `0..15` | **1.29.0**: `Color2Index`(82) —— 花纹色（热带鱼）; 越界整项忽略 |

> ⚠ **`mTick` 是硬性字段**（26.40 线格式）: `SetActorDataPacket` 与 `UpdateAttributesPacket`
> 最后都带一个 `mTick`。**不填（=0）客户端会把包当成过期包丢掉** —— 表现是"出生包里元数据都对、
> 画面却没变化"（2026-10-16 实机: 热带鱼变种/河豚鼓胀档位就是这样，属性二次确认同样受影响）。
> 视图覆盖域现在统一填 `currentViewTick()`（由 `tickPulse` 每拍同步）；customentity 域一直是这么发的
> （`CustomEntityManager`: `sadp.mTick = tick;`）—— 写新代码时别漏。

> 变种四项（`variant` / `mark` / `color` / `color2`）的**协议类型**取自引擎自己的真实出生包：
> `Variant`/`MarkVariant` 是 Int，`ColorIndex`/`Color2Index` 是 **Byte**（解码 `logs/packetdump.log` 里
> 一只真羊的 AddActor 得到：`id2(type2)` `id43(type2)` `id3(type0)` `id82(type0)`）。
> 用途示例: 掉落物实体化时让每条热带鱼掉落物长得不一样、河豚一律全鼓
> （`viewEntity("", uid, "type=minecraft:tropicalfish;variant=1;mark=3;color=9;color2=6")`）。

```js
const viewEntity  = ll.import("HologramLib", "viewEntity");
const viewBlock   = ll.import("HologramLib", "viewBlock");
const viewClearAll = ll.import("HologramLib", "viewClearAll");
const viewSelfType = ll.import("HologramLib", "viewSelfType");
const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");

// Steve 看到的这只僵尸变成一头牛, 名字牌"§c奶牛"并常显
viewEntity("Steve", 12345, "type=minecraft:cow;name=§c奶牛;always=1");
// 让 Steve 看不到它（服务端世界里它照常存在、照常打人）
viewEntity("Steve", 12345, "hidden=1");
// Steve 客户端的 (100,64,100) 显示成钻石块
viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
// 自己变成僵尸（所有人可见）, 再卸下
viewSelfType("Steve", "minecraft:zombie");
// 判在线: 只认 -1（uniqueId 本身是负数, 别写 < 0）
const id = viewUniqueIdOf("Alex");
if (id !== -1) viewEntity("", id, "type=minecraft:cow");
viewClearAll("Steve");   // 收工
```

---

## 已知问题

当前没有已知问题。

`ghost*` 的 2 个导出（`ghostPollInteractions` / `ghostClearInteractions`）**曾经**漏在注册流程之外
（`ModEntry::exportLseFunctions()` 没有调用 `GhostInteractRouter::exportLse()`，脚本侧 `ll.import` 得 `null`）。
现已接回：270 处 `exportAs` 在运行期全部注册。

> 本库维护者另有一份**由源码逐文件统计**的权威清单（含每个前缀的逐函数名、重名与注释检查），位于 `docs/internal/LSE导出清单.md` —— 该目录是本地文档，**不随库发布**。
> 发现本页与代码不一致时，以代码为准。
