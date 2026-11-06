# Actor 表现 · Endstone

对任意 runtimeId 的实体播动画/事件：受击、死亡、冻结、图腾，还有那个给朋友的彩蛋 `easterEggCreeper`（⑨），也能按玩家或按实体**拦截**指定事件（出站包过滤）。

彩蛋的来历见 [LSE 的「朋友的⑨」](../lse/easter-egg.md)：_昨日一去不回，噢耶耶，开心比什么都贵........_

动画名用原版 `animation.*` 控制器名；执行拦截要调一次 `ensure_packet_filter`（插件启用时已自动挂）。

## 快速开始

```python
rid = lib.actoreffect.runtime_id_of_entity(my_entity_id)
lib.actoreffect.play_hurt(rid)
lib.actoreffect.block_event(3)          # 全局拦掉某个事件
lib.actoreffect.unblock_event(3)
```

## 函数

### `block_clear`

`lib.actoreffect.block_clear()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorBlockClear`
- 示例：

    ```python
    lib.actoreffect.block_clear()
    ```

### `block_event`

`lib.actoreffect.block_event(event_id: int)`

- 参数：
  - `event_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorBlockEvent`
- 示例：

    ```python
    lib.actoreffect.block_event(1)
    ```

### `block_event_for`

`lib.actoreffect.block_event_for(player_name: str, event_id: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `event_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorBlockEventFor`
- 示例：

    ```python
    lib.actoreffect.block_event_for("Steve", 1)
    ```

### `block_event_for_entity`

`lib.actoreffect.block_event_for_entity(runtime_id: int, event_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `event_id` : `int`
- 返回值：按实体号过滤：登记成"该 runtimeId 的事件丢弃"，由 plugin 的出站过滤执行。
- 返回值类型：`bool`
- 对应 LSE 导出：`actorBlockEventForEntity`
- 示例：

    ```python
    lib.actoreffect.block_event_for_entity(1, 1)
    ```

### `block_stats`

`lib.actoreffect.block_stats()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`actorBlockStats`
- 示例：

    ```python
    lib.actoreffect.block_stats()
    ```

### `clear_interactions`

`lib.actoreffect.clear_interactions()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorClearInteractions`
- 示例：

    ```python
    lib.actoreffect.clear_interactions()
    ```

### `freeze`

`lib.actoreffect.freeze(runtime_id: int, strength: float)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `strength` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorFreeze`
- 示例：

    ```python
    lib.actoreffect.freeze(1, 1)
    ```

### `play_animation`

`lib.actoreffect.play_animation(runtime_id: int, animation: str, stop_expression: str = '', blend_out_time: float = 0.0)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `blend_out_time` : `float`（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayAnimation`
- 示例：

    ```python
    lib.actoreffect.play_animation(1, "...", "...", 1)
    ```

### `play_animation_full`

`lib.actoreffect.play_animation_full(runtime_id: int, animation: str, next_state: str = 'none', stop_expression: str = '', blend_out_time: float = 0.0, controller: str = '', molang_version: int = 0)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `next_state` : `str`（默认 `'none'`）
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `blend_out_time` : `float`（默认 `0.0`）
  - `controller` : `str`（默认 `''`）
  - `molang_version` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayAnimationFull`
- 示例：

    ```python
    lib.actoreffect.play_animation_full(1, "...", "...", "...", 1, "...", 1)
    ```

### `play_death`

`lib.actoreffect.play_death(runtime_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayDeath`
- 示例：

    ```python
    lib.actoreffect.play_death(1)
    ```

### `play_event`

`lib.actoreffect.play_event(runtime_id: int, event_id: int, data: int = 0)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `event_id` : `int`
  - `data` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayEvent`
- 示例：

    ```python
    lib.actoreffect.play_event(1, 1, 1)
    ```

### `play_event_for`

`lib.actoreffect.play_event_for(player_name: str, runtime_id: int, event_id: int, data: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `event_id` : `int`
  - `data` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayEventFor`
- 示例：

    ```python
    lib.actoreffect.play_event_for("Steve", 1, 1, 1)
    ```

### `play_hurt`

`lib.actoreffect.play_hurt(runtime_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayHurt`
- 示例：

    ```python
    lib.actoreffect.play_hurt(1)
    ```

### `play_instant_death`

`lib.actoreffect.play_instant_death(runtime_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayInstantDeath`
- 示例：

    ```python
    lib.actoreffect.play_instant_death(1)
    ```

### `play_totem`

`lib.actoreffect.play_totem(runtime_id: int, hold_ticks: int = 40)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `hold_ticks` : `int`（默认 `40`）
- 返回值：图腾激活动画（ActorEvent 35）：不换副手（endstone 无手持槽 API），只播事件。
- 返回值类型：`bool`
- 对应 LSE 导出：`actorPlayTotem`
- 示例：

    ```python
    lib.actoreffect.play_totem(1, 1)
    ```

### `poll_interactions`

`lib.actoreffect.poll_interactions()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`actorPollInteractions`
- 示例：

    ```python
    lib.actoreffect.poll_interactions()
    ```

### `runtime_id_of_entity`

`lib.actoreffect.runtime_id_of_entity(lib_id: int)`

- 参数：
  - `lib_id` : `int`
- 返回值：库内实体 id -> runtimeId（走自定义实体注册表）。
- 返回值类型：`int`
- 对应 LSE 导出：`actorRuntimeIdOfEntity`
- 示例：

    ```python
    lib.actoreffect.runtime_id_of_entity(1)
    ```

### `runtime_id_of_player`

`lib.actoreffect.runtime_id_of_player(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`actorRuntimeIdOfPlayer`
- 示例：

    ```python
    lib.actoreffect.runtime_id_of_player("Steve")
    ```

### `set_pose`

`lib.actoreffect.set_pose(runtime_id: int, pose_index: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `pose_index` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorSetPose`
- 示例：

    ```python
    lib.actoreffect.set_pose(1, 1)
    ```

### `unblock_event`

`lib.actoreffect.unblock_event(event_id: int)`

- 参数：
  - `event_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorUnblockEvent`
- 示例：

    ```python
    lib.actoreffect.unblock_event(1)
    ```

### `unblock_event_for`

`lib.actoreffect.unblock_event_for(player_name: str, event_id: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `event_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorUnblockEventFor`
- 示例：

    ```python
    lib.actoreffect.unblock_event_for("Steve", 1)
    ```

### `unblock_event_for_entity`

`lib.actoreffect.unblock_event_for_entity(runtime_id: int, event_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
  - `event_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorUnblockEventForEntity`
- 示例：

    ```python
    lib.actoreffect.unblock_event_for_entity(1, 1)
    ```

### `unfreeze`

`lib.actoreffect.unfreeze(runtime_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`actorUnfreeze`
- 示例：

    ```python
    lib.actoreffect.unfreeze(1)
    ```

### `creeper_display`

`lib.actoreffect.creeper_display(runtime_id: int)`

- 参数：
  - `runtime_id` : `int`<br>实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）
- 返回值：苦力怕膨胀彩蛋：AnimateEntity + LL 同款 molang 表达式。
- 返回值类型：`bool`
- 对应 LSE 导出：`easterEggCreeper`
- 示例：

    ```python
    lib.actoreffect.creeper_display(1)
    ```

### `easter_egg`

`lib.itemdisplay.easter_egg(display_id: int, duration_ticks: int = 60, rotate: bool = True)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `60`）
  - `rotate` : `bool`（默认 `True`）
- 返回值：彩蛋「朋友的⑨」的展示版：到点自己停，自动还原。
- 返回值类型：`bool`
- 对应 LSE 导出：`easterEggCreeperDisplay`
- 示例：

    ```python
    lib.itemdisplay.easter_egg(1, 1, "...")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
