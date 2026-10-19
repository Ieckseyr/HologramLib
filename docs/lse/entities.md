# 实体与交互 · LSE

本页覆盖三组能力：

- `entity*`（37 个）：**协议层自定义实体** —— 用原版实体类型在客户端凭空造一只实体，适合做 NPC 壳、装饰生物、盔甲架布景。服务端没有这只实体：没有 AI、没有碰撞、不落存档，也不参与任何服务端逻辑。
- `ghost*`（2 个）：**交互事件轮询** —— 玩家点到上面这类协议实体时，库把点击路由成事件放进队列，脚本自己轮询取走。
- `particle*`（22 个）：**粒子形状** —— 用原版粒子摆出点、线、矩形边框、填充面、长方体框、六面、多面体，并支持平移、平滑移动、旋转、自旋、缩放、跟随玩家。

三组都只负责"画出来"和"接住点击"，业务逻辑（命令、权限、持久化）写在你的脚本里。

> 本节示例里，`id` 指**实体**创建函数返回的库内 id，`pid` 指**粒子**创建函数返回的形状 id。两者是两套互不相干的编号：实体 id 从 1 开始自增（也可指定或随机），粒子 id 从 1 开始自增。


## 快速开始

```js
const entityCreate          = ll.import("HologramLib", "entityCreate");
const entitySetNametag      = ll.import("HologramLib", "entitySetNametag");
const ghostPollInteractions = ll.import("HologramLib", "ghostPollInteractions");
const particleCreateBox     = ll.import("HologramLib", "particleCreateBox");

// 1. 在主世界 (100.5, 64, 100.5) 造一只盔甲架（dim：0=主世界 1=下界 2=末地）
const id = entityCreate("armor_stand", 100.5, 64, 100.5, 0);

// 2. 给它挂个名字牌（§ 颜色码直接写在文本里）
entitySetNametag(id, "§e守卫");

// 3. 每 200 毫秒取一次点击事件 —— 事件进队列，必须自己轮询
setInterval(() => {
  for (const line of ghostPollInteractions()) {
    logger.info(line);   // 例：player=Steve action=2 domain=entity id=1
  }
}, 200);

// 4. 在它身上套一个粒子方框（interval=2：每 2 tick 重发一次；lifetime=0：永久）
const pid = particleCreateBox("demo", 0, 100.5, 64.5, 100.5, 1.5, 1.5, 1.5, 1, "minecraft:endrod", 2, 0);
```

## 自定义实体 · 创建

这一组负责把实体造出来。造出来之后它会在**满足可见条件的玩家**客户端上出现：同维度、在视距内、`enabled` 为开、且在白名单内（没设白名单就是全员）。

### 创建一个自定义实体

`entityCreate(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体类型，如 `minecraft:cow`、`armor_stand`；写短名（`cow`）会自动补 `minecraft:` 前缀
  - `x` : `Float`<br>世界坐标 X（浮点，`.5` 落在方块中心）
  - `y` : `Float`<br>世界坐标 Y（浮点，`.5` 落在方块中心）
  - `z` : `Float`<br>世界坐标 Z（浮点，`.5` 落在方块中心）
  - `dim` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
- 返回值：库内实体 id（后续所有 `entity*` 都用它）；`identifier` 为空返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const entityCreate = ll.import("HologramLib", "entityCreate");
    const id = entityCreate("minecraft:cow", 100.5, 64, 100.5, 0);
    ```

### 创建时设好朝向缩放名字牌

`entityCreateAdvanced(identifier, x, y, z, dim, yaw, pitch, scale, nametag)`

- 参数：
  - `identifier` : `String`<br>实体类型，同 `entityCreate`
  - `x` : `Float`<br>世界坐标 X，同 `entityCreate`
  - `y` : `Float`<br>世界坐标 Y，同 `entityCreate`
  - `z` : `Float`<br>世界坐标 Z，同 `entityCreate`
  - `dim` : `Integer`<br>维度，同 `entityCreate`
  - `yaw` : `Float`<br>偏航角（度），水平朝向
  - `pitch` : `Float`<br>俯仰角（度），抬头/低头
  - `scale` : `Float`<br>整体缩放；发包时被钳制到 `0.0625 ~ 10`
  - `nametag` : `String`<br>名字牌文字，支持 `§` 颜色码；空串 = 不要名字牌
