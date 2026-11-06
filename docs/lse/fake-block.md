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

## 注意

- 假方块是**客户端本地**的：服务端世界、碰撞、掉落都不变，别的没收到这条包的玩家看到的还是真方块。
- 每次调用只发一个方块；要铺一片就循环调（或者自己按批合并）。
- 状态名/值写错会解析失败返回 `false`，不会发半成品包。
