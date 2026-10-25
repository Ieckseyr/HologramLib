# 假玩家 NPC · LSE

`playerNpc*` 造出来的是**纯协议假玩家**：客户端看到一个完整的玩家模型，有名字牌、会出现在 Tab 列表里，但服务端**并没有这个玩家** —— 没有 AI、不占实体系统、不写存档。适合做商店老板、任务发布人、雕像、人形立牌这类「站在那里的角色」。

皮肤有四条来路：PNG 文件注册、从在线玩家采集、目录批量导入、base64 blob 恢复。**皮肤必须先注册、再创建 NPC**，否则创建会失败。库不落盘：重启后要自己重新注册皮肤、按自己的存档重建 NPC。

点击这个假玩家不进回调，要用 `ghost*` 轮询取（条目里 `domain=npc`）。出生事件同理，走 `playerNpcPollSpawns()`。


## 快速开始

```js
const registerSkin = ll.import("HologramLib", "playerNpcRegisterSkin");
const create       = ll.import("HologramLib", "playerNpcCreate");
const setNametag   = ll.import("HologramLib", "playerNpcSetNametag");

// 1. 注册一张 PNG 皮肤（skinId 自己起，之后都用它引用）
registerSkin("E:/server/skins/girl.png", "girl", "", "slim");

// 2. 用这张皮肤在主世界造一个假玩家
const npc = create(100.5, 64, 100.5, 0, "§e小美", "girl");

// 3. 改名字牌（支持 § 颜色码）
setNametag(npc, "§e小美 §7<店员>");
```

`npc` 就是这个假玩家的 id，之后所有 `playerNpc*` 函数都用它。

## 皮肤注册表

### 把 PNG 贴图注册成皮肤

`playerNpcRegisterSkin(pngPath, skinId, geometry, armSize)`

- 参数：
  - `pngPath` : `String`<br>PNG 文件路径（建议写绝对路径）。图片必须是**正方形**，边长是 2 的幂且不小于 64（64 / 128 / 256 / 512 / 1024）
  - `skinId` : `String`<br>给这张皮肤起的名字；空串 = 用文件名（不含扩展名）
  - `geometry` : `String`<br>几何模型名；空串 = 用标准玩家模型 `geometry.humanoid.custom`
  - `armSize` : `String`<br>手臂粗细，`"wide"`（粗，默认）或 `"slim"`（细）
- 返回值：`true` = 注册成功；文件打不开或尺寸不合规返回 `false`。同一个 `skinId` 重复注册会覆盖旧皮肤
- 返回值类型：`Boolean`
- 示例：

    ```js
    const reg = ll.import("HologramLib", "playerNpcRegisterSkin");
    reg("E:/server/skins/girl.png", "girl", "", "slim");   // slim = 细手臂
    reg("E:/server/skins/guard.png", "", "", "");          // skinId 留空 → 用文件名 "guard"
    ```

### 注册图片平面（128 PNG，透明可选）

`playerNpcRegisterImageSkin(pngPath, skinId, sizeBlocks, transparent)`

- 参数：
  - `pngPath` : `String`<br>128×128 PNG 路径
  - `skinId` : `String`<br>皮肤名；相同名字重复注册会覆盖
  - `sizeBlocks` : `Float`<br>平面边长（格）；`1` = 1×1 格（128 px/格），`1.25` = 1.25 格
  - `transparent` : `Boolean`<br>`true` 保留透明（alpha ≥ 128 不透明，其余挖空）；`false` 透明区域铺白
- 返回值：`true` = 注册成功
- 返回值类型：`Boolean`
- 说明：库内生成双面平面几何，几何 identifier 按尺寸唯一；拼接/摆放由调用方自己算
- 示例：

    ```js
    const registerImage = ll.import("HologramLib", "playerNpcRegisterImageSkin");
    registerImage("E:/server/images/logo.png", "logo", 1, true);
    const npc = create(100.5, 64, 100.5, 0, "§bLogo", "logo");
    ```

### 采集在线玩家的皮肤

`playerNpcCaptureSkin(skinId, playerName)`

- 参数：
  - `skinId` : `String`<br>注册名，不能为空
  - `playerName` : `String`<br>在线玩家的名字（`realName`）
