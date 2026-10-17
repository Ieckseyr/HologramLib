# 视图覆盖与感知 · 让某个玩家看到不一样的东西 / 判断他用什么设备

`view*` 只改**客户端视图**：某个玩家（或所有玩家）客户端上的某只实体、某个方块换个样子，
而服务端世界、存档、碰撞、其他玩家看到的都是原样。`sensing*` 用来查玩家当前的输入设备
（键鼠 / 触屏 / 手柄 / 体感），最常见的用法是**按设备分流界面**：触屏给虚拟容器，键鼠给交易菜单。

覆盖只存在服务器的内存里：重启就没了，但按玩家名记的覆盖在该玩家重进后仍然有效。
`view*` 与 `sensing*` 都**没有事件、也没有轮询队列**，直接调用直接拿结果。

## 快速开始

```js
const viewEntity     = ll.import("HologramLib", "viewEntity");
const viewBlock      = ll.import("HologramLib", "viewBlock");
const viewSelfType   = ll.import("HologramLib", "viewSelfType");
const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");
const viewClearAll   = ll.import("HologramLib", "viewClearAll");

// Steve 看到的 12345 号实体变成一头牛，名字牌「§c奶牛」并一直显示
viewEntity("Steve", 12345, "type=minecraft:cow;name=§c奶牛;always=1");
// Steve 客户端的 (100,64,100) 显示成钻石块
viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
// 让所有人看 Steve 都是一只僵尸
viewSelfType("Steve", "minecraft:zombie");

// 拿别人的 uniqueId：判在线只认 -1（uniqueId 本身可能是负数，别写 < 0）
const uid = viewUniqueIdOf("Alex");
if (uid !== -1) viewEntity("", uid, "type=minecraft:cow");   // 所有人都看到 Alex 是一头牛

viewClearAll("");   // 收工：清掉所有玩家的覆盖
```

## spec 串

`viewEntity` / `viewSelf` 的 `spec` 用分号分隔，每项写成 `键=值`；键不分大小写，值两边的空白会被去掉。

| 键 | 值 | 说明 |
|---|---|---|
| `type=` | 实体类型名 | 换成该类型（`cow` 会自动补成 `minecraft:cow`）；对玩家实体 = 给他造一个同 id 的生物替身 |
| `as=` | 在线玩家名 | 换成那名玩家的样子（皮肤取自他）；只对玩家实体有效 |
| `skin=` | 已注册的皮肤 id | 换成注册表里的皮肤（由 `playerNpcRegisterSkin` / `playerNpcCaptureSkin` 注册）；只对非玩家实体有效，客户端会用玩家模型渲染它，服务端那边还是原来那只生物 |
| `hidden=` | `1` / `true` / `yes` / `on` | 对这名玩家隐藏这只实体 |
| `name=` | 名字牌文字 | 出现这个键就改名字牌；值给空串 = 清空名字 |
| `always=` | `1` / `true` / `yes` / `on` | 名字牌一直显示（要和 `name=` 一起用） |
| `pose=` | `0..13` | 姿态编号，越界会被钳进范围；客户端只对盔甲架与玩家按它摆姿势 |
| `scale=` | `0.0625~10` | 元数据缩放（客户端实体缩放 + 碰撞箱），越界整项忽略；与 `skin=` 那档的模型缩放不是一回事 |
| `variant=` | 整数 | 变种：羊的颜色档 / 河豚的鼓胀档（`0` 常态 `1` 半鼓 `2` 全鼓）/ 热带鱼体型… |
| `mark=` | 整数 | 二级变种（也可写 `markvariant=`）：热带鱼花纹 `0..5`、马的斑纹等 |
| `color=` | `0..15` | 主色（也可写 `colorindex=`），越界整项忽略 |
| `color2=` | `0..15` | 花纹色（热带鱼用），越界整项忽略 |

> `pose=` / `scale=` / `variant=` / `mark=` / `color=` / `color2=` 是**附加项**，要和主项
> （`type=` / `as=` / `skin=` / `hidden=` / `name=`）一起给：只给附加项的 spec 会被当成「没有任何生效项」，
> 效果等于把这条覆盖撤掉。

---

## 主入口

### 覆盖玩家看到的某只实体

`viewEntity(playerName, entityUniqueId, spec)`

- 参数：
  - `playerName` : `String`<br>玩家名；空串 = 对所有玩家生效
  - `entityUniqueId` : `Integer`<br>实体的唯一 id —— 玩家的用 `viewUniqueIdOf` 取，其它实体可取自 `Entity.uniqueId`（用 `Number(e.uniqueId)` 转成数字）
  - `spec` : `String`<br>覆盖内容，见上文「spec 串」：换类型、换皮肤、隐藏、改名字牌
