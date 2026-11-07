# 1.6 ICustomEntity · 自定义实体

用 `AddActorPacket` 直发客户端、凭空造一只**纯视觉实体**：不占服务端实体系统，
没有 AI、没有碰撞、不掉落、不落存档。适合做 NPC 壳、装饰生物、盔甲架布景、任务标记。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().customEntities()`
- 坐标：世界坐标，单位方块
- 维度：`0` 主世界 / `1` 下界 / `2` 末地
- 朝向：`yaw`（偏航）/ `pitch`（俯仰），单位**度**
- id：`int64_t`，属于自定义实体域，不能传给假玩家 / 物品展示 / 悬浮字
- 交互：点它产生的交互走 [GhostInteractEvent](ghost-interact.md)（`domain=entity`）

下面示例里的 `id` 都指本页创建接口返回的实体 id，`cfg` 指 `CustomEntityConfig`。

!!! info "谁能看到：四个条件的交集"

    维度相同 + 在 `viewDistance` 内 + 在白名单里（没设就是全员）+ `enabled` 为开，
    四个条件都满足的玩家才会收到这只实体。可见性由库内 tick hook 定期重算，
    所以新建的实体**最迟约 1 秒**才出现在客户端；改一次属性会顺带触发重算，通常更快。

!!! note "属性改动的两种通道"

    结构性改动（类型 / 姿势 / 装备 / `flags` / 变种）会合并成一次重建；
    坐标、朝向、名字牌、缩放、隐身走增量包。两者都在**下一 tick** 生效，过程不闪。

## 配置结构体（CustomEntityConfig）

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `identifier` | `std::string` | `"minecraft:armor_stand"` | 实体类型标识符；短名自动补 `minecraft:`；空串会让创建失败 |
| `x` / `y` / `z` | `float` | `0` / `64` / `0` | 世界坐标，单位方块 |
| `dimension` | `int` | `0` | 维度 id：`0` 主世界 / `1` 下界 / `2` 末地 |
| `yaw` / `pitch` | `float` | `0` / `0` | 朝向（度）；头与身体用同一个值 |
| `nametag` | `std::string` | 空 | 头顶名字，支持 `§` 颜色码；空串 = 没有名字牌 |
| `nametagAlwaysShow` | `bool` | `false` | `false` = 准星对准才显示；`true` = 始终显示 |
| `scale` | `float` | `1.0` | 实体缩放；发包时钳制到 `0.0625 ~ 10` |
| `variant` | `int` | `0` | 主变种编号（皮肤 / 亚种），含义由实体类型决定 |
| `markVariant` | `int` | `0` | 二级变种编号 |
| `colorIndex` | `int` | `0` | 颜色索引（羊、项圈这类染色实体） |
| `flags` | `std::int64_t` | `0` | 原始 flags 位掩码：`0x01` = 着火、`0x20` = 隐身 |
| `invisible` | `bool` | `false` | 隐身便捷开关；与 `flags` 独立，发包时合成 `0x20` |
| `viewDistance` | `double` | `64.0` | 可见距离，单位方块；`<= 0` = 不限距离 |
| `enabled` | `bool` | `true` | `false` = 从所有客户端撤掉（id 仍有效） |
| `pose` | `int` | `0` | 姿势编号 `0 ~ 13`：`0` 站立、`1` 无底板、`2` 显示手臂、`3 ~ 13` 坐 / 睡 / 跳舞等 |
| `equipment[6]` | `CustomEntityEquipment[6]` | 全空 | 六个装备槽，顺序见下 |
| `ridePlayerName` | `std::string` | 空 | 骑到指定玩家头上（`SetActorLinkPacket`，玩家须在线） |
| `rideEntityId` | `int64_t` | `0` | 骑到另一只自定义实体上（对方的库内 id） |
| `mobProperties` | `std::vector<EntityMobProperty>` | 空 | 按名同步的客户端属性（见 `setMobProperty`） |

装备槽位编码与 `MobEquipmentPacket` 一致：
`0` = 主手、`1` = 副手、`2` = 头、`3` = 胸、`4` = 腿、`5` = 脚。

`CustomEntityEquipment` 的字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `name` | `std::string` | 物品标识符；空串 = 空槽位 |
| `aux` | `int` | 物品附加值（data 值） |
| `nbt` | `std::string` | SNBT 附加数据；空串 = 无 |

`EntityMobProperty` 只有 `name`（完整属性名，如 `minecraft:sulfur_cube_archetype`）
与 `value`（字符串取值，如 `sticky`）两个字段。

!!! note "协议实体不参与服务端逻辑"

    打它不掉血、走过去不碰撞、关服全部消失。要持久化就自己存 id，重启后用
    `createWithId` 恢复；玩家重连由库自动补发，不需要消费方处理。

## 创建

### 创建一个自定义实体

`int64_t create(CustomEntityConfig const& config)`

- 参数：
  - `config` : `CustomEntityConfig const&`<br>完整实体配置，字段见上表；只填想改的字段即可
- 返回值：库内实体 id（`>= 1`）；`identifier` 为空或该域被 `HLIB_NO_ENTITY` 停用时返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();

    hologramlib::CustomEntityConfig cfg;
    cfg.identifier = "armor_stand";
    cfg.x          = 100.5f;
    cfg.y          = 64.0f;
    cfg.z          = 100.5f;
    cfg.dimension  = 0;
    cfg.nametag    = "§e守卫";

    int64_t id = entities.create(cfg);
    if (id < 0) {
        return; // 没建出来
    }
    ```