- 返回值：采到的是**当时那一份完整快照**，这名玩家之后换肤不影响它；`skinId` 为空、玩家不在线返回 `false`。同一个 `skinId` 重复采集会覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const capture = ll.import("HologramLib", "playerNpcCaptureSkin");
    const create  = ll.import("HologramLib", "playerNpcCreate");
    capture("steve_skin", "Steve");
    const npc = create(100.5, 64, 100.5, 0, "Steve的分身", "steve_skin");
    ```

### 批量导入目录下的皮肤

`playerNpcImportSkins(dirPath)`

- 参数：
  - `dirPath` : `String`<br>目录路径。每个子文件夹的名字就是 `skinId`，文件夹里要有一个 `.png` 贴图（必需）和一个 `.json` 几何模型（可选，缺省用标准玩家模型）
- 返回值：成功导入的数量；`dirPath` 不是目录返回 `-1`；子文件夹里找不到 PNG 会被跳过、不计入数量
- 返回值类型：`Integer`
- 示例：

    ```js
    const importSkins = ll.import("HologramLib", "playerNpcImportSkins");
    // E:/server/skins/girl/girl.png → skinId 就是 "girl"
    const n = importSkins("E:/server/skins");
    logger.info(`导入了 ${n} 套皮肤`);
    ```

### 查询皮肤是否已注册

`playerNpcHasSkin(skinId)`

- 参数：
  - `skinId` : `String`<br>皮肤名
- 返回值：`true` = 已注册
- 返回值类型：`Boolean`
- 示例：

    ```js
    const hasSkin = ll.import("HologramLib", "playerNpcHasSkin");
    if (!hasSkin("girl")) registerSkin("E:/server/skins/girl.png", "girl", "", "");
    ```

### 列出已注册的全部皮肤名

`playerNpcGetSkinIds()`

- 返回值：所有 `skinId`；一套都没有时是空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const getSkinIds = ll.import("HologramLib", "playerNpcGetSkinIds");
    logger.info(`已注册皮肤: ${getSkinIds().join(", ")}`);
    ```

### 注销一套已注册的皮肤

`playerNpcUnregisterSkin(skinId)`

- 参数：
  - `skinId` : `String`<br>要注销的皮肤名
- 返回值：`true` = 已注销；**还有 NPC 在用这套皮肤时会被拒绝**（返回 `false`，先销毁那些 NPC 或给它们换肤），皮肤本来就不存在也返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const unregisterSkin = ll.import("HologramLib", "playerNpcUnregisterSkin");
    unregisterSkin("girl");   // 还有 NPC 用着 → false
    ```

### 把皮肤导出成 base64 字符串

`playerNpcGetSkinBlobB64(skinId)`

- 参数：
  - `skinId` : `String`<br>皮肤名
- 返回值：这套皮肤完整快照的 base64 文本（可以直接写进文件或数据库）；皮肤未注册返回空串 `""`
- 返回值类型：`String`
- 示例：

    ```js
    const getBlob = ll.import("HologramLib", "playerNpcGetSkinBlobB64");
    const b64 = getBlob("girl");   // 存进自己的配置文件；未注册时是 ""
    logger.info(`皮肤 blob 长度 ${b64.length}`);
    ```

### 从 base64 字符串恢复皮肤

`playerNpcRegisterSkinFromBlobB64(b64)`

- 参数：
  - `b64` : `String`<br>`playerNpcGetSkinBlobB64` 导出的字符串
- 返回值：`true` = 注册成功（皮肤名沿用导出时的那一个）；字符串解不开、为空或格式不合法返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const regBlob = ll.import("HologramLib", "playerNpcRegisterSkinFromBlobB64");
    // 开服时把上次存下来的字符串读回来注册
    const b64 = "……上次存的字符串……";
    if (!regBlob(b64)) logger.warn("皮肤恢复失败");
    ```

### 把皮肤发给在线玩家换肤

`playerNpcInjectSkin(viewerName, targetName, skinId)`

- 参数：
  - `viewerName` : `String`<br>只发给谁看；**空串 = 发给所有在线玩家，包含被换肤的本人**（这是「让玩家自己看到自己被换肤」的正路）
  - `targetName` : `String`<br>被换肤的在线玩家名字（`realName`）
  - `skinId` : `String`<br>已注册的皮肤名