- 返回值：`true` = 已登记；`false` = `spec` 里没有任何生效项（这次调用等于撤销覆盖，原本没有覆盖就是 `false`）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewEntity = ll.import("HologramLib", "viewEntity");
    viewEntity("Steve", 12345, "type=minecraft:cow;name=§c奶牛;always=1");
    viewEntity("Steve", 12345, "hidden=1");   // 只让 Steve 看不到它
    ```

### 覆盖玩家看到的某个方块

`viewBlock(playerName, x, y, z, type)`

- 参数：
  - `playerName` : `String`<br>玩家名；空串 = 对所有玩家生效
  - `x` : `Integer`<br>方块坐标 X（整数）
  - `y` : `Integer`<br>方块坐标 Y（整数）
  - `z` : `Integer`<br>方块坐标 Z（整数）
  - `type` : `String`<br>方块 id，如 `"minecraft:diamond_block"`；空串 = 撤销这个坐标的覆盖
- 返回值：`true` = 已登记；`false` = 类型名不在客户端注册表里（`type` 传空时，`false` = 这个坐标本来就没有覆盖）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewBlock = ll.import("HologramLib", "viewBlock");
    viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
    viewBlock("Steve", 100, 64, 100, "");   // 撤销这一格
    ```

### 撤销单只实体的覆盖

`viewClearEntity(playerName, entityUniqueId)`

- 参数：
  - `playerName` : `String`<br>玩家名；空串 = 对所有玩家生效
  - `entityUniqueId` : `Integer`<br>实体的唯一 id
- 返回值：`true` = 已撤销并恢复（这名玩家重新看到真身）；`false` = 没有这条覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewClearEntity = ll.import("HologramLib", "viewClearEntity");
    viewClearEntity("Steve", 12345);
    ```

### 撤销单个方块的覆盖

`viewClearBlock(playerName, x, y, z)`

- 参数：
  - `playerName` : `String`<br>玩家名；空串 = 对所有玩家生效
  - `x` : `Integer`<br>方块坐标 X（整数）
  - `y` : `Integer`<br>方块坐标 Y（整数）
  - `z` : `Integer`<br>方块坐标 Z（整数）
- 返回值：`true` = 已撤销并推回真实方块；`false` = 这个坐标本来就没有覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewClearBlock = ll.import("HologramLib", "viewClearBlock");
    viewClearBlock("Steve", 100, 64, 100);
    ```

### 撤销这名玩家的全部覆盖

`viewClearAll(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名；空串 = 全部玩家
- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const viewClearAll = ll.import("HologramLib", "viewClearAll");
    viewClearAll("Steve");   // 只清 Steve; 传 "" 就是全服清空
    ```

---

## 「自己」一族

这一组都作用在**调用者自己**身上，并且是给**所有玩家**看的（管理员自我伪装、活动变身这类用途）。

### 给自己套一层覆盖

`viewSelf(playerName, spec)`

- 参数：
  - `playerName` : `String`<br>玩家名，必须在线
  - `spec` : `String`<br>覆盖内容（同 `viewEntity`，见上文「spec 串」；等价于 `viewEntity("", viewUniqueIdOf(playerName), spec)`，登记后所有玩家都看得见）