### 用随机 id 创建

`int64_t createRandom(CustomEntityConfig const& config)`

- 参数：
  - `config` : `CustomEntityConfig const&`<br>同 `create`
- 返回值：`0x10000000 ~ 0x7FFFFFFF` 随机段里的新 id（与自增段隔离）；连续 128 次都撞到已占用 id 时返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    int64_t id = entities.createRandom(cfg);
    ```

### 用指定 id 创建（持久化恢复用）

`int64_t createWithId(CustomEntityConfig const& config, int64_t desiredId)`

- 参数：
  - `config` : `CustomEntityConfig const&`<br>同 `create`
  - `desiredId` : `int64_t`<br>想占用的 id，必须 `> 0`；一般来自你自己存档里记下的 id
- 返回值：成功就是 `desiredId`；`desiredId <= 0` 或该 id 已被占用返回 `-2`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();

    constexpr int64_t kSavedId = 2001; // 存档里记的实体 id
    if (!entities.isIdUsed(kSavedId)) {
        int64_t id = entities.createWithId(cfg, kSavedId);
        if (id == kSavedId) {
            // 恢复成功
        }
    }
    ```

## 查询与销毁

### 判断实体是否存在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：存在 `true`，不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    if (!entities.exists(id)) {
        return; // 已经被销毁了
    }
    ```

### 读回当前配置

`bool get(int64_t id, CustomEntityConfig& out) const`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `out` : `CustomEntityConfig&`<br>出参；成功时被整体覆盖为当前配置
- 返回值：读到了 `true`；id 不存在 `false`（此时 `out` 不被改动）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();

    hologramlib::CustomEntityConfig cfg{};
    if (entities.get(id, cfg)) {
        // cfg.identifier / cfg.pose / cfg.equipment ... 都是当前值
    }
    ```

### 判断 id 是否被占用

`bool isIdUsed(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>要查的实体 id
- 返回值：已占用 `true`（此时 `createWithId` 会返回 `-2`），空闲 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    bool free = !entities.isIdUsed(2001);
    ```

