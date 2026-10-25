# 客户端附加 · LSE

三个 26.51.6 新增的小域，都是"给某个玩家客户端加一点东西"，没有服务端状态之外的东西：

- `itemActor*`（8 个）：**协议层掉落物** —— 客户端凭空出现一只掉落物实体，服务端没有它
- `form*`（2 个）：**强制关掉客户端表单**
- `locatorBar*`（4 个）：**方位条航点**


## 掉落物（itemActor*）

服务端没有这只实体：不参与拾取判定、不进存档、没有物理。运动由客户端算 —— 生成时给初速度，
之后随时用 `setMotion` 改速度与方向。

```js
const itemActorSpawn     = ll.import("HologramLib", "itemActorSpawn");
const itemActorSetMotion = ll.import("HologramLib", "itemActorSetMotion");
const itemActorTake      = ll.import("HologramLib", "itemActorTake");

// 在 (100.5, 65, 100.5) 抛一颗钻石出去（初速度向上 0.4）
const id = itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);

// 1 秒后让它往回飞 —— 中途改速度/方向就是这一句
setTimeout(() => itemActorSetMotion(id, -0.3, 0.2, 0), 1000);

// 玩家点了就播"飞过去"的动画再删掉（物品不会真的进背包）
itemActorTake(id, player.realName);
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `itemActorSpawn(itemId, aux, count, x, y, z, dim, vx, vy, vz)` | `Integer` | 只在**有玩家的那个维度**生成；返回库内 id，`-1` = 失败 |
| `itemActorSetMotion(id, vx, vy, vz)` | `Boolean` | 飞行途中改速度/方向；`0,0,0` = 停住 |
| `itemActorMoveTo(id, x, y, z)` | `Boolean` | 瞬移（不发速度） |
| `itemActorTake(id, playerName)` | `Boolean` | 只给这名玩家播拾取动画，然后对所有人删掉 |
| `itemActorDestroy(id)` | `Boolean` | 删掉 |
| `itemActorDestroyAll()` | `Null` | 全删 |
| `itemActorExists(id)` | `Boolean` | — |
| `itemActorGetIds()` | `Array<Integer>` | — |

> 边界：**不带 NBT**。地上一个普通钻石不需要自定义名，所以这条路径不碰物品描述符的 NBT 方言
> （BDS 那份描述符的构造是私有的，模组侧造不出来）。要"带名字的物品"请用 `itemDisplay*` 那条路。


## 强制关闭表单（form*）

库不生成表单 —— 表单还是 LSE / LeviLamina Form API / MeowMenu 那边的事。这里只提供"把它关掉"。

| 函数 | 返回 | 说明 |
|---|---|---|
| `formClose(playerName)` | `Boolean` | 关掉该玩家当前打开的表单；玩家不在线 = `false` |
| `formCloseAll()` | `Integer` | 发给所有在线玩家，返回发给了几个 |

```js
const formClose = ll.import("HologramLib", "formClose");
mc.listen("onLeft", (p) => formClose(p.realName));   // 掉线前顺手收一下
```


## 方位条（locatorBar*）

1.26 客户端在快捷栏下方有一条方位指示。每个玩家一组航点，`key` 自己起名，同一个 key 再 `set`
就是更新位置 —— 不需要保管 UUID（库按 key 稳定派生）。

| 函数 | 返回 | 说明 |
|---|---|---|
| `locatorBarSet(playerName, key, x, y, z, dim, color, visible)` | `Boolean` | 新增或更新；`color` = ARGB，`-1` = 客户端默认 |
| `locatorBarRemove(playerName, key)` | `Boolean` | 删除一个航点 |
| `locatorBarClear(playerName)` | `Integer` | 清全部，返回清掉了几个 |
| `locatorBarCount(playerName)` | `Integer` | 当前航点数 |
| `locatorBarHideFromOthers(playerName, hidden)` | `Boolean` | 把该玩家从**其他人的客户端**上藏起来（本人照常看得到自己） |
| `locatorBarIsHiddenFromOthers(playerName)` | `Boolean` | 查询隐藏状态 |
| `locatorBarHideFromOthers(playerName, hidden)` | `Boolean` | 把该玩家从**其他人的客户端**上藏起来（本人照常看得到自己） |
| `locatorBarIsHiddenFromOthers(playerName)` | `Boolean` | 查询隐藏状态 |

```js
const locatorBarSet  = ll.import("HologramLib", "locatorBarSet");
const locatorBarClear = ll.import("HologramLib", "locatorBarClear");

// 给 Steve 指一个点（每 tick 刷新一次就是"跟着某物走"）
mc.listen("onTick", () => {
  locatorBarSet("Steve", "boss", bossX, bossY, bossZ, 0, 0xFF0000, true);
});

locatorBarClear("Steve");
```

> 边界：图标 / 贴图要不要资源包配合、只给位置能不能显示，**都还没实测**。
> 位置与可见性是必填的，先按这两项验。

### 把某个玩家从其他人眼里藏起来

`locatorBarHideFromOthers(playerName, true)` 之后，**其他人的客户端不再收到这名玩家的出生包、移动、
状态、皮肤与 Tab 条目**；他本人一切照常（看得到自己、照常能动）。

```js
const locatorBarHideFromOthers = ll.import("HologramLib", "locatorBarHideFromOthers");
locatorBarHideFromOthers("Steve", true);    // 隐身
locatorBarHideFromOthers("Steve", false);   // 恢复，不用重进
```

实现走视图覆盖域那条通道：`overrideEntity("", uniqueId, hidden)` 是"所有人的覆盖"，
而视图覆盖域本身有一条规则是**本人不参与自己实体的覆盖** —— 所以正好等于"别人看不见、自己看得见"。

### 把某个玩家从其他人眼里藏起来

`locatorBarHideFromOthers(playerName, true)` 之后，**其他人的客户端不再收到这名玩家的出生包、移动、
状态、皮肤与 Tab 条目**；他本人一切照常（还能看到自己、还能动）。

```js
const locatorBarHideFromOthers = ll.import("HologramLib", "locatorBarHideFromOthers");
locatorBarHideFromOthers("Steve", true);    // 隐身
locatorBarHideFromOthers("Steve", false);   // 恢复（不用重进）
```

实现走的是视图覆盖域那条已经验证过的隐藏通道：`overrideEntity("", uniqueId, hidden)` ——
"所有人的覆盖"，而视图覆盖域本身有一条规则是**本人不参与自己实体的覆盖**，所以正好等于"别人看不见、自己看得见"。