- 返回值：`true` = 已发出；目标不在线、`skinId` 未注册、`viewerName` 指定的观看者不在线都返回 `false`。**它不会自动重发**：客户端偶尔丢一次的话，自己隔一会儿再调一次
- 返回值类型：`Boolean`
- 示例：

    ```js
    const inject = ll.import("HologramLib", "playerNpcInjectSkin");
    inject("", "Alex", "girl");        // 所有在线玩家（含 Alex 自己）看到 Alex 换成 girl 皮肤
    inject("Steve", "Alex", "girl");   // 只让 Steve 看到
    ```

### 发给所有在线玩家换肤

`playerNpcInjectSkinAll(targetName, skinId)`

- 参数：
  - `targetName` : `String`<br>被换肤的在线玩家名字
  - `skinId` : `String`<br>已注册的皮肤名
- 返回值：等价于 `viewerName` 传空串的 `playerNpcInjectSkin`，失败原因相同
- 返回值类型：`Boolean`
- 示例：

    ```js
    const injectAll = ll.import("HologramLib", "playerNpcInjectSkinAll");
    injectAll("Alex", "girl");
    ```

## 创建与删除

### 在坐标上创建假玩家

`playerNpcCreate(x, y, z, dim, name, skinId)`

- 参数：
  - `x` : `Float`<br>世界坐标 X
  - `y` : `Float`<br>世界坐标 Y
  - `z` : `Float`<br>世界坐标 Z
  - `dim` : `Integer`<br>维度 id，`0` / `1` / `2`
  - `name` : `String`<br>显示名，同时也是名字牌与 Tab 列表里的名字；空串 = 用默认名 `NPC`
  - `skinId` : `String`<br>已注册的皮肤名；空串 = 用默认的 `default`（没注册过就会失败）
- 返回值：假玩家 id；失败返回负数（`-3` = 皮肤未注册）
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "playerNpcCreate");
    const npc = create(100.5, 64, 100.5, 0, "§e小美", "girl");
    if (npc < 0) logger.warn("创建失败，皮肤注册了吗？");
    ```

### 用随机 id 创建假玩家

`playerNpcCreateRandom(x, y, z, dim, name, skinId)`

- 参数：同 `playerNpcCreate`
- 返回值：随机段（`0x10000000` 起）的假玩家 id，不会和自增 id 冲突；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const createRandom = ll.import("HologramLib", "playerNpcCreateRandom");
    const npc = createRandom(100.5, 64, 100.5, 0, "§e小美", "girl");
    ```

### 指定 id 创建以恢复 NPC

`playerNpcCreateWithId(x, y, z, dim, name, skinId, desiredId)`

- 参数：
  - 前 6 个参数：同 `playerNpcCreate`
  - `desiredId` : `Integer`<br>想用的 id，必须大于 `0` 且当前没被占用
- 返回值：成功就是 `desiredId`；`desiredId <= 0` 或已被占用返回 `-2`；皮肤未注册返回 `-3`
- 返回值类型：`Integer`
- 示例：

    ```js
    const createWithId = ll.import("HologramLib", "playerNpcCreateWithId");
    const npc = createWithId(100.5, 64, 100.5, 0, "§e小美", "girl", 7);   // 还原成上次的 id 7
    ```

### 删掉一个假玩家

`playerNpcDestroy(id)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
- 返回值：`true` = 已删除（已经看到它的玩家会看到它消失）；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroy = ll.import("HologramLib", "playerNpcDestroy");
    destroy(npc);
    ```

### 删掉全部假玩家

`playerNpcDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const destroyAll = ll.import("HologramLib", "playerNpcDestroyAll");
    destroyAll();   // 清场
    ```

## 属性

### 移动假玩家并可换维度

`playerNpcSetPos(id, x, y, z, dim)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `x` : `Float`<br>新的世界坐标 X
  - `y` : `Float`<br>新的世界坐标 Y
  - `z` : `Float`<br>新的世界坐标 Z
  - `dim` : `Integer`<br>目标维度；填负数 = 只改坐标、保持原维度
- 返回值：`true` = 已改；id 不存在返回 `false`。变更会合并到 tick 末尾一次性重发，不会反复闪
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setPos = ll.import("HologramLib", "playerNpcSetPos");
    setPos(npc, 105.5, 64, 100.5, -1);   // dim = -1：只挪坐标，维度不变
    setPos(npc, 105.5, 64, 100.5, 1);    // 换到下界
    ```

### 设置假玩家的朝向

`playerNpcSetRotation(id, yaw)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `yaw` : `Float`<br>朝向角，单位是度
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setRotation = ll.import("HologramLib", "playerNpcSetRotation");
    setRotation(npc, 90);   // 面朝 +X 方向
    ```