### 列出全部 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有自定义实体的 id；一个都没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    for (int64_t one : entities.getAllIds()) {
        entities.setInvisible(one, false);
    }
    ```

### 读取运行态摘要

`std::string getDebugInfo(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：一行摘要（id、维度、坐标、类型、开关、缩放、姿势、装备数、视距、白名单、已见玩家数、骑乘目标）；id 不存在返回 `"not_found"`
- 返回值类型：`std::string`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    // 例：id=1 dim=0 pos=(100.5,64.0,100.5) identifier='minecraft:armor_stand' ...
    logger.info(entities.getDebugInfo(id));
    ```

### 查找最近的实体

`int64_t findNearest(float x, float y, float z, int dim, double maxDist) const`

- 参数：
  - `x` / `y` / `z` : `float`<br>参考点的世界坐标
  - `dim` : `int`<br>只在同一个维度里找；维度不同的直接跳过
  - `maxDist` : `double`<br>最大距离，单位方块；`<= 0` = 不限制
- 返回值：最近实体的库内 id；范围内没有实体返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    int64_t near = entities.findNearest(100.5f, 64.0f, 100.5f, 0, 8.0); // 8 格内最近
    if (near >= 0) {
        entities.setNametag(near, "§e你好");
    }
    ```

### 销毁一只实体

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：确实销毁了 `true`；id 不存在 `false`。客户端上那一只也会立刻消失
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.destroy(id);
    ```

### 销毁全部实体

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.destroyAll(); // 关服 / 重载插件时收场
    ```

## 属性

### 更换实体类型

`bool setIdentifier(int64_t id, std::string const& identifier)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `identifier` : `std::string const&`<br>新的实体类型；短名自动补 `minecraft:`；空串被拒绝
- 返回值：改成功 `true`；id 不存在或 `identifier` 为空 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setIdentifier(id, "minecraft:skeleton"); // 盔甲架换成骷髅
    ```

### 移动实体

`bool setPosition(int64_t id, float x, float y, float z, int dim)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `x` / `y` / `z` : `float`<br>新的世界坐标，单位方块
  - `dim` : `int`<br>`>= 0` 时把实体放到该维度；`< 0` = 只改坐标、维度不动
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPosition(id, 105.5f, 64.0f, 100.5f, -1); // 只挪位置
    entities.setPosition(id, 0.5f, 70.0f, 0.5f, 1);     // 搬到下界
    ```

只改坐标走增量包（`MoveActorAbsolute`，`kTeleport` 标志），不重建、不闪；维度变了才走重建。
`enabled` 为关、或当前没有观看者时，改动只写进配置，等下次 spawn 再按新值下发。

### 设置朝向

`bool setRotation(int64_t id, float yaw, float pitch)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `yaw` : `float`<br>偏航角（度），水平朝向
  - `pitch` : `float`<br>俯仰角（度），抬头 / 低头
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setRotation(id, 180.0f, 0.0f); // 转身面向南
    ```

### 设置头顶名字牌

`bool setNametag(int64_t id, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `text` : `std::string const&`<br>名字牌文字，支持 `§` 颜色码；空串 = 清除名字牌
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setNametag(id, "§a商人 §7(右键)");
    entities.setNametag(id, ""); // 清掉名字牌
    ```

名字牌默认只在准星对准时显示；要始终显示，在配置里把 `nametagAlwaysShow` 设为 `true`
（创建时设置，或 `get` 出来改完再 `createWithId` 覆盖）。

### 设置实体缩放

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `scale` : `float`<br>目标缩放，模型与体积等比；`<= 0` 直接拒绝；发包时钳制到客户端有效域 `0.0625 ~ 10`
- 返回值：已改 `true`；`scale <= 0` 或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setScale(id, 2.0f); // 放大到 2 倍
    entities.setScale(id, 1.0f); // 恢复原大小
    ```

### 按倍数相对缩放

`bool scaleBy(int64_t id, double factor)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `factor` : `double`<br>倍数：`> 1` 放大，`0 < factor < 1` 缩小；结果自动钳制到 `0.0625 ~ 10`
- 返回值：已改 `true`；`factor <= 0` 或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.scaleBy(id, 1.2f); // 每次调用都变大一成
    entities.scaleBy(id, 0.5f); // 缩到一半
    ```

### 设置主变种编号

`bool setVariant(int64_t id, int variant)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `variant` : `int`<br>变种编号（皮肤 / 亚种）；同一个数字在不同实体上含义不同，库不做校验
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setVariant(id, 1);
    ```

### 设置二级变种编号

`bool setMarkVariant(int64_t id, int markVariant)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `markVariant` : `int`<br>二级变种编号，含义由实体类型决定（例如热带鱼花纹、马斑纹）
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setMarkVariant(id, 3);
    ```

### 设置颜色索引

`bool setColorIndex(int64_t id, int colorIndex)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `colorIndex` : `int`<br>颜色编号，用于羊、项圈这类会染色的实体；具体色号由原版定义
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setColorIndex(id, 14); // 红色系
    ```

### 覆写原始 flags

`bool setFlags(int64_t id, std::int64_t flags)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `flags` : `std::int64_t`<br>位掩码原值，例如 `0x01` = 着火、`0x20` = 隐身
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setFlags(id, 0x01); // 视觉着火
    entities.setFlags(id, 0);    // 清掉全部 flags
    ```

这是**整块覆盖**，不会与旧值合并；`invisible` 是独立开关，两者互不影响。

### 切换隐身

`bool setInvisible(int64_t id, bool on)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `on` : `bool`<br>`true` 隐身 / `false` 现身；只动隐身的这一位
- 返回值：已改 `true`（与当前值相同也返回 `true`，不会重复发包）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setInvisible(id, true);
    entities.setInvisible(id, false); // 现身
    ```

### 开关实体总可见性

`bool setEnabled(int64_t id, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `enabled` : `bool`<br>`false` = 从所有客户端撤掉，`true` = 重新发出去
- 返回值：已设 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setEnabled(id, false); // 全场隐藏
    entities.setEnabled(id, true);  // 恢复
    ```

### 设置可见距离

`bool setViewDistance(int64_t id, double dist)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `dist` : `double`<br>距离，单位方块；`<= 0` = 不做距离限制；创建时默认 `64` 格
- 返回值：已设 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setViewDistance(id, 32.0); // 32 格内才看得到
    entities.setViewDistance(id, 0.0);  // 不限距离
    ```

### 设置姿势

`bool setPose(int64_t id, int pose)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `pose` : `int`<br>姿势编号（`PoseIndex`），只对按它摆姿的模型有效：`0` 站立、`1` 无底板、`2` 显示手臂、`3 ~ 13` 坐姿 / 睡姿 / 跳舞等；越界自动钳进 `0 ~ 13`
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPose(id, 2); // 盔甲架显示手臂
    entities.setPose(id, 0); // 回到站立
    ```

### 设置装备槽里的物品

`bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `slot` : `int`<br>`0` 主手 / `1` 副手 / `2` 头 / `3` 胸 / `4` 腿 / `5` 脚；其它值返回 `false`
  - `name` : `std::string const&`<br>物品标识符，短名自动补 `minecraft:`；空串 = 清空该槽
  - `aux` : `int`<br>物品附加值（data 值）
  - `nbt` : `std::string const&`<br>SNBT 附加数据；空串 = 不带
- 返回值：已改 `true`；`slot` 越界或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setEquipmentSlot(id, 0, "diamond_sword", 0, "");   // 主手钻石剑
    entities.setEquipmentSlot(id, 2, "golden_helmet", 0, "");   // 头上金头盔
    entities.setEquipmentSlot(id, 0, "", 0, "");                // 清空主手
    ```

### 设置客户端属性

`bool setMobProperty(int64_t id, std::string const& name, std::string const& value)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `name` : `std::string const&`<br>完整属性名，例如 `minecraft:sulfur_cube_archetype`；空串被拒绝
  - `value` : `std::string const&`<br>字符串取值，例如 `sticky`
- 返回值：已改 `true`；`name` 为空或 id 不存在 `false`。同名属性会就地更新，不会重复添加
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setMobProperty(id, "minecraft:sulfur_cube_archetype", "sticky");
    ```

属性通过 `ChangeMobProperty` 立即推给当前能看到的玩家，同时写进配置；之后再被 spawn 的玩家会收到同一份。
名字相同就覆盖旧值，名字不同就追加。适用于带 `client_sync` 的原版属性（1.23.0 追加）。

### 清除客户端属性

`bool clearMobProperties(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：已清空 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearMobProperties(id);
    ```

!!! note "清列表不等于撤回"

    它只清服务端配置列表：之后新看到这只实体的玩家不会再收到这些属性，
    但**已经收到过的客户端不会被主动还原**（没有撤回包）。要让现有客户端复原，
    用 `setMobProperty` 把该属性设回默认值。

### 用白名单限定可见玩家

`bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerNames` : `std::vector<std::string> const&`<br>玩家名数组（按 `realName` 匹配）；空数组 = 清除限制、恢复全员可见
- 返回值：已设 `true`；id 不存在 `false`。写入后立刻重算一次可见性
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setVisiblePlayers(id, {"Steve", "Alex"}); // 只给这两个人看
    entities.setVisiblePlayers(id, {});                // 取消限制
    ```

白名单是**替换**整份名单，不是追加；与维度、视距、`enabled` 叠加生效。

### 只让一名玩家看到

`bool setVisiblePlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>唯一的可见玩家名；**替换**整份名单，不是追加
- 返回值：已设 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setVisiblePlayer(id, "Steve");
    ```

### 清除白名单

`bool clearVisiblePlayers(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：成功 `true`（本来就没设白名单也返回 `true`）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearVisiblePlayers(id); // 恢复维度内全员可见
    ```

## 骑乘

### 骑到玩家头上

`bool setRidePlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>实体 id（乘客）
  - `playerName` : `std::string const&`<br>玩家名，必须在线；空串 = 清除骑乘链接
- 返回值：已设 `true`（本来没链接、空串清除也算 `true`）；id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setRidePlayer(id, "Steve"); // 挂到 Steve 头上
    entities.setRidePlayer(id, "");      // 下来
    ```

### 骑到另一只实体上

`bool setRideEntity(int64_t id, int64_t vehicleEntityId)`

- 参数：
  - `id` : `int64_t`<br>实体 id（乘客）
  - `vehicleEntityId` : `int64_t`<br>载具实体的**库内 id**；`0` = 清除链接
- 返回值：已设 `true`；载具不存在、载具就是自己、或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    int64_t horse = entities.create(horseCfg);
    entities.setRideEntity(id, horse); // id 骑到 horse 身上
    entities.setRideEntity(id, 0);     // 解除
    ```

两种骑乘互斥，后设置的生效（设玩家会把 `rideEntityId` 清零，反之亦然）。

### 解除骑乘链接

`bool clearRide(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：已解除 `true`（本来就没链接也返回 `true`）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearRide(id);
    ```

## 动画

### 播放原版动画

`bool playAnimation(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `animation` : `std::string const&`<br>资源包里的动画标识符，例如 `animation.humanoid.base_pose`；空串被拒绝
  - `stopExpression` : `std::string const&`<br>Molang 停止表达式，结果为真时停止；**空串 = 常驻不自动停**；传 `"query.any_animation"`（或 `"query.any_animation()"`）会被当成「停止」指令
  - `durationTicks` : `int`<br>`> 0` 时到这么多 tick 后自动补发停止；`0` = 不限时
- 返回值：已排队 `true`（约 2 tick 后到客户端）；id 不存在、`animation` 为空、实体被 `setEnabled` 关掉、或当前没有任何玩家看到它时 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.playAnimation(id, "animation.humanoid.base_pose", "", 0);   // 常驻
    entities.playAnimation(id, "animation.humanoid.attack", "", 20);     // 播 1 秒后停
    ```

### 只给一名玩家播动画

`bool playAnimationTo(int64_t id, std::string const& playerName, std::string const& animation, std::string const& stopExpression, int durationTicks)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>目标玩家，必须在线且当前能看到这只实体
  - `animation` / `stopExpression` / `durationTicks` : 同 `playAnimation`
- 返回值：已排队 `true`；id 不存在、玩家不在线、玩家还没看到这只实体、`animation` 为空或实体被关掉时 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.playAnimationTo(id, "Steve", "animation.humanoid.wave", "", 40);
    ```

### 停止动画

`bool stopAnimation(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：已排队停止 `true`；id 不存在或当前没有观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.stopAnimation(id);
    ```

### 只停一名玩家的动画

`bool stopAnimationTo(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>目标玩家，必须在线且当前能看到这只实体
- 返回值：已排队停止 `true`；id 不存在、玩家不在线或还没看到这只实体 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.stopAnimationTo(id, "Steve");
    ```

### 注册出生回调

`void setEntitySpawnCallback(EntitySpawnCallback callback)`

- 参数：
  - `callback` : `EntitySpawnCallback`<br>`std::function<void(int64_t id, std::string const& playerName)>`，每次「这只实体在某玩家客户端完成一次 spawn」就回调一次；传空 `{}` 取消
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();

    entities.setEntitySpawnCallback([](int64_t id, std::string const& playerName) {
        logger.info("{} 看到了实体 {}", playerName, id);
        // 玩家刚看到它：可以在这里补播一段动画
    });
    ```

!!! note "回调是「每人每次出生」"

    同一个玩家断线重连、实体被 `setEnabled` 关掉又打开，都会再触发一次。
    回调在 tick 阶段统一排空调用，别在里面做重活，也别假设它只来一次。
    回调只有一份，重复设置是**替换**，不是叠加。

## 逐玩家外观

同一个实体，让不同玩家看到不同的朝向、名字牌、缩放和装备。覆盖只对指定玩家的客户端生效，
其他人仍然看到通用配置；玩家离线、或还没看到这只实体时，设置会返回 `false`。

### 覆盖某个玩家看到的朝向

`bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw, float pitch)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>观看者名字，必须在线且已经见过这只实体
  - `yaw` : `float`<br>该玩家看到的偏航角（度）
  - `pitch` : `float`<br>该玩家看到的俯仰角（度）
- 返回值：已设 `true`；id 不存在、玩家不在线或还没见过这只实体 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPlayerRotation(id, "Steve", 180.0f, 0.0f); // 让 Steve 看到它背对自己
    ```

这是 1.20.0 的能力：出生包与增量包都按覆盖值下发，未覆盖的玩家仍用配置朝向；
变更走轻脏通道（无闪烁），下一 tick 生效。LSE 侧对应导出是
`entitySetPlayerRotation` / `entityClearPlayerRotation` / `entityClearPlayerRotations`（26.51.6 起）。

### 清除单个玩家的朝向覆盖

`bool clearPlayerRotation(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>玩家名，必须在线
- 返回值：确实清掉了 `true`；本来没有覆盖、玩家不在线或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearPlayerRotation(id, "Steve");
    ```

### 清除全部朝向覆盖

`bool clearPlayerRotations(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>实体 id
- 返回值：成功 `true`（本来就没有覆盖也返回 `true`）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearPlayerRotations(id);
    ```

### 覆盖某个玩家看到的名字牌

`bool setPlayerNametag(int64_t id, std::string const& playerName, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>观看者名字，必须在线且已经见过这只实体
  - `text` : `std::string const&`<br>只有这个玩家能看到的文字，支持 `§`；空串 = 清除覆盖、回到通用名字牌
- 返回值：已设 `true`；id 不存在、玩家不在线或还没见过这只实体 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPlayerNametag(id, "Steve", "§c只在 Steve 眼里是这样");
    entities.setPlayerNametag(id, "Steve", ""); // 还原
    ```

### 覆盖某个玩家看到的缩放

`bool setPlayerScale(int64_t id, std::string const& playerName, float scale)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>观看者名字，必须在线且已经见过这只实体
  - `scale` : `float`<br>该玩家看到的缩放，钳制到 `0.0625 ~ 10`；`<= 0` = 清除覆盖、回到通用缩放
- 返回值：已设 `true`；id 不存在、玩家不在线或还没见过这只实体 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPlayerScale(id, "Steve", 3.0f); // Steve 眼里特别大
    entities.setPlayerScale(id, "Steve", 0.0f); // 还原
    ```

### 覆盖某个玩家看到的装备

`bool setPlayerEquipmentSlot(int64_t id, std::string const& playerName, int slot, std::string const& name, int aux, std::string const& nbt)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>观看者名字，必须在线且已经见过这只实体
  - `slot` : `int`<br>`0` 主手 / `1` 副手 / `2` 头 / `3` 胸 / `4` 腿 / `5` 脚；其它值返回 `false`
  - `name` : `std::string const&`<br>物品 id；空串 = 这个槽对该玩家回退到通用装备
  - `aux` : `int`<br>物品附加值
  - `nbt` : `std::string const&`<br>SNBT 附加数据；空串 = 不带
- 返回值：已设 `true`；`slot` 越界、id 不存在、玩家不在线或还没见过这只实体 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.setPlayerEquipmentSlot(id, "Steve", 2, "diamond_helmet", 0, ""); // Steve 看到钻石头盔
    entities.setPlayerEquipmentSlot(id, "Steve", 2, "", 0, "");               // 还原该槽
    ```

### 清除单个玩家的外观覆盖

`bool clearPlayerAppearance(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>实体 id
  - `playerName` : `std::string const&`<br>玩家名，必须在线
- 返回值：确实清掉了覆盖 `true`；该玩家本来就没有任何外观覆盖、id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& entities = hologramlib::IHologramLib::getInstance().customEntities();
    entities.clearPlayerAppearance(id, "Steve"); // Steve 眼中的它恢复成通用配置
    ```

它清的是**名字牌、缩放、装备**三项覆盖；朝向覆盖是独立的一组，要用 `clearPlayerRotation` /
`clearPlayerRotations` 清。

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

// 造一只拿剑的盔甲架「守卫」：始终显示名字、面向南；Steve 眼里的名字牌和朝向单独覆盖
int64_t spawn_guard() {
    auto& lib      = hologramlib::IHologramLib::getInstance();
    auto& entities = lib.customEntities();

    hologramlib::CustomEntityConfig cfg;
    cfg.identifier        = "armor_stand";
    cfg.x                 = 100.5f;
    cfg.y                 = 64.0f;
    cfg.z                 = 100.5f;
    cfg.dimension         = 0;
    cfg.yaw               = 180.0f;
    cfg.nametag           = "§c守卫";
    cfg.nametagAlwaysShow = true;
    cfg.scale             = 1.0f;
    cfg.viewDistance      = 64.0;
    cfg.equipment[0].name = "minecraft:iron_sword";  // 主手
    cfg.equipment[2].name = "minecraft:iron_helmet"; // 头

    int64_t id = entities.create(cfg);
    if (id < 0) {
        return -1;
    }

    // 逐玩家覆盖必须等玩家「已经看到」这只实体：刚 create 完还没有人看到，直接调用会返回 false。
    // 出生回调正好是这个时机。
    entities.setEntitySpawnCallback([&entities, id](int64_t spawnedId, std::string const& playerName) {
        if (spawnedId != id || playerName != "Steve") {
            return;
        }
        entities.setPlayerNametag(id, playerName, "§e老熟人");
        entities.setPlayerRotation(id, playerName, 0.0f, 0.0f);
    });

    return id;
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
