# 掉落物 · Endstone

协议层掉落物：客户端看到一个真实的物品实体，会飘、会被「捡走」，但服务端没有实体、不占实体系统。适合做奖励展示、商店陈列。

## 快速上手

```python
aid = lib.itemactor.spawn("minecraft:diamond", 1, 0, 65, 0, 0)
lib.itemactor.set_motion(aid, 0.0, 0.1, 0.0)
lib.itemactor.move_to(aid, 0, 64, 0, 0)
```

## 函数（9 个）

#### `destroy`

`lib.itemactor.destroy(actor_id: int)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
- 返回值类型：`bool`
- 对应 LSE： `itemActorDestroy`

#### `destroy_all`

`lib.itemactor.destroy_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `itemActorDestroyAll`

#### `exists`

`lib.itemactor.exists(actor_id: int)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
- 返回值类型：`bool`
- 对应 LSE： `itemActorExists`

#### `get_ids`

`lib.itemactor.get_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `itemActorGetIds`

#### `move_to`

`lib.itemactor.move_to(actor_id: int, x: float, y: float, z: float)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值类型：`bool`
- 对应 LSE： `itemActorMoveTo`

#### `runtime_id`

`lib.itemactor.runtime_id(actor_id: int)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
- 返回值类型：`int`
- 对应 LSE： `itemActorRuntimeId`

#### `set_motion`

`lib.itemactor.set_motion(actor_id: int, vx: float, vy: float, vz: float)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
  - `vx` : `float`
  - `vy` : `float`
  - `vz` : `float`
- 返回值类型：`bool`
- 对应 LSE： `itemActorSetMotion`

#### `spawn`

`lib.itemactor.spawn(item_id: str, aux: int, count: int, x: float, y: float, z: float, dim: int = 0, vx: float = 0.0, vy: float = 0.0, vz: float = 0.0)`

- 参数：
  - `item_id` : `str`
  - `aux` : `int`<br>物品数据值（damage / aux）
  - `count` : `int`<br>数量
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `0`）
  - `vx` : `float`（默认 `0.0`）
  - `vy` : `float`（默认 `0.0`）
  - `vz` : `float`（默认 `0.0`）
- 返回值类型：`int`
- 对应 LSE： `itemActorSpawn`

#### `take`

`lib.itemactor.take(actor_id: int, player_name: str)`

- 参数：
  - `actor_id` : `int`<br>掉落物 id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `itemActorTake`


---

完整索引见 [Endstone API 索引](api.md)。