### 改假玩家头顶的名字牌

`playerNpcSetNametag(id, text)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `text` : `String`<br>名字牌文字，支持 `§` 颜色码；空串 = 清除名字牌。重复设同一个文本不会触发重发
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setNametag = ll.import("HologramLib", "playerNpcSetNametag");
    setNametag(npc, "§e小美 §7<店员>");
    setNametag(npc, "");   // 清掉名字牌
    ```

### 给假玩家换一套皮肤

`playerNpcSetSkin(id, skinId)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `skinId` : `String`<br>已注册的皮肤名
- 返回值：`true` = 已换；皮肤未注册或 id 不存在返回 `false`。换肤会让已经看到它的玩家看到它重建一次
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setSkin = ll.import("HologramLib", "playerNpcSetSkin");
    setSkin(npc, "guard");   // 换成卫兵皮肤
    ```

### 缩放模型并同步碰撞箱

`playerNpcSetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `scale` : `Float`<br>缩放倍数，客户端硬限是 `0.0625 ~ 10`
- 返回值：`true` = 已设；小于 `0.0625` 或大于 `10`、id 不存在都返回 `false`。变更会合并到 tick 末尾一次性重发
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setScale = ll.import("HologramLib", "playerNpcSetScale");
    setScale(npc, 1.5);     // 放大一半
    setScale(npc, 0.5);     // 变成半身大小
    ```

### 设置假玩家的可见距离

`playerNpcSetViewDistance(id, dist)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `dist` : `Float`<br>距离，单位方块；`<= 0` = 不限距离（默认是 96 格）
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setView = ll.import("HologramLib", "playerNpcSetViewDistance");
    setView(npc, 32);   // 32 格内可见
    setView(npc, 0);    // 不限距离
    ```

### 打开或关闭这个假玩家

`playerNpcSetEnabled(id, enabled)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `enabled` : `Boolean`<br>`false` = 玩家看不到它（id 仍然有效，随时可以开回来），`true` = 恢复显示
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setEnabled = ll.import("HologramLib", "playerNpcSetEnabled");
    setEnabled(npc, false);   // 先藏起来（比如打烊）
    setEnabled(npc, true);    // 再开门
    ```

## 可见玩家白名单

### 只让名单里的玩家看到

`playerNpcSetVisiblePlayers(id, players)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `players` : `Array<String>`<br>玩家名数组，按 `realName` 匹配；空数组 = 清除限制、全员可见
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setVisiblePlayers = ll.import("HologramLib", "playerNpcSetVisiblePlayers");
    setVisiblePlayers(npc, ["Steve", "Alex"]);   // 只给这两个人看
    setVisiblePlayers(npc, []);                  // 清空 = 全员可见
    ```

### 白名单只填一个玩家

`playerNpcSetVisiblePlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
  - `playerName` : `String`<br>唯一可见的玩家名（`realName`）
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setVisiblePlayer = ll.import("HologramLib", "playerNpcSetVisiblePlayer");
    setVisiblePlayer(npc, "Steve");   // 只有 Steve 能看到这个 NPC
    ```

### 清除白名单恢复全员可见

`playerNpcClearVisiblePlayers(id)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
- 返回值：`true` = 已清除（本来就没有限制也算成功）；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearVisible = ll.import("HologramLib", "playerNpcClearVisiblePlayers");
    clearVisible(npc);   // 所有人都能看到了
    ```

## 查询

### 查询假玩家是否还存在

`playerNpcExists(id)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
- 返回值：`true` = 存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const exists = ll.import("HologramLib", "playerNpcExists");
    if (!exists(npc)) logger.warn("NPC 已经被删掉了");
    ```

### 查询 id 是否已被占用

`playerNpcIsIdUsed(id)`

- 参数：
  - `id` : `Integer`<br>要查的 id
- 返回值：`true` = 已被占用（此时 `playerNpcCreateWithId` 会返回 `-2`）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const isUsed = ll.import("HologramLib", "playerNpcIsIdUsed");
    if (!isUsed(7)) createWithId(100.5, 64, 100.5, 0, "§e小美", "girl", 7);
    ```

### 列出所有假玩家的 id

`playerNpcGetAllIds()`

- 返回值：所有假玩家 id；一个都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const getAllIds = ll.import("HologramLib", "playerNpcGetAllIds");
    for (const id of getAllIds()) logger.info(`NPC ${id}`);
    ```

