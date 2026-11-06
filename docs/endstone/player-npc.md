# 玩家 NPC · Endstone

纯协议假玩家：客户端看到一个完整玩家模型，有名字牌、进 Tab 列表，但服务端没有这个人。适合商店老板、任务发布人、雕像。

**皮肤必须先注册再创建**（endstone 拿不到 `Image`，只能从 PNG 解），外观改动（皮肤/缩放/名字）走「拆了重建」，会重发 PlayerList + AddPlayer。

## 快速开始

```python
lib.playernpc.register_skin("E:/server/skins/girl.png", "girl", "", 0)
nid = lib.playernpc.create(0, 64, 0, 0, "§e小美", "girl")
lib.playernpc.set_nametag(nid, "§e小美 §7<店员>")
lib.playernpc.play_animation(nid, "animation.player.attack.positions", "", 20)
lib.playernpc.set_ride_entity(nid, some_entity_id)
```

## 函数

### `capture_skin`

`lib.playernpc.capture_skin(skin_id: str, player_name: str)`

- 参数：
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：把玩家皮肤存成 skinId。endstone 不暴露 Image，只能复用已注册的同名皮肤。
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcCaptureSkin`
- 示例：

    ```python
    lib.playernpc.capture_skin("...", "Steve")
    ```

### `clear_player_rotation`

`lib.playernpc.clear_player_rotation(npc_id: int, player_name: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcClearPlayerRotation`
- 示例：

    ```python
    lib.playernpc.clear_player_rotation(1, "Steve")
    ```

### `clear_player_rotations`

`lib.playernpc.clear_player_rotations(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcClearPlayerRotations`
- 示例：

    ```python
    lib.playernpc.clear_player_rotations(1)
    ```

### `clear_ride`

`lib.playernpc.clear_ride(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcClearRide`
- 示例：

    ```python
    lib.playernpc.clear_ride(1)
    ```

### `clear_spawns`

`lib.playernpc.clear_spawns()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcClearSpawns`
- 示例：

    ```python
    lib.playernpc.clear_spawns()
    ```

### `clear_visible_players`

`lib.playernpc.clear_visible_players(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcClearVisiblePlayers`
- 示例：

    ```python
    lib.playernpc.clear_visible_players(1)
    ```

### `create`

`lib.playernpc.create(x, y, z, dim, name, skin_id)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `name` : `String`<br>名字 / 标识符
  - `skin_id` : `String`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcCreate`
- 示例：

    ```python
    lib.playernpc.create(0.0, 64.0, 0.0, 0, "hello", "...")
    ```

### `create_ex`

`lib.playernpc.create_ex(x, y, z, dim, name, skin_id, yaw = 0.0, scale = 1.0)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `name` : `String`<br>名字 / 标识符
  - `skin_id` : `String`<br>皮肤 id（注册时起的名字）
  - `yaw` : `Float`<br>水平朝向（度）（默认 `0.0`）
  - `scale` : `Float`<br>缩放倍率，1.0 = 原始大小（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcCreateEx`
- 示例：

    ```python
    lib.playernpc.create_ex(0.0, 64.0, 0.0, 0, "hello", "...", "...", "...")
    ```

### `create_random`

`lib.playernpc.create_random(x, y, z, dim, name, skin_id)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `name` : `String`<br>名字 / 标识符
  - `skin_id` : `String`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcCreateRandom`
- 示例：

    ```python
    lib.playernpc.create_random(0.0, 64.0, 0.0, 0, "hello", "...")
    ```

### `create_with_id`

`lib.playernpc.create_with_id(x, y, z, dim, name, skin_id, desired_id)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `name` : `String`<br>名字 / 标识符
  - `skin_id` : `String`<br>皮肤 id（注册时起的名字）
  - `desired_id` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcCreateWithId`
- 示例：

    ```python
    lib.playernpc.create_with_id(0.0, 64.0, 0.0, 0, "hello", "...", "...")
    ```

### `destroy`

`lib.playernpc.destroy(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcDestroy`
- 示例：

    ```python
    lib.playernpc.destroy(1)
    ```

### `destroy_all`

`lib.playernpc.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcDestroyAll`
- 示例：

    ```python
    lib.playernpc.destroy_all()
    ```

### `exists`

`lib.playernpc.exists(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcExists`
- 示例：

    ```python
    lib.playernpc.exists(1)
    ```

### `get_all_ids`

`lib.playernpc.get_all_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`playerNpcGetAllIds`
- 示例：

    ```python
    lib.playernpc.get_all_ids()
    ```

### `get_debug_info`

`lib.playernpc.get_debug_info(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`playerNpcGetDebugInfo`
- 示例：

    ```python
    lib.playernpc.get_debug_info(1)
    ```

### `get_skin_blob_b64`

`lib.playernpc.get_skin_blob_b64(skin_id: str)`

- 参数：
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`playerNpcGetSkinBlobB64`
- 示例：

    ```python
    lib.playernpc.get_skin_blob_b64("...")
    ```

### `get_skin_ids`

`lib.playernpc.get_skin_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`playerNpcGetSkinIds`
- 示例：

    ```python
    lib.playernpc.get_skin_ids()
    ```

### `has_skin`

`lib.playernpc.has_skin(skin_id: str)`

- 参数：
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcHasSkin`
- 示例：

    ```python
    lib.playernpc.has_skin("...")
    ```

### `import_skins`

`lib.playernpc.import_skins(dir_path: str)`

- 参数：
  - `dir_path` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcImportSkins`
- 示例：

    ```python
    lib.playernpc.import_skins("...")
    ```

### `inject_skin`

`lib.playernpc.inject_skin(viewer_name: str, target_name: str, skin_id: str)`

- 参数：
  - `viewer_name` : `str`
  - `target_name` : `str`
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcInjectSkin`
- 示例：

    ```python
    lib.playernpc.inject_skin("...", "...", "...")
    ```

### `inject_skin_all`

`lib.playernpc.inject_skin_all(target_name: str, skin_id: str)`

- 参数：
  - `target_name` : `str`
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcInjectSkinAll`
- 示例：

    ```python
    lib.playernpc.inject_skin_all("...", "...")
    ```

### `is_id_used`

`lib.playernpc.is_id_used(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcIsIdUsed`
- 示例：

    ```python
    lib.playernpc.is_id_used(1)
    ```

### `play_animation`

`lib.playernpc.play_animation(npc_id: int, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcPlayAnimation`
- 示例：

    ```python
    lib.playernpc.play_animation(1, "...", "...", 1)
    ```

### `play_animation_to`

`lib.playernpc.play_animation_to(npc_id: int, player_name: str, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcPlayAnimationTo`
- 示例：

    ```python
    lib.playernpc.play_animation_to(1, "Steve", "...", "...", 1)
    ```

### `play_emote`

`lib.playernpc.play_emote(npc_id: int, piece_id: str, length_ticks: int = 0)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `piece_id` : `str`
  - `length_ticks` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcPlayEmote`
- 示例：

    ```python
    lib.playernpc.play_emote(1, "...", 1)
    ```

### `poll_spawns`

`lib.playernpc.poll_spawns()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`playerNpcPollSpawns`
- 示例：

    ```python
    lib.playernpc.poll_spawns()
    ```

### `register_image_skin`

`lib.playernpc.register_image_skin(png_path: str, skin_id: str, size_blocks: float = 1.0, transparent: bool = False)`

- 参数：
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `size_blocks` : `float`（默认 `1.0`）
  - `transparent` : `bool`<br>是否保留透明（默认 `False`）
- 返回值：图像皮肤：按普通皮肤解码 PNG，记录 sizeBlocks（贴图缩放由客户端几何决定）。
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcRegisterImageSkin`
- 示例：

    ```python
    lib.playernpc.register_image_skin("...", "...", 1, "...")
    ```

### `register_image_skin_sized`

`lib.playernpc.register_image_skin_sized(png_path: str, skin_id: str, width_blocks: float, height_blocks: float, transparent: bool = False)`

- 参数：
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `width_blocks` : `float`
  - `height_blocks` : `float`
  - `transparent` : `bool`<br>是否保留透明（默认 `False`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcRegisterImageSkinSized`
- 示例：

    ```python
    lib.playernpc.register_image_skin_sized("...", "...", 1, 1, "...")
    ```

### `register_skin`

`lib.playernpc.register_skin(png_path: str, skin_id: str, geometry: str = '', arm_size: int = 0)`

- 参数：
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `geometry` : `str`<br>几何模型名；空串 = 标准玩家模型（默认 `''`）
  - `arm_size` : `int`<br>手臂粗细：0 粗（wide）/ 1 细（slim）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcRegisterSkin`
- 示例：

    ```python
    lib.playernpc.register_skin("...", "...", "...", 1)
    ```

### `register_skin_data`

`lib.playernpc.register_skin_data(png_path: str, skin_id: str, arm_size: int = 0, geometry: str = '')`

- 参数：
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `arm_size` : `int`<br>手臂粗细：0 粗（wide）/ 1 细（slim）（默认 `0`）
  - `geometry` : `str`<br>几何模型名；空串 = 标准玩家模型（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcRegisterSkinData`
- 示例：

    ```python
    lib.playernpc.register_skin_data("...", "...", 1, "...")
    ```

### `register_skin_data_opaque`

`lib.playernpc.register_skin_data_opaque(png_path: str, skin_id: str, arm_size: int = 0, geometry: str = '')`

- 参数：
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
  - `arm_size` : `int`<br>手臂粗细：0 粗（wide）/ 1 细（slim）（默认 `0`）
  - `geometry` : `str`<br>几何模型名；空串 = 标准玩家模型（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcRegisterSkinDataOpaque`
- 示例：

    ```python
    lib.playernpc.register_skin_data_opaque("...", "...", 1, "...")
    ```

### `register_skin_from_blob_b64`

`lib.playernpc.register_skin_from_blob_b64(blob: str)`

- 参数：
  - `blob` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`playerNpcRegisterSkinFromBlobB64`
- 示例：

    ```python
    lib.playernpc.register_skin_from_blob_b64("...")
    ```

### `register_skin_from_blob_file`

`lib.playernpc.register_skin_from_blob_file(path: str)`

- 参数：
  - `path` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`playerNpcRegisterSkinFromBlobFile`
- 示例：

    ```python
    lib.playernpc.register_skin_from_blob_file("...")
    ```

### `runtime_id`

`lib.playernpc.runtime_id(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`playerNpcRuntimeId`
- 示例：

    ```python
    lib.playernpc.runtime_id(1)
    ```

### `set_enabled`

`lib.playernpc.set_enabled(npc_id: int, enabled: bool)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetEnabled`
- 示例：

    ```python
    lib.playernpc.set_enabled(1, "...")
    ```

### `set_equipment`

`lib.playernpc.set_equipment(npc_id: int, slot: int, name: str, aux: int = 0, nbt: str = '')`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `slot` : `int`<br>槽位号
  - `name` : `str`<br>名字 / 标识符
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetEquipment`
- 示例：

    ```python
    lib.playernpc.set_equipment(1, 1, "hello", 1, "...")
    ```

### `set_image_size`

`lib.playernpc.set_image_size(npc_id: int, png_path: str, width_blocks: float, height_blocks: float, transparent: bool = False)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `png_path` : `str`<br>PNG 文件路径（建议绝对路径）
  - `width_blocks` : `float`
  - `height_blocks` : `float`
  - `transparent` : `bool`<br>是否保留透明（默认 `False`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetImageSize`
- 示例：

    ```python
    lib.playernpc.set_image_size(1, "...", 1, 1, "...")
    ```

### `set_nametag`

`lib.playernpc.set_nametag(npc_id: int, text: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetNametag`
- 示例：

    ```python
    lib.playernpc.set_nametag(1, "hello")
    ```

### `set_player_rotation`

`lib.playernpc.set_player_rotation(npc_id: int, player_name: str, yaw: float)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `yaw` : `float`<br>水平朝向（度）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetPlayerRotation`
- 示例：

    ```python
    lib.playernpc.set_player_rotation(1, "Steve", 1)
    ```

### `set_pos`

`lib.playernpc.set_pos(npc_id: int, x, y, z, dim = None)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `None`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetPos`
- 示例：

    ```python
    lib.playernpc.set_pos(1, 0.0, 64.0, 0.0, 0)
    ```

### `set_pos_light`

`lib.playernpc.set_pos_light(npc_id: int, x, y, z, dim = None)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `None`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetPosLight`
- 示例：

    ```python
    lib.playernpc.set_pos_light(1, 0.0, 64.0, 0.0, 0)
    ```

### `set_pose`

`lib.playernpc.set_pose(npc_id: int, pose: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `pose` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetPose`
- 示例：

    ```python
    lib.playernpc.set_pose(1, 1)
    ```

### `set_ride_entity`

`lib.playernpc.set_ride_entity(npc_id: int, vehicle_entity_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `vehicle_entity_id` : `int`<br>载具的库内实体 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetRideEntity`
- 示例：

    ```python
    lib.playernpc.set_ride_entity(1, 1)
    ```

### `set_ride_player`

`lib.playernpc.set_ride_player(npc_id: int, player_name: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：骑在真实玩家身上需要玩家的 actor uniqueId，endstone 只给 uuid，暂不支持。
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetRidePlayer`
- 示例：

    ```python
    lib.playernpc.set_ride_player(1, "Steve")
    ```

### `set_ride_unique_id`

`lib.playernpc.set_ride_unique_id(npc_id: int, vehicle_unique_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `vehicle_unique_id` : `int`<br>载具的 uniqueId
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetRideUniqueId`
- 示例：

    ```python
    lib.playernpc.set_ride_unique_id(1, 1)
    ```

### `set_rot_light`

`lib.playernpc.set_rot_light(npc_id: int, yaw: float)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `yaw` : `float`<br>水平朝向（度）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetRotLight`
- 示例：

    ```python
    lib.playernpc.set_rot_light(1, 1)
    ```

### `set_rotation`

`lib.playernpc.set_rotation(npc_id: int, yaw: float)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `yaw` : `float`<br>水平朝向（度）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetRotation`
- 示例：

    ```python
    lib.playernpc.set_rotation(1, 1)
    ```

### `set_scale`

`lib.playernpc.set_scale(npc_id: int, scale: float)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetScale`
- 示例：

    ```python
    lib.playernpc.set_scale(1, 1)
    ```

### `set_skin`

`lib.playernpc.set_skin(npc_id: int, skin_id: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetSkin`
- 示例：

    ```python
    lib.playernpc.set_skin(1, "...")
    ```

### `set_view_distance`

`lib.playernpc.set_view_distance(npc_id: int, dist: float)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `dist` : `float`<br>距离（格）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetViewDistance`
- 示例：

    ```python
    lib.playernpc.set_view_distance(1, 1)
    ```

### `set_visible_player`

`lib.playernpc.set_visible_player(npc_id: int, player_name: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetVisiblePlayer`
- 示例：

    ```python
    lib.playernpc.set_visible_player(1, "Steve")
    ```

### `set_visible_players`

`lib.playernpc.set_visible_players(npc_id: int, names)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `names` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcSetVisiblePlayers`
- 示例：

    ```python
    lib.playernpc.set_visible_players(1, "...")
    ```

### `stop_animation`

`lib.playernpc.stop_animation(npc_id: int)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcStopAnimation`
- 示例：

    ```python
    lib.playernpc.stop_animation(1)
    ```

### `stop_animation_to`

`lib.playernpc.stop_animation_to(npc_id: int, player_name: str)`

- 参数：
  - `npc_id` : `int`<br>玩家 NPC id
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcStopAnimationTo`
- 示例：

    ```python
    lib.playernpc.stop_animation_to(1, "Steve")
    ```

### `unregister_skin`

`lib.playernpc.unregister_skin(skin_id: str)`

- 参数：
  - `skin_id` : `str`<br>皮肤 id（注册时起的名字）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerNpcUnregisterSkin`
- 示例：

    ```python
    lib.playernpc.unregister_skin("...")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