- 返回值：库内实体 id；`identifier` 为空返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const entityCreateAdvanced = ll.import("HologramLib", "entityCreateAdvanced");
    // 一只面向东边、放大 1.5 倍的僵尸，头顶写着"§c守卫"
    const id = entityCreateAdvanced("zombie", 100.5, 64, 100.5, 0, 90, 0, 1.5, "§c守卫");
    ```

### 用随机 id 创建实体

`entityCreateRandom(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体类型，同 `entityCreate`
  - `x` : `Float`<br>世界坐标 X，同 `entityCreate`
  - `y` : `Float`<br>世界坐标 Y，同 `entityCreate`
  - `z` : `Float`<br>世界坐标 Z，同 `entityCreate`
  - `dim` : `Integer`<br>维度，同 `entityCreate`
- 返回值：库内实体 id，取值在 `[0x10000000, 0x7FFFFFFF)` 区间（与自增段长期隔离，不会撞车）；取不到空闲 id 返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const entityCreateRandom = ll.import("HologramLib", "entityCreateRandom");
    const id = entityCreateRandom("armor_stand", 100.5, 64, 100.5, 0);
    ```

### 用指定 id 创建实体

`entityCreateWithId(identifier, x, y, z, dim, desiredId)`

- 参数：
  - `identifier` : `String`<br>实体类型，同 `entityCreate`
  - `x` : `Float`<br>世界坐标 X，同 `entityCreate`
  - `y` : `Float`<br>世界坐标 Y，同 `entityCreate`
  - `z` : `Float`<br>世界坐标 Z，同 `entityCreate`
  - `dim` : `Integer`<br>维度，同 `entityCreate`
  - `desiredId` : `Integer`<br>想占用的库内 id，必须 `> 0`
- 返回值：成功时就是 `desiredId`；`desiredId <= 0` 或这个 id 已被占用返回 `-2`
- 返回值类型：`Integer`
- 示例：

    ```js
    const entityCreateWithId = ll.import("HologramLib", "entityCreateWithId");
    const entityIsIdUsed = ll.import("HologramLib", "entityIsIdUsed");
    const wanted = 1001;                       // 你自己存盘记住的 id
    const id = entityIsIdUsed(wanted) ? -2 : entityCreateWithId("armor_stand", 100.5, 64, 100.5, 0, wanted);
    ```

## 自定义实体 · 属性

改属性全部走"下一个 tick 生效"：结构性改动（类型、姿势、装备、flags 等）合并成一次重建，坐标/朝向/名字牌/缩放/隐身走增量包 —— 两种情况都不会闪。

### 更换实体类型

`entitySetIdentifier(id, identifier)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `identifier` : `String`<br>新的实体类型；短名自动补 `minecraft:`；空串被拒绝
- 返回值：改成功 `true`；id 不存在或 `identifier` 为空 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetIdentifier = ll.import("HologramLib", "entitySetIdentifier");
    entitySetIdentifier(id, "minecraft:skeleton");   // 僵尸换成骷髅
    ```

### 移动实体到新坐标

`entitySetPosition(id, x, y, z, dim)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `x` : `Float`<br>新的世界坐标 X
  - `y` : `Float`<br>新的世界坐标 Y
  - `z` : `Float`<br>新的世界坐标 Z
  - `dim` : `Integer`<br>`>= 0` 时把实体放到该维度；`< 0` = 只改坐标、维度不动
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetPosition = ll.import("HologramLib", "entitySetPosition");
    entitySetPosition(id, 105.5, 64, 100.5, -1);   // 只挪位置，还在原维度
    entitySetPosition(id, 0.5, 70, 0.5, 1);        // 搬到下界
    ```

### 设置实体朝向

`entitySetRotation(id, yaw, pitch)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `yaw` : `Float`<br>偏航角（度），水平朝向
  - `pitch` : `Float`<br>俯仰角（度）
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetRotation = ll.import("HologramLib", "entitySetRotation");
    entitySetRotation(id, 180, 0);   // 转身面向南
    ```

### 设置头顶名字牌

`entitySetNametag(id, text)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `text` : `String`<br>名字牌文字，支持 `§` 颜色码；空串 = 清除名字牌
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetNametag = ll.import("HologramLib", "entitySetNametag");
    entitySetNametag(id, "§a商人 §7(右键)");
    entitySetNametag(id, "");                 // 清掉名字牌
    ```

### 设置实体缩放

`entitySetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `scale` : `Float`<br>目标缩放（模型与体积等比）；`<= 0` 直接拒绝；实际发包时被钳制到客户端有效域 `0.0625 ~ 10`
- 返回值：`true` 成功；`scale <= 0` 或 id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetScale = ll.import("HologramLib", "entitySetScale");
    entitySetScale(id, 2);      // 放大到 2 倍
    entitySetScale(id, 1);      // 恢复原大小
    ```

### 按倍数相对缩放

`entityScaleBy(id, factor)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `factor` : `Float`<br>倍数，`> 1` 放大、`0 < factor < 1` 缩小；结果自动钳制到 `0.0625 ~ 10`
- 返回值：`true` 成功；`factor <= 0` 或 id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityScaleBy = ll.import("HologramLib", "entityScaleBy");
    entityScaleBy(id, 1.2);   // 每次调用都变大一成
    entityScaleBy(id, 0.5);   // 缩到一半
    ```

### 设置主变种编号

`entitySetVariant(id, variant)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `variant` : `Integer`<br>变种编号（皮肤/亚种）；同一个数字在不同实体上含义不同（例如热带鱼的体型、河豚的鼓胀档），库不做校验
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetVariant = ll.import("HologramLib", "entitySetVariant");
    entitySetVariant(id, 1);
    ```

### 设置二级变种编号

`entitySetMarkVariant(id, markVariant)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `markVariant` : `Integer`<br>二级变种编号，含义由实体类型决定（例如热带鱼的花纹、马的斑纹）
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetMarkVariant = ll.import("HologramLib", "entitySetMarkVariant");
    entitySetMarkVariant(id, 3);
    ```

### 设置实体颜色索引

`entitySetColorIndex(id, colorIndex)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `colorIndex` : `Integer`<br>颜色编号，用于羊、项圈这类会染色的实体
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetColorIndex = ll.import("HologramLib", "entitySetColorIndex");
    entitySetColorIndex(id, 14);   // 红色系
    ```

### 覆写原始 flags 位掩码

`entitySetFlags(id, flags)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `flags` : `Integer`<br>位掩码原值（高级用法），如 `0x01` = 着火、`0x20` = 隐身；是整块覆盖，不会与旧值合并
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetFlags = ll.import("HologramLib", "entitySetFlags");
    entitySetFlags(id, 0x01);   // 让它着火（视觉）
    entitySetFlags(id, 0);      // 清掉全部 flags
    ```

### 切换实体隐身

`entitySetInvisible(id, on)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `on` : `Boolean`<br>`true` 隐身 / `false` 现身；只动隐身这一位；和当前值相同时也返回 `true`（重复调用无副作用）
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetInvisible = ll.import("HologramLib", "entitySetInvisible");
    entitySetInvisible(id, true);
    entitySetInvisible(id, false);   // 现身
    ```

### 设置实体姿势

`entitySetPose(id, pose)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `pose` : `Integer`<br>姿势编号（`PoseIndex`），只对按它摆姿的模型有效；`0..13`（`0`=站立、`1`=无底板、`2`=显示手臂，`3..13` 是坐姿/睡姿/跳舞等）；越界自动钳进这个范围
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetPose = ll.import("HologramLib", "entitySetPose");
    entitySetPose(id, 2);    // 盔甲架显示手臂
    entitySetPose(id, 0);    // 回到站立
    ```

### 设置装备槽里的物品

`entitySetEquipmentSlot(id, slot, name, aux, nbt)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `slot` : `Integer`<br>`0`=主手 `1`=副手 `2`=头 `3`=胸 `4`=腿 `5`=脚；其它值返回 `false`
  - `name` : `String`<br>物品 id，短名自动补 `minecraft:`；空串 = 清空该槽
  - `aux` : `Integer`<br>物品的数据值，具体取值随物品而定
  - `nbt` : `String`<br>额外数据，SNBT 文本；空串 = 不带附加数据
- 返回值：`true` 成功；`slot` 越界或 id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetEquipmentSlot = ll.import("HologramLib", "entitySetEquipmentSlot");
    entitySetEquipmentSlot(id, 0, "diamond_sword", 0, "");      // 主手一把钻石剑
    entitySetEquipmentSlot(id, 2, "golden_helmet", 0, "");      // 头上金头盔
    entitySetEquipmentSlot(id, 0, "", 0, "");                   // 清空主手
    ```

## 自定义实体 · 可见性

谁能看到这只实体，由**维度 + 视距 + 白名单 + 总开关**共同决定，四个条件是叠加的。

### 开关实体总可见性

`entitySetEnabled(id, enabled)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `enabled` : `Boolean`<br>`false` = 从所有客户端上撤掉，`true` = 重新发出去
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetEnabled = ll.import("HologramLib", "entitySetEnabled");
    entitySetEnabled(id, false);   // 全场隐藏
    entitySetEnabled(id, true);    // 恢复
    ```

### 设置逐玩家可见半径

`entitySetViewDistance(id, dist)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `dist` : `Float`<br>距离（方块）；`<= 0` = 不做距离限制。创建时的默认值是 64 格
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetViewDistance = ll.import("HologramLib", "entitySetViewDistance");
    entitySetViewDistance(id, 32);   // 32 格内才看得到
    entitySetViewDistance(id, 0);    // 不限距离
    ```

### 用白名单限定可见玩家

`entitySetVisiblePlayers(id, playerNames)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerNames` : `Array<String>`<br>玩家名数组（按 LSE 的 `player.realName` 匹配）；空数组 = 清除限制、恢复全员可见
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetVisiblePlayers = ll.import("HologramLib", "entitySetVisiblePlayers");
    entitySetVisiblePlayers(id, ["Steve", "Alex"]);   // 只给这两个人看
    entitySetVisiblePlayers(id, []);                  // 取消限制
    ```

### 只让一名玩家看到实体

`entitySetVisiblePlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>唯一的可见玩家名；**替换**整份名单，不是追加。想看多人请用 `entitySetVisiblePlayers`；脚本侧推荐用这个（不用拼数组）
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetVisiblePlayer = ll.import("HologramLib", "entitySetVisiblePlayer");
    entitySetVisiblePlayer(id, "Steve");
    ```

### 清除白名单恢复全员可见

`entityClearVisiblePlayers(id)`

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：`true` 成功（本来就没设白名单也返回 `true`）；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityClearVisiblePlayers = ll.import("HologramLib", "entityClearVisiblePlayers");
    entityClearVisiblePlayers(id);
    ```

## 自定义实体 · 骑乘与动画

### 让实体骑到玩家头上

`entitySetRidePlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>玩家名，必须在线；空串 = 清除骑乘链接
- 返回值：`true` 成功（本来就没有链接、空串清除也算 `true`）；id 不存在或玩家不在线 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetRidePlayer = ll.import("HologramLib", "entitySetRidePlayer");
    entitySetRidePlayer(id, "Steve");   // 挂到 Steve 头上
    entitySetRidePlayer(id, "");        // 下来
    ```

### 让实体骑到另一只实体上

`entitySetRideEntity(id, vehicleEntityId)`

- 参数：
  - `id` : `Integer`<br>实体 id（乘客）
  - `vehicleEntityId` : `Integer`<br>载具实体的库内 id；`0` = 清除链接
- 返回值：`true` 成功；载具不存在、载具就是自己、或 id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetRideEntity = ll.import("HologramLib", "entitySetRideEntity");
    const horse = ll.import("HologramLib", "entityCreate")("horse", 100.5, 64, 100.5, 0);
    entitySetRideEntity(id, horse);   // id 骑到 horse 身上
    entitySetRideEntity(id, 0);       // 解除
    ```

### 解除骑玩家与骑实体的链接

`entityClearRide(id)`

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：`true` 成功（本来就没链接也返回 `true`）；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityClearRide = ll.import("HologramLib", "entityClearRide");
    entityClearRide(id);
    ```

### 给已见过实体的玩家放动画

`entityPlayAnimation(id, animation, stopExpression, durationTicks)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `animation` : `String`<br>资源包里的动画标识符，如 `animation.humanoid.base_pose`；空串被拒绝
  - `stopExpression` : `String`<br>Molang 停止表达式，表达式为真时停止；空串 = 常驻不自动停
  - `durationTicks` : `Integer`<br>`> 0` 时到这么多 tick 后自动补发停止；`0` = 不限时
- 返回值：`true` 表示已排队发出（约 2 tick 后到客户端）；id 不存在、`animation` 为空、实体被 `entitySetEnabled` 关掉、或当前没有任何玩家见过这只实体时 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityPlayAnimation = ll.import("HologramLib", "entityPlayAnimation");
    entityPlayAnimation(id, "animation.humanoid.base_pose", "", 0);        // 常驻
    entityPlayAnimation(id, "animation.humanoid.attack", "", 20);          // 播 1 秒后自动停
    ```

## 自定义实体 · 逐客户端外观

同一个实体，让不同玩家看到不同的名字牌、缩放和装备。覆盖只在你指定的那个玩家客户端上生效，其他人仍然看到通用配置。

### 改单个玩家看到的名字牌

`entitySetPlayerNametag(id, playerName, text)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>观看者名字，必须在线且已经见过这只实体
  - `text` : `String`<br>只有这个玩家能看到的文字；空串 = 清除该玩家的覆盖，回到通用名字牌
- 返回值：`true` 成功；id 不存在、玩家不在线、或该玩家还没见过这只实体 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetPlayerNametag = ll.import("HologramLib", "entitySetPlayerNametag");
    entitySetPlayerNametag(id, "Steve", "§c只在 Steve 眼里是这样");
    entitySetPlayerNametag(id, "Steve", "");   // 还原
    ```

### 改单个玩家看到的缩放

`entitySetPlayerScale(id, playerName, scale)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>观看者名字，必须在线且已经见过这只实体
  - `scale` : `Float`<br>该玩家看到的缩放，钳制到 `0.0625 ~ 10`；`<= 0` = 清除覆盖、回到通用缩放
- 返回值：`true` 成功；id 不存在、玩家不在线、或该玩家还没见过这只实体 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetPlayerScale = ll.import("HologramLib", "entitySetPlayerScale");
    entitySetPlayerScale(id, "Steve", 3);    // Steve 眼里这只实体特别大
    entitySetPlayerScale(id, "Steve", 0);    // 还原
    ```

### 改单个玩家看到的装备

`entitySetPlayerEquipmentSlot(id, playerName, slot, name, aux, nbt)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>观看者名字，必须在线且已经见过这只实体
  - `slot` : `Integer`<br>`0`=主手 `1`=副手 `2`=头 `3`=胸 `4`=腿 `5`=脚；其它值返回 `false`
  - `name` : `String`<br>物品 id；空串 = 这个槽对该玩家回退到通用装备
  - `aux` : `Integer`<br>物品的数据值，具体取值随物品而定
  - `nbt` : `String`<br>额外数据，SNBT 文本；空串 = 不带附加数据
- 返回值：`true` 成功；`slot` 越界、id 不存在、玩家不在线、或该玩家还没见过这只实体 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entitySetPlayerEquipmentSlot = ll.import("HologramLib", "entitySetPlayerEquipmentSlot");
    entitySetPlayerEquipmentSlot(id, "Steve", 2, "diamond_helmet", 0, "");  // Steve 看到钻石头盔
    entitySetPlayerEquipmentSlot(id, "Steve", 2, "", 0, "");                // 还原该槽
    ```

### 清除单个玩家的外观覆盖

`entityClearPlayerAppearance(id, playerName)`

- 参数：
  - `id` : `Integer`<br>实体 id
  - `playerName` : `String`<br>观看者名字，必须在线
- 返回值：`true` 表示确实清掉了覆盖；该玩家本来就没有任何覆盖、id 不存在、或玩家不在线返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityClearPlayerAppearance = ll.import("HologramLib", "entityClearPlayerAppearance");
    entityClearPlayerAppearance(id, "Steve");   // Steve 眼中的它恢复成通用配置
    ```

## 自定义实体 · 查询与销毁

### 找最近的自定义实体

`entityFindNearest(x, y, z, dim, maxDist)`

- 参数：
  - `x` : `Float`<br>参考点世界坐标 X
  - `y` : `Float`<br>参考点世界坐标 Y
  - `z` : `Float`<br>参考点世界坐标 Z
  - `dim` : `Integer`<br>只在同一个维度的实体里找（维度不同的直接跳过）
  - `maxDist` : `Float`<br>最大距离（方块）；`<= 0` = 不限制
- 返回值：最近实体的库内 id；范围内没有实体返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const entityFindNearest = ll.import("HologramLib", "entityFindNearest");
    const pos = player.pos;
    const near = entityFindNearest(pos.x, pos.y, pos.z, 0, 8);   // 8 格内最近的自定义实体
    if (near !== -1) entitySetNametag(near, "§e你好");
    ```

### 读取实体运行态摘要

`entityGetInfo(id)`

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：摘要文字（诊断探针；id、维度、坐标、类型、开关、缩放、姿势、装备数、视距、白名单、已见玩家数、骑乘）；id 不存在返回 `"not_found"`
- 返回值类型：`String`
- 示例：

    ```js
    const entityGetInfo = ll.import("HologramLib", "entityGetInfo");
    logger.info(entityGetInfo(id));   // 例：id=1 dim=0 pos=(100.5,64.0,100.5) identifier='minecraft:cow' ...
    ```

### 判断实体是否存在

`entityExists(id)`

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：存在 `true`，不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityExists = ll.import("HologramLib", "entityExists");
    if (entityExists(id)) entitySetNametag(id, "§a还在");
    ```

### 判断 id 是否被占用

`entityIsIdUsed(id)`

与 `entityExists` 查的是同一份记录，名字更贴合"创建前先探一下"的用法。

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：已占用 `true`，空闲 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityIsIdUsed = ll.import("HologramLib", "entityIsIdUsed");
    if (!entityIsIdUsed(1001)) {
      // 1001 空闲，可以用 entityCreateWithId 占下来
    }
    ```

### 列出全部实体 id

`entityGetAllIds()`

- 返回值：id 数组；一个实体都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const entityGetAllIds = ll.import("HologramLib", "entityGetAllIds");
    for (const one of entityGetAllIds()) logger.info(entityGetInfo(one));
    ```

### 销毁一只实体

`entityDestroy(id)`

- 参数：
  - `id` : `Integer`<br>实体 id
- 返回值：确实销毁了 `true`；id 不存在 `false`。客户端上那一只也跟着消失
- 返回值类型：`Boolean`
- 示例：

    ```js
    const entityDestroy = ll.import("HologramLib", "entityDestroy");
    entityDestroy(id);
    ```

### 销毁全部自定义实体

`entityDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const entityDestroyAll = ll.import("HologramLib", "entityDestroyAll");
    entityDestroyAll();   // 关服或重载脚本时收尾
    ```

## 交互事件（ghost*）

`ghost*` 只有两个函数，但用法和其他域不一样，先看这三条：

1. **事件进队列，没有回调。** 玩家点击库内协议实体（自定义实体 / 物品悬浮 / 假玩家 NPC）时，库把这次交互放进一个待处理队列，你必须自己按 tick 或定时器调 `ghostPollInteractions()` 取走；不取就一直堆在队列里。
2. **事件来源于客户端交互包。** 库在收包路径上挂了一层钩子，把客户端发来的交互包按目标反查回**库内 id**，再格式化成字符串入队。只有点到**库内协议实体**的交互会进这个队列 —— 打到服务端真实实体（原版生物、玩家）的点击不会出现在这里。
3. **队列有上限。** 最多存 256 条，满了以后新事件会把最旧的挤掉；不轮询 = 丢事件。

### 取走并清空交互事件

`ghostPollInteractions()`

- 返回值：每条是一个事件字符串，队列空时是空数组
- 返回值类型：`Array<String>`

  格式：`player=<玩家名> action=<1..6> domain=<entity|itemDisplay|npc> id=<库内 id>`，客户端带了坐标时（`InteractUpdate` 等）末尾再追加 `pos=(x,y,z)`。

  `action` 取值：`1`=Interact（右键交互）`2`=Attack（左键攻击）`3`=StopRiding `4`=InteractUpdate `5`=NpcOpen `6`=OpenInventory。处理前先判断 `action`，不要假设只有 1 和 2。

  `domain` 说明点的是哪一类：`entity` = 自定义实体，`itemDisplay` = FMBE 物品悬浮，`npc` = 假玩家 NPC。
- 示例：

    ```js
    const ghostPollInteractions = ll.import("HologramLib", "ghostPollInteractions");
    setInterval(() => {
      for (const line of ghostPollInteractions()) {
        const m = line.match(/player=(\S+) action=(\d+) domain=(\S+) id=(-?\d+)/);
        if (!m) continue;
        const [, who, action, domain, targetId] = m;
        if (action === "1" && domain === "entity") logger.info(`${who} 右键了实体 ${targetId}`);
        if (action === "2" && domain === "entity") entityDestroy(Number(targetId));   // 左键 = 打掉
      }
    }, 100);
    ```

### 丢弃未处理的交互事件

`ghostClearInteractions()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const ghostClearInteractions = ll.import("HologramLib", "ghostClearInteractions");
    // 玩家重连或场景切换时清一次，避免处理到过期的点击
    ghostClearInteractions();
    ```

## 注意事项

1. **`ghost*` 必须轮询。** 交互事件不进回调，只在队列里；不调 `ghostPollInteractions()` 就永远收不到。队列上限 256 条，满了丢最旧的。
2. **只有点到库内协议实体的交互会入队。** 自定义实体、物品悬浮、假玩家 NPC 的点击会带 `domain` 与库内 `id`；打到服务端真实实体（原版生物、玩家）的点击不会出现在 `ghostPollInteractions()` 的返回值里。
3. **自定义实体不是"创建即出现"。** 谁能看到由库定期重算，所以新建的实体最迟约 1 秒内出现在客户端；改一次属性会顺带触发一次重算，通常更快。
4. **协议实体不参与服务端逻辑。** 它没有 AI、没有碰撞、不落存档：打它不会掉血，走过去不会撞到，重启后全部消失 —— 需要持久化就自己存 id，再用 `entityCreateWithId` 恢复。
5. **可见性是四个条件的交集。** 维度 + 视距 + 白名单 + `entitySetEnabled` 开关都满足的玩家才看得到；白名单不是"追加"，`entitySetVisiblePlayer` 会替换整份名单。
6. **名字牌默认只在准星对准时显示。** LSE 侧没有切换"常显"的函数；要和全息字一样始终可见，用 `holo*` 域另挂一行字。
7. **逐客户端朝向没有 LSE 导出。** "每个玩家看到不同朝向"（`setPlayerRotation` 一族）只有 C++ 接口，本页的逐客户端函数只覆盖名字牌、缩放和装备。
8. **`entityPlayAnimation` 只发一次包、不持久化。** 玩家后来才看到这只实体时不会自动补发动画；`entitySetEnabled` 关掉时调用会直接返回 `false`。
9. **粒子形状没有"销毁全部"和"列出全部 id"的导出。** 脚本要自己保存创建时返回的 id，收尾时逐个 `particleDestroy`。
10. **粒子白名单按 uuid，并且和维度叠加。** 白名单里一个有效玩家都不在形状所在维度时，这个形状会被自动移除（不会留着等人）；形状所在维度为空只是空转保留。
11. **粒子是瞬态视觉，靠重发维持。** `interval` 决定观感与发包量：间隔越大越省带宽、越容易看出"一批一批"的闪烁；`step` 越小粒子越密，面形状尤其要注意粒子数量。
12. **纯视觉、不改服务端状态。** 这些能力都只在客户端出现，服务端的真实世界、存档与逻辑完全不受影响。