### 取运行态摘要用于排查

`playerNpcGetDebugInfo(id)`

- 参数：
  - `id` : `Integer`<br>假玩家 id
- 返回值：摘要文本，包含 id、名字、皮肤、坐标、维度、朝向、视距、开关、已见玩家数与可见名单状态；id 不存在返回 `not_found`
- 返回值类型：`String`
- 示例：

    ```js
    const getInfo = ll.import("HologramLib", "playerNpcGetDebugInfo");
    logger.info(getInfo(npc));
    ```

## 轻脏通道与逐玩家朝向

26.51.6 补的 8 个导出，之前只有 C++ 接口。

| 函数 | 作用 |
|---|---|
| `playerNpcSetPosLight(id, x, y, z, dim)` | 只发 `MoveActorAbsolute`，不重建实体、不重发皮肤。跨维度返回 `false`（跨维度要用 `playerNpcSetPos`） |
| `playerNpcSetRotLight(id, yaw)` | 同上，只改朝向 |
| `playerNpcSetPlayerRotation(id, playerName, yaw)` | 指定玩家看到的朝向（"看向某人"） |
| `playerNpcClearPlayerRotation(id, playerName)` | 清某个玩家的朝向覆盖 |
| `playerNpcClearPlayerRotations(id)` | 清全部朝向覆盖 |
| `playerNpcPlayAnimationTo(id, playerName, animation, stopExpression, durationTicks)` | 只给一个玩家播 |
| `playerNpcPlayEmote(id, pieceId, lengthTicks)` | 原版表情动作（`EmotePacket`）；`0` = 客户端时长 |
| `playerNpcPollSpawns()` | 取走出生事件，每条 `player=<名字> id=<npcId>` |
| `playerNpcClearSpawns()` | 丢弃出生事件队列 |

同一条轻脏通道内的多次调用会在同一 tick 合并成一条包，所以每 tick 喂坐标是安全的：

```js
const playerNpcSetPosLight = ll.import("HologramLib", "playerNpcSetPosLight");
mc.listen("onTick", () => {
  // 让 NPC 跟着某只怪走：只发移动包，不重建实体，客户端不闪
  playerNpcSetPosLight(npcId, x, y, z, 0);
});
```

## 注意事项

- **皮肤先注册再建人**：`skinId` 没注册过，创建会失败（`-3`）。运行中皮肤被注销或缺失，这个 NPC 会一直不生成，脚本侧不会有报错。
- **库不落盘**：皮肤和 NPC 都只在内存里。服务端或脚本重启后要重新注册皮肤（或用 `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` 把皮肤存盘，重启后恢复），NPC 要按自己的存档记录重建（`playerNpcCreateWithId` 能还原原来的 id）。
- **假玩家会常驻 Tab 列表**：库不下发 PlayerList 移除条目（客户端在皮肤条目还活着时移除会崩），所以只要玩家看到过这个 NPC，它的名字就会一直留在 Tab 列表里，直到 NPC 被销毁。
- **每个 tick 最多只生成 2 个 NPC**（多个同时生成会让客户端一次收到大量数据而断开），所以可见性最多晚一个 tick；视距进出带 4 格滞回，避免在边界上反复重发皮肤。
- **属性变更会合并**：同一 tick 里连改位置 / 朝向 / 缩放 / 皮肤，只会重建一次（不闪）。但每次重建都会把皮肤重发一遍，所以高频跟踪式移动不适合放在脚本 tick 里 —— LSE 侧没有轻量位置通道，这类需求在 C++ 侧用 `setPositionLight`。
- **点击要轮询**：假玩家的交互不进回调，用 `ghostPollInteractions()` 取，条目格式形如 `player=Steve action=2 domain=npc id=7`（`action=1` 右键交互，`action=2` 攻击）。
- **部分能力没有脚本接口**：`setRotationLight` / `setPositionLight` / `playAnimation` / `playAnimationTo` / `setEntitySpawnCallback` 目前只有 C++ 接口，脚本里 `ll.import` 会拿到 `null`；可以先 `ll.hasExported("HologramLib", "...")` 判断再决定要不要用。
- **白名单按名字匹配**：用的是玩家名（LSE 的 `realName`），不是 uuid；玩家改名后旧名单会失效。