- 返回值：`true` = 已登记；`false` = 玩家不在线，或 `spec` 里没有任何生效项
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelf = ll.import("HologramLib", "viewSelf");
    viewSelf("Steve", "type=minecraft:cow;scale=0.8");   // 所有人看 Steve 都是一头小牛
    ```

### 撤销套在自己身上的覆盖

`viewSelfClear(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` = 已撤销；`false` = 玩家不在线或本来就没有覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelfClear = ll.import("HologramLib", "viewSelfClear");
    viewSelfClear("Steve");
    ```

### 让自己在所有玩家眼里变身

`viewSelfType(playerName, type)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `type` : `String`<br>实体类型名，如 `"minecraft:zombie"`（玩家/生物类型都可以）
- 返回值：`true` = 已登记；`false` = 玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelfType = ll.import("HologramLib", "viewSelfType");
    viewSelfType("Steve", "minecraft:zombie");
    ```

### 借别人的皮肤显示自己

`viewSelfSkin(playerName, skinOfPlayer)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `skinOfPlayer` : `String`<br>皮肤来源的在线玩家名（换肤这一族是唯一连本人一起换的覆盖）
- 返回值：`true` = 已登记；`false` = 玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelfSkin = ll.import("HologramLib", "viewSelfSkin");
    viewSelfSkin("Steve", "Alex");   // Steve 变成 Alex 的样子
    ```

### 改自己头顶的名字牌

`viewSelfName(playerName, nametag, alwaysShow)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `nametag` : `String`<br>新名字（支持 `§` 颜色码）；空串 = 清空名字
  - `alwaysShow` : `Boolean`<br>`true` = 名字牌一直显示
- 返回值：`true` = 已登记（所有玩家都看得到）；`false` = 玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelfName = ll.import("HologramLib", "viewSelfName");
    viewSelfName("Steve", "§c队长", true);
    ```

### 对所有人隐藏自己

`viewSelfHide(playerName, hidden)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `hidden` : `Boolean`<br>`true` = 其他人都看不到你；`false` = 恢复可见（等于撤销这条覆盖）
- 返回值：`true` = 已登记；`false` = 玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewSelfHide = ll.import("HologramLib", "viewSelfHide");
    viewSelfHide("Steve", true);    // 潜行模式：别人看不到 Steve
    viewSelfHide("Steve", false);   // 现身
    ```

---

## 逐字段与按 id 操作

拿不到别人 uniqueId 的时候，用 `viewTargetType`；要一次清掉某只实体的全部覆盖，用 `viewClearId`。

### 让所有玩家看到它是该类型

`viewTargetType(entityUniqueId, type)`

- 参数：
  - `entityUniqueId` : `Integer`<br>实体的唯一 id
  - `type` : `String`<br>实体类型名，如 `"minecraft:cow"`（不限于玩家实体）
- 返回值：`true` = 已登记；`false` = `type` 传空并且没有可撤销的覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewTargetType = ll.import("HologramLib", "viewTargetType");
    viewTargetType(12345, "minecraft:sheep");   // 所有人都看到一只羊
    ```

### 撤销这只实体的全部覆盖

`viewClearId(entityUniqueId)`

- 参数：
  - `entityUniqueId` : `Integer`<br>实体的唯一 id
- 返回值：`true` = 已撤销；`false` = 没有这条覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewClearId = ll.import("HologramLib", "viewClearId");
    viewClearId(12345);
    ```

---

## 查询与诊断

### 取走「刚出生的实体」事件（1.30.0 / 26.51.4 起）

`viewPollSpawnEvents()`

- 参数：无
- 返回值：字符串数组，每条是一只要么刚出生的实体，读一次**清空**（取走语义，与 `ghost*` / `containerPollClicks` 一致）
  - 掉落物：`drop|uid|dim|x|y|z|`
  - 其它实体：`actor|uid|dim|x|y|z|identifier`
- 返回值类型：`Array<String>`
- 说明：出生包在**出站钩子里**就记一条（每只实体只记一次），所以脚本不需要再靠定时扫描（0.5~1s 一轮）去找"刚刷出来的东西"，
  最晚**一拍**就能拿到。典型用法：掉落物一出生就 `itemDisplayEmitForEntity(玩家, uid, ...)` 让这名玩家看到模型；生物一出生就套伪装规则。
  队列有上限（512 条），脚本停摆太久会丢掉最早的几条。
- 示例：

    ```js
    const pollSpawn = ll.import("HologramLib", "viewPollSpawnEvents");
    const emitOne   = ll.import("HologramLib", "itemDisplayEmitForEntity");

    function onTick() {
        for (const ev of pollSpawn()) {
            const [kind, uid, dim, x, y, z, identifier] = ev.split("|");
            if (kind === "drop") attach(Number(uid), "-0.4", "0", "0", "0.375", 180, 2, "", true);
        }
    }
    ```

### 按玩家名取他的 uniqueId

`viewUniqueIdOf(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名（脚本平时拿不到实体的这个 id，用它来取）
- 返回值：该玩家实体的 uniqueId；**`-1` = 不在线**（uniqueId 本身可能是负数，判断只认 `-1`，不要写 `< 0`）
- 返回值类型：`Integer`
- 示例：

    ```js
    const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");
    const uid = viewUniqueIdOf("Alex");
    if (uid !== -1) logger.info(`Alex 的 uniqueId = ${uid}`);
    ```

### 查这名玩家挂了多少条覆盖

`viewDescribe(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：以 `entities=N blocks=N` 开头的摘要（诊断用），后面还跟着若干钩子计数（排查覆盖有没有生效时看它）
- 返回值类型：`String`
- 示例：

    ```js
    const viewDescribe = ll.import("HologramLib", "viewDescribe");
    logger.info(viewDescribe("Steve"));
    ```

### 把字符串原样回显

`viewEcho(text)`

- 参数：
  - `text` : `String`<br>任意字符串（用来确认这一串到底有没有传进来）
- 返回值：形如 `len=12 hex=7479... text=[...]`；`hex` 是最多前 16 个字节
- 返回值类型：`String`
- 示例：

    ```js
    const viewEcho = ll.import("HologramLib", "viewEcho");
    logger.info(viewEcho("type=minecraft:cow"));   // 先确认这一串没被截断/串码
    ```

### 离线验 spec 折出的元数据

`viewMetaProbe(spec)`

- 参数：
  - `spec` : `String`<br>覆盖内容（同 `viewEntity`）；不需要客户端就能验它写对没有
- 返回值：形如 `items=N bytes=N hex=...`；`spec` 里没有任何生效项时是 `items=0 plan=empty`
- 返回值类型：`String`
- 示例：

    ```js
    const viewMetaProbe = ll.import("HologramLib", "viewMetaProbe");
    logger.info(viewMetaProbe("type=minecraft:sheep;color=5"));   // 看这一串到底发出了什么
    ```

### 查掉落物出生包有没有被拦

`viewItemSpawnHooked()`

- 返回值：本版本恒为 `true`（掉落物换类型 / 隐藏一次到位）；老库里没有这个函数名（`ll.hasExported` 判为 `false`）→ 视为不支持，脚本需要周期性重套覆盖
- 返回值类型：`Boolean`
- 示例：

    ```js
    const viewItemSpawnHooked = ll.import("HologramLib", "viewItemSpawnHooked");
    if (viewItemSpawnHooked()) logger.info("掉落物覆盖一次到位，不用周期重套");
    ```

---

## 感知（sensing*）

数据源是客户端每 tick 上报的输入方式，库记在内存里。**没有事件、不用轮询**，需要时直接查。

### 查玩家当前用什么设备

`sensingDeviceOf(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown`（玩家不在线或还没上报过输入时是 `unknown`）
- 返回值类型：`String`
- 示例：

    ```js
    const sensingDeviceOf = ll.import("HologramLib", "sensingDeviceOf");
    logger.info(`Steve 的设备：${sensingDeviceOf("Steve")}`);
    ```

### 查玩家是不是触屏

`sensingIsTouch(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` = 触屏；`false` = 不是触屏，或玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
    const containerOpen  = ll.import("HologramLib", "containerOpen");
    const tradeOpen      = ll.import("HologramLib", "tradeOpen");

    // 触屏用容器列表（点击必然发包），键鼠用交易菜单
    if (sensingIsTouch("Steve")) containerOpen("Steve", "§8任务列表", 3);
    else tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    ```

### 取设备编号的数字版本

`sensingDeviceCode(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`0` = Unknown `1` = 键鼠 `2` = 触屏 `3` = 手柄 `4` = 体感（方便存进计分板或配置）
- 返回值类型：`Integer`
- 示例：

    ```js
    const sensingDeviceCode = ll.import("HologramLib", "sensingDeviceCode");
    logger.info(`设备码：${sensingDeviceCode("Steve")}`);
    ```

---

## 注意事项

- **只改客户端视图**：服务端世界、存档、碰撞、其他玩家看到的都是原样。把某个玩家显示成一头牛时，
  他的 id 没变 —— 打他就是打他本人，伤害与掉落都按真身结算。
- **覆盖只存在内存里**：重启服务器就没了。按玩家名记的覆盖在该玩家重进后仍然有效
  （实体重新出生时自动套用，方块随区块送达自动补发）。
- **自己的客户端不参与自己实体的覆盖**：换类型、隐藏都是给别人看的（否则本人客户端会出问题）；
  只有换肤那一类（`viewSelfSkin` / `as=`）连本人一起换。
- **实体还没出生也能先设覆盖**，库会在它出生时套用；反过来，本来就不发给这名玩家的实体或方块
  （视野外等）不会因为覆盖而凭空出现。
- **撤销时离得太远（> 64 格）或已经消失的实体不会重建** —— 客户端上本来也没有它，等它下次出生即可。
- **逐玩家的发光 / 隐身做不了**：基岩版的隐身是效果而不是外观，这一版 API 里没有这两项。
  逐玩家的实体缩放要用 `scale=`（`0.0625~10`），不要和 `skin=` 那档的模型缩放混用。
- **`sensing*` 不用轮询**：换设备后下一个输入包就更新，玩家离线即清零。
- 界面类域（容器 / 对话 / 交易）的事件才需要轮询，见[界面与容器](interfaces.md)。

## 相关

- 域指南：[视图覆盖与感知](../domains/view-sensing.md)（能做什么 / 做不到什么、逐玩家隔离与恢复规则）
- 假的玩家实体：[假玩家 NPC](player-npc.md)（皮肤注册表就是 `skin=` 的来源）
- 完整的 C++ 侧说明：[C++ 接口参考](../reference/cpp-api.md)
