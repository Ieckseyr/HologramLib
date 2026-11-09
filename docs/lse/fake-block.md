# 假方块（`blockFake*`）

只发给客户端、不改服务端世界的方块更新。用来做"看得见但踩不到"的装饰方块，支持完整方块状态。

## 坐标与状态串

状态串格式：

```text
minecraft:oak_stairs[waterlogged=true,upside_down_bit=false,weirdo_direction=2]
minecraft:weathered_cut_copper_stairs["upside_down_bit"=true,"weirdo_direction"=3,"waterlogged"=true]
minecraft:stone
```

- 键名用基岩版的方块状态名（`waterlogged` / `upside_down_bit` / `weirdo_direction` 等）；
- 值支持 `true`/`false`、整数、小数、带引号的字符串；
- 不写状态就是该方块的默认状态。

## 单个玩家

```js
blockFakeSet("Steve", 100, 64, 100, 'minecraft:weathered_cut_copper_stairs["upside_down_bit"=true,"weirdo_direction"=3,"waterlogged"=true]');
blockFakeClear("Steve", 100, 64, 100);   // 恢复该位置的真实方块
```

- `blockFakeSet(playerName, x, y, z, state)`：只发给这个玩家；状态串解析失败返回 `false`。
- `blockFakeClear(playerName, x, y, z)`：读玩家所在维度的真实方块，把这个位置的客户端方块改回去。
- 玩家重载区块后会自动恢复真实方块，不需要额外清理。

## 全维度广播

```js
blockFakeSetAll(100, 64, 100, 0, 'minecraft:oak_stairs[waterlogged=true]');   // 返回发送人数
blockFakeClearAll(100, 64, 100, 0);
```

## 解析网络 ID

```js
const id = blockFakeResolve('minecraft:oak_stairs[waterlogged=true]');   // 失败返回 -1
```

只做状态串 → 网络方块 ID 的解析，排错用。


## 批量假方块（`blockBatch*`）

一次调用把整批方块交给库，库按**子区块**合并成 `UpdateSubChunkBlocks`（包 172）发出，并按每 tick 预算分帧。
1,602 个方块只发几十条包，逐条 `blockFakeSet` 要 1,602 条。

### 数据格式

`packed` 是紧凑字符串：`x y z state;x y z state`（`state` 里不能出现 `;`）

```js
const blockBatchSet = ll.import("HologramLib", "blockBatchSet");
const id = blockBatchSet("Steve", [
    "100 64 100 minecraft:stone",
    "101 64 100 minecraft:glass",
].join(";"), false);
```

### 函数

| 导出 | 参数 | 返回 | 说明 |
|---|---|---|---|
| `blockBatchSet` | `playerName, packed, immediate=false` | `batchId`（-1 失败） | 只发给这个玩家 |
| `blockBatchSetAll` | `dimId, packed, immediate=false` | `batchId` | 发给该维度所有玩家 |
| `blockBatchSetNear` | `playerName, packed, radius, invisible, immediate=false` | `batchId` | 批量 + **靠近切换**（见下） |
| `blockBatchClear` | `playerName, positions` | `Boolean` | `positions` = `"x y z;x y z"`，逐条恢复真实方块 |
| `blockBatchStats` | — | `String` | `batch=… queue=… packets=… bytes=… nearSets=…` |
| `blockBatchProgress` | `batchId` | `Int` | 0~100，-1 = 没有这个批次 |
| `blockBatchSelfTest` | `packed` | `String` | 只解析/分组/编码，**不发包**，排错用 |

- `immediate=true` 绕过分帧一次发完（只建议小批量，大会卡客户端）。
- 每包最多 1024 条改动；每 tick 全局 8 条包预算。
- 老版本没有这些导出，`ll.import` 会拿到 `null` —— 先判空再调。

### 靠近切换成硫磺立方体（可穿过）

`blockBatchSetNear` 在批量下发的同时登记这批方块：

- 玩家进入 `radius` 格 → 该位置发 **air** + 原地生成一个**硫磺立方体**（主手物品 = 对应方块），
  视觉上"方块变成了怪"，玩家可以直接穿过去；
- 离开 `radius × 1.3`（滞回）→ 销毁立方体、重新发回假方块；
- `invisible = true` 时**立方体隐身、被吞的方块照常渲染** —— 观感是"方块还在原地，但能直接穿过去"；
  `invisible = false` 则连立方体一起看到（方块被吞在怪身上）；
- 检查每 4 tick 一次，`clear` / 掉线 / 关服会一并销毁立方体。

```js
// 一层可穿过的墙：靠近 5 格时方块变成（可见的）硫磺立方体
blockBatchSetNear("Steve", packed, 5, false, false);

// 只想"让路"、不想看到怪：立方体隐身
blockBatchSetNear("Steve", packed, 5, true, false);
```

## 用方块 NBT / SNBT 直接铺方块（1.37.0）

有时候调用方手里已经有方块 NBT（`bl.getNbt().toSNBT()`），不想自己拼状态串。这三个导出直接吃 SNBT：

### 解析 SNBT 得到 network id

`blockFakeResolveNbt(snbt)`

- 参数：
  - `snbt` : `String`<br>形如 `{"name":"minecraft:stone_stairs","states":{"weirdo_direction":2},"version":18168865}`
- 返回值：对应的 network id；解析失败 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const resolve = ll.import("HologramLib", "blockFakeResolveNbt");
    const id = resolve('{"name":"minecraft:stone_stairs","states":{"weirdo_direction":2},"version":18168865}');
    ```

### 按 NBT 放一个假方块

`blockFakeSetNbt(playerName, x, y, z, snbt)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `x` / `y` / `z` : `Integer`<br>方块坐标
  - `snbt` : `String`<br>方块 NBT（含 `states`）
- 返回值：`true` 已下发；NBT 解析失败或玩家不在线 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setNbt = ll.import("HologramLib", "blockFakeSetNbt");
    setNbt("Steve", 100, 64, 100, block.getNbt().toSNBT());
    ```

### 发方块实体内容（告示牌文字 / 旗帜图案 / 箱子）

`blockFakeSetBlockEntity(playerName, x, y, z, snbt)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `x` / `y` / `z` : `Integer`<br>方块坐标（会覆盖 NBT 里的 x/y/z）
  - `snbt` : `String`<br>`BlockEntity.getNbt().toSNBT()`，必须含 `id`（如 `"Sign"` / `"Banner"` / `"Chest"`）
- 返回值：`true` 已下发 `BlockActorData`（包 56）；无 `id` 或解析失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setBe = ll.import("HologramLib", "blockFakeSetBlockEntity");
    if (block.hasBlockEntity()) {
        setBe("Steve", 100, 64, 100, block.getBlockEntity().getNbt().toSNBT());
    }
    ```

!!! note "方块本体要先有"

    `BlockActorData` 只给已存在的方块挂数据。旗帜/告示牌要先 `blockFakeSetNbt`（或 `blockBatchSet`）把方块本体放上去，
    再发方块实体，否则客户端直接忽略。

## 注意

- 假方块是**客户端本地**的：服务端世界、碰撞、掉落都不变，别的没收到这条包的玩家看到的还是真方块。
- `blockFake*` 每次只发一个方块；铺一片用 `blockBatch*`（按子区块合并），别循环调 `blockFakeSet`。
- 状态名/值写错会解析失败返回 `false`，不会发半成品包。
