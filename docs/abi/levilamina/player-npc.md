# 1.9 IPlayerNpc · 假玩家 NPC

纯协议假玩家：客户端看到一个带皮肤的玩家模型，服务端**没有这个玩家**，不占 BDS 实体系统。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().playerNpcs()`
- 皮肤注册表：进程内全局共享。注册一次，任意多个 NPC 复用，解码 / 采集只做一次
- 坐标：世界坐标，单位方块；`dim` 用 `0` 主世界 / `1` 下界 / `2` 末地
- 缩放：`0.0625 ~ 10`，碰撞箱（宽 `0.6 × scale`、高 `1.8 × scale`）等比跟随
- id：`int64_t`，`create` 系列从 `1` 自增或落在随机段（`0x10000000` 起）；失败为负数

!!! info "26.40.2 起皮肤可正常渲染"

    皮肤注册（PNG / 在线采集 / 目录导入）、`getSkinBlob` / `registerSkinFromBlob` 导出恢复、
    创建 / 移动 / 朝向 / 缩放 / 视距 / 显隐、点击交互全部可用。
    此前的「客户端不渲染皮肤、回退默认模型」根因是 `PlayerList` 皮肤条目的 `Id` / `FullId` 为空或残留了原玩家缓存键，
    已在 `finalizeSkinIds` 里统一补全（PNG 注册 / 采集 / blob 恢复三条路都走它）。

!!! tip "库不落盘"

    皮肤和 NPC 都只在内存里。持久化由消费方负责：
    `getSkinBlob` 导出二进制快照 → 自己存盘 → 重启时 `registerSkinFromBlob` 恢复；
    NPC 本身要按自己的存档记录用 `createWithId` 重建（能还原原来的 id）。
    不依赖源 PNG、也不依赖源玩家在线。

!!! info "能力版本"

    - 1.19.0（26.10.7）：`geometryData` 自定义模型、缩放
    - 1.20.0：`setRotationLight` 轻量朝向
    - 1.26.0（26.40.8）：`setPositionLight`、`injectSkin` / `injectSkinAll`、`playAnimation` / `playAnimationTo`、`setEntitySpawnCallback`
    - 1.32.0（26.51.6）：`playEmote`

    26.40.2 的皮肤渲染修复见上。

## 数据结构

```cpp
struct PlayerNpcSkin {
    std::string pngPath;                                  // PNG 路径（方形，边长 2 的幂且 >= 64）
    std::string skinId;                                   // 空 = 用文件名（不含扩展名）
    std::string geometry{"geometry.humanoid.custom"};     // 模型（resourcePatch 里的默认几何）
    std::string armSize{"wide"};                          // "wide" / "slim"（其它值按 wide）
    std::string geometryData;                             // 完整几何 JSON（可选，提供时启用自定义模型）
};

struct PlayerNpcConfig {
    std::string name{"NPC"};        // 显示名（名字牌 + PlayerList 条目同名）
    std::string skinId{"default"};  // 必须已注册，否则创建失败 -3
    float       x{0}, y{64}, z{0};  // 出生坐标
    int         dimension{0};       // 维度 id
    float       yaw{0};             // 出生朝向（度）
    double      viewDistance{96.0}; // 可见距离（方块）；<= 0 = 不限
    float       scale{1.0f};        // 模型与碰撞箱缩放，0.0625 ~ 10
    bool        enabled{true};      // false = 创建后不生成（等同立刻 setEnabled(false)）
};
```

- `PlayerNpcSkin.pngPath` : `std::string`<br>PNG 路径；解码失败（不存在、不是方形、边长不是 2 的幂、小于 64）注册返回 `false`
- `PlayerNpcSkin.skinId` : `std::string`<br>皮肤名（就是 `create` 时填的值）；留空用 PNG 文件名，重复注册会覆盖
- `PlayerNpcSkin.geometry` : `std::string`<br>默认几何标识；`geometryData` 非空时以 JSON 里的 identifier 为准
- `PlayerNpcSkin.armSize` : `std::string`<br>`"wide"` 宽臂 / `"slim"` 细臂
- `PlayerNpcSkin.geometryData` : `std::string`<br>完整几何 JSON（1.18.0 格式）；空 = 用标准玩家模型
- `PlayerNpcConfig.name` : `std::string`<br>显示名；空串 = 无名字牌
- `PlayerNpcConfig.skinId` : `std::string`<br>必须已注册；**注册表里没有内置的 `"default"`**，不先注册一个同名皮肤就会 `-3`
- `PlayerNpcConfig.viewDistance` : `double`<br>可见距离；进入用 `viewDistance`，退出要超出 `viewDistance + 4`（滞回，避免边界反复重发）
- `PlayerNpcConfig.scale` : `float`<br>缩放；`create` 时不校验范围，客户端按 `0.0625 ~ 10` 夹紧；运行中改请用 `setScale`

## 皮肤注册表

### 用 PNG 注册一套皮肤

`bool registerSkin(PlayerNpcSkin const& skin)`

- 参数：
  - `skin` : `PlayerNpcSkin const&`<br>PNG 路径 + 皮肤名 + 可选几何；`skinId` 留空用文件名，重复注册覆盖旧条目
- 返回值：注册成功 `true`；路径不存在、PNG 不是方形 / 边长不是 2 的幂 / 小于 64、解码失败都返回 `false`（原因写日志）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& npc = hologramlib::IHologramLib::getInstance().playerNpcs();

    hologramlib::PlayerNpcSkin skin;
    skin.pngPath = "plugins/MyPlugin/skins/girl.png";
    skin.skinId  = "girl";          // 留空则用 "girl.png" 的文件名 "girl"
    skin.armSize = "slim";
    if (!npc.registerSkin(skin)) {
        return; // PNG 不合法
    }
    ```

### 从在线玩家采集皮肤

`bool captureSkin(std::string const& skinId, std::string const& playerName)`

- 参数：
  - `skinId` : `std::string const&`<br>注册到哪个名字下；空串直接失败
  - `playerName` : `std::string const&`<br>要采集的玩家名，必须在线
- 返回值：采集成功 `true`；皮肤名空、玩家不在线、玩家没有可用皮肤对象都返回 `false`。重复同名会覆盖
- 返回值类型：`bool`
- 示例：

    ```cpp
    // 采集一次，之后玩家换肤不影响这个 NPC
    npc.captureSkin("steve_snapshot", "Steve");
    ```

!!! note "采集的是全量快照"

    贴图、披风、动画贴图、几何、Persona 部件、染色全部拷贝，以 `skinId` 注册成运行时快照。
    所以采集完之后目标玩家换肤，这个 `skinId` 的外观不会跟着变。

### 批量导入一个目录的皮肤

`int importSkins(std::string const& dirPath)`

- 参数：
  - `dirPath` : `std::string const&`<br>根目录；**只扫描下一层子目录**，一个子目录 = 一套皮肤（子目录名即 `skinId`）
- 返回值：成功导入的数量；目录不存在或不是目录返回 `-1`。没有 PNG 的子目录被跳过（只记日志，不算失败）
- 返回值类型：`int`
- 示例：

    ```cpp
    // plugins/MyPlugin/skins/girl/{girl.png, girl.json}
    // plugins/MyPlugin/skins/guard/guard.png
    int n = npc.importSkins("plugins/MyPlugin/skins");
    // n == 2
    ```

!!! note "导入规则"

    每个子目录取**第一个** `.png`（必需）和**第一个** `.json`（可选，作为 `geometryData`）。
    根目录下直接放的 PNG 不会被导入 —— 必须放进子目录。库不落盘，导入后仍需自己用
    `getSkinBlob` 导出持久化。

### 查询皮肤是否已注册

`bool hasSkin(std::string const& skinId) const`

- 参数：
  - `skinId` : `std::string const&`<br>皮肤名
- 返回值：已注册 `true`；未注册 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (!npc.hasSkin("girl")) {
        return;
    }
    ```

### 注销一套皮肤

`bool unregisterSkin(std::string const& skinId)`

- 参数：
  - `skinId` : `std::string const&`<br>皮肤名
- 返回值：注销成功 `true`；未注册，或**还有 NPC 正在用这套皮肤**时返回 `false`（拒绝注销）
- 返回值类型：`bool`
- 示例：

    ```cpp
    // 先把引用它的 NPC 换肤或销毁，再注销
    npc.unregisterSkin("girl");
    ```

### 列出已注册的皮肤

`std::vector<std::string> getSkinIds() const`

- 参数：无
- 返回值：当前注册表里的全部皮肤名；没有时是空数组
- 返回值类型：`std::vector<std::string>`
- 示例：

    ```cpp
    for (auto const& id : npc.getSkinIds()) {
        logger.info("skin: {}", id);
    }
    ```

### 导出皮肤快照

`bool getSkinBlob(std::string const& skinId, std::string& out) const`

- 参数：
  - `skinId` : `std::string const&`<br>皮肤名
  - `out` : `std::string&`<br>输出参数，写入**二进制**快照（`HLNS` 魔数 + 版本，非文本、不是 JSON）
- 返回值：导出成功 `true`；皮肤未注册 `false`（此时 `out` 不动）
- 返回值类型：`bool`
- 示例：

    ```cpp
    std::string blob;
    if (npc.getSkinBlob("girl", blob)) {
        std::ofstream out("plugins/MyPlugin/skins/girl.hlns", std::ios::binary);
        out.write(blob.data(), static_cast<std::streamsize>(blob.size()));
    }
    ```

### 从快照恢复皮肤

`bool registerSkinFromBlob(std::string const& blob)`

- 参数：
  - `blob` : `std::string const&`<br>`getSkinBlob` 导出的二进制快照（按二进制读回，不要做文本转换）
- 返回值：恢复成功 `true`；魔数 / 版本不对、内容截断、`skinId` 为空都返回 `false`；同名会覆盖
- 返回值类型：`bool`
- 示例：

    ```cpp
    std::ifstream in("plugins/MyPlugin/skins/girl.hlns", std::ios::binary);
    std::string blob{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};

    if (!blob.empty()) {
        npc.registerSkinFromBlob(blob); // 源 PNG 和原玩家都不需要了
    }
    ```

## 创建与销毁

### 创建一个假玩家

`int64_t create(PlayerNpcConfig const& config)`

- 参数：
  - `config` : `PlayerNpcConfig const&`<br>创建参数（名字 / 皮肤 / 坐标 / 维度 / 朝向 / 视距 / 缩放 / 开关）；`skinId` 必须已注册
- 返回值：假玩家 id（自增，从 `1` 起、只增不减，销毁后 id 不会复用）；失败返回负数：
  `-1` = 本域被禁用（`HLIB_NO_NPC=1`）；`-3` = 皮肤未注册
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    hologramlib::PlayerNpcConfig cfg;
    cfg.name      = "§e小美";
    cfg.skinId    = "girl";
    cfg.x         = 100.5f;
    cfg.y         = 64.0f;
    cfg.z         = 100.5f;
    cfg.dimension = 0;
    cfg.yaw       = 180.0f;

    int64_t id = npc.create(cfg);
    if (id < 0) {
        return; // -3 基本都是皮肤没注册
    }
    ```

!!! note "创建是延迟的，而且会进 Tab 列表"

    调用只登记配置，真正的包在 tick 末尾发：`PlayerList(Add)` 带皮肤 → `AddPlayer` → `PlayerSkin`。
    这串包会先让假玩家出现在 Tab 名单里。库**不发** `PlayerList(Remove)` ——
    客户端在皮肤条目仍活跃时移除该条目会崩，所以名字会一直留在 Tab 列表里，实体本身照常销毁。
    另外单次扫描 / 脏刷新最多生成 2 个 NPC（防止客户端一次收到大量「新玩家」而断线）。

### 用随机 id 创建

`int64_t createRandom(PlayerNpcConfig const& config)`

- 参数：
  - `config` : `PlayerNpcConfig const&`<br>同 `create`
- 返回值：随机段（`0x10000000` 起）的假玩家 id，不会和自增 id 冲突；失败返回 `-1`（重试 128 次仍撞号或本域被禁用）或 `-3`（皮肤未注册）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    int64_t id = npc.createRandom(cfg);
    ```

### 用指定 id 创建

`int64_t createWithId(PlayerNpcConfig const& config, int64_t desiredId)`

- 参数：
  - `config` : `PlayerNpcConfig const&`<br>同 `create`
  - `desiredId` : `int64_t`<br>想用的 id，必须 `> 0` 且当前没被占用
- 返回值：成功返回 `desiredId`（可用于重启后还原旧存档里的 id）；失败 `-2` = `desiredId <= 0` 或已被占用，`-3` = 皮肤未注册，`-1` = 本域被禁用
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    int64_t id = npc.createWithId(cfg, 7); // 还原成上次的 id 7
    ```

### 销毁一个假玩家

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：成功 `true`（已经看到它的玩家会收到移除包）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.destroy(id);
    ```

### 销毁全部假玩家

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    npc.destroyAll();
    ```

### 查询假玩家是否存在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：存在 `true`；不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (npc.exists(id)) {
        npc.setEnabled(id, false);
    }
    ```

### 读取配置副本

`bool get(int64_t id, PlayerNpcConfig& out) const`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `out` : `PlayerNpcConfig&`<br>输出参数，写入当前配置
- 返回值：成功 `true`；id 不存在 `false`（`out` 不动）
- 返回值类型：`bool`
- 示例：

    ```cpp
    hologramlib::PlayerNpcConfig cfg;
    if (npc.get(id, cfg)) {
        logger.info("{} @ ({}, {}, {})", cfg.name, cfg.x, cfg.y, cfg.z);
    }
    ```

### 查询 id 是否已被占用

`bool isIdUsed(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>要检查的 id
- 返回值：已被某个假玩家占用 `true`；空闲 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (!npc.isIdUsed(7)) {
        npc.createWithId(cfg, 7);
    }
    ```

### 列出全部假玩家 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有假玩家 id；没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t id : npc.getAllIds()) {
        npc.destroy(id);
    }
    ```

## 基础属性

### 移动假玩家（可选换维度）

`bool setPosition(int64_t id, float x, float y, float z, int dim)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `x` / `y` / `z` : `float`<br>新的世界坐标
  - `dim` : `int`<br>目标维度；**传负数 = 只改坐标、保持原维度**
- 返回值：成功 `true`；id 不存在 `false`。属于结构变更：tick 末尾合并成一次「先删后生成」（新 runtimeId、同一 uniqueId），会看到一次重生成
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setPosition(id, 105.5f, 64.0f, 100.5f, -1); // 只挪坐标
    npc.setPosition(id, 105.5f, 64.0f, 100.5f, 1);  // 换到下界
    ```

### 设置朝向

`bool setRotation(int64_t id, float yaw)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `yaw` : `float`<br>朝向角（度）；落到协议里会量化成 1/256 圈（约 1.41°）
- 返回值：成功 `true`；id 不存在 `false`。结构变更，tick 末尾重建一次
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setRotation(id, 90.0f); // 面朝 -X（西）
    ```

### 改名字牌

`bool setNametag(int64_t id, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `text` : `std::string const&`<br>名字牌文字，支持 `§` 颜色码；空串 = 清除名字牌
- 返回值：成功 `true`；id 不存在 `false`。文本与当前一致时直接返回 `true`、不触发重发
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setNametag(id, "§e小美 §7<店员>");
    npc.setNametag(id, "");
    ```

### 换一套皮肤

`bool setSkin(int64_t id, std::string const& skinId)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `skinId` : `std::string const&`<br>已注册的皮肤名
- 返回值：成功 `true`；皮肤未注册或 id 不存在 `false`。已经看到它的玩家会经历一次重生成（重发整套皮肤包）
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setSkin(id, "guard");
    ```

### 缩放模型并同步碰撞箱

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `scale` : `float`<br>缩放倍数；客户端硬限 `0.0625 ~ 10`
- 返回值：成功 `true`；超出 `0.0625 ~ 10` 或 id 不存在 `false`（**不夹紧，直接拒绝**）。碰撞箱同步为宽 `0.6 × scale`、高 `1.8 × scale`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setScale(id, 1.5f); // 放大一半
    npc.setScale(id, 0.5f); // 半身大小
    ```

### 设置可见距离

`bool setViewDistance(int64_t id, double dist)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `dist` : `double`<br>可见距离（方块）；`<= 0` = 不限距离（默认 `96`）
- 返回值：成功 `true`；id 不存在 `false`。进出带 4 格滞回
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setViewDistance(id, 32.0);
    npc.setViewDistance(id, 0.0); // 不限
    ```

### 打开或关闭显示

`bool setEnabled(int64_t id, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `enabled` : `bool`<br>`false` = 从所有观看者处移除并停止生成（id 仍有效）；`true` = 恢复生成
- 返回值：成功 `true`；id 不存在 `false`。变更在下一 tick 的脏刷新里生效（单轮最多生成 2 个，多个 NPC 同时恢复会顺延）
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setEnabled(id, false); // 打烊
    npc.setEnabled(id, true);  // 开门
    ```

## 可见玩家白名单

### 只让名单里的玩家看到

`bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerNames` : `std::vector<std::string> const&`<br>玩家名列表（按 `realName` 匹配）；**空数组 = 清除限制、恢复全员可见**
- 返回值：成功 `true`；id 不存在 `false`。不在名单里的观看看者会被立刻移除
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setVisiblePlayers(id, {"Steve", "Alex"});
    npc.setVisiblePlayers(id, {}); // 全员
    ```

!!! note "按名字匹配，玩家改名后失效"

    白名单存的是玩家名，不是 UUID。玩家改名后旧名单不再命中，需要重新设置。

### 白名单只填一个玩家

`bool setVisiblePlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>唯一可见的玩家名
- 返回值：成功 `true`；id 不存在 `false`。**是覆盖不是追加**：会把名单整体替换成「只有这个人」
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setVisiblePlayer(id, "Steve"); // 只有 Steve 看得到
    ```

### 清除白名单

`bool clearVisiblePlayers(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：成功 `true`；id 不存在 `false`。等效于传空数组给 `setVisiblePlayers`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.clearVisiblePlayers(id);
    ```

## 轻量通道（不重建实体）

### 轻量移动

`bool setPositionLight(int64_t id, float x, float y, float z, int dim)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `x` / `y` / `z` : `float`<br>新的世界坐标
  - `dim` : `int`<br>目标维度；`< 0` = 不校验、只挪坐标；**与当前维度不同时返回 `false`**
- 返回值：成功 `true`；`dim` 与当前维度不同、或 id 不存在 `false`。只发 `MoveActorAbsolute`，不重建实体、不重发皮肤、无闪烁
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (!npc.setPositionLight(id, x, y, z, 0)) {
        npc.setPosition(id, x, y, z, 0); // 跨维度只能走重建
    }
    ```

### 轻量转向

`bool setRotationLight(int64_t id, float yaw)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `yaw` : `float`<br>朝向角（度）
- 返回值：成功 `true`；id 不存在 `false`。只发移动包的朝向字段，不重建
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setRotationLight(id, 45.0f);
    ```

!!! tip "轻量通道可以每 tick 喂"

    `setPositionLight` 和 `setRotationLight` 共用同一条「轻脏」通道，同一 tick 内多次调用合并成**一条** `MoveActorAbsolute`
    发给每个已见过它的玩家。适合让 NPC 跟着会移动的目标走；位置和朝向一起改时，两次调用也只出一条包。

### 覆盖单个玩家看到的朝向

`bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>玩家名；**该玩家必须已经能看到这个 NPC**
  - `yaw` : `float`<br>只对这个玩家生效的朝向（度）
- 返回值：成功 `true`；id 不存在、玩家不在线、玩家还没见过这个 NPC 都返回 `false`。出生包与增量包都会按覆盖值发
- 返回值类型：`bool`
- 示例：

    ```cpp
    // 让 NPC 一直面向 Steve（只影响 Steve 的客户端）
    npc.setPlayerRotation(id, "Steve", yawToSteve);
    ```

### 清除单玩家的朝向覆盖

`bool clearPlayerRotation(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：成功清除 `true`；id 不存在、玩家不在线、该玩家本来就没有覆盖都返回 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.clearPlayerRotation(id, "Steve");
    ```

### 清除全部朝向覆盖

`bool clearPlayerRotations(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：成功 `true`（本来就没有覆盖也返回 `true`）；id 不存在 `false`。清除后所有人回到 `config.yaw`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.clearPlayerRotations(id);
    ```

## 皮肤注入

### 给指定观看者就地换肤

`bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId)`

- 参数：
  - `viewerName` : `std::string const&`<br>观看者玩家名；**空串 = 所有在线玩家（含 target 本人）**
  - `targetName` : `std::string const&`<br>要换肤的玩家名（真实玩家），必须在线
  - `skinId` : `std::string const&`<br>已注册的皮肤名
- 返回值：至少成功发给一个观看者 `true`；皮肤未注册、target 不在线、指定的 viewer 不在线都返回 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.injectSkin("Steve", "Alex", "girl"); // 只让 Steve 看到 Alex 换了皮
    ```

!!! note "这是「让真实玩家看到自己换肤」的正路"

    用 target 自己的 UUID / uniqueId 发一条 `PlayerList(Add)` 就地更新皮肤条目，带真实 xuid 与
    `OverridesPlayerAppearance = true`（缺这个客户端会拒绝覆盖已装备皮肤）。
    **不会自动重发** —— 客户端偶发丢帧时由消费方再调一次（常见的做法是约 100 ms 后补一次）。

### 给所有在线玩家就地换肤

`bool injectSkinAll(std::string const& targetName, std::string const& skinId)`

- 参数：
  - `targetName` : `std::string const&`<br>要换肤的玩家名，必须在线
  - `skinId` : `std::string const&`<br>已注册的皮肤名
- 返回值：至少成功发给一个玩家 `true`；否则 `false`（等价 `injectSkin("", targetName, skinId)`）
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.injectSkinAll("Alex", "girl");
    ```

## 动画与表情

### 按名播放动画

`bool playAnimation(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `animation` : `std::string const&`<br>动画标识符，如 `animation.npc.wave`；必须能被客户端解析（原版动画或随资源包下发）；空串失败
  - `stopExpression` : `std::string const&`<br>停止表达式；**空串 = 常驻**，`query.any_animation` 或 `query.any_animation()` = 立刻停
  - `durationTicks` : `int`<br>`> 0` 时到期自动补发停止包（tick）；`<= 0` 不自动停
- 返回值：已把动画包排进队列 `true`；id 不存在、动画名为空、NPC 被关闭、没有玩家见过它都返回 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.playAnimation(id, "animation.npc.wave", "", 0);    // 常驻，不自动停
    npc.playAnimation(id, "animation.npc.wave", "", 40);   // 2 秒后自动补停止包
    npc.playAnimation(id, "animation.npc.wave", "query.any_animation", 0); // 立刻停
    ```

!!! info "控制器名由库自动唯一化"

    库给每个 NPC 生成 `controller.animation.holo_npc_<id>` 当 `AnimateEntityPacket` 的控制器名，
    所以**资源包里不需要预先存在同名控制器**；动画本身仍要能被客户端解析。
    包在 `+2 tick` 后发出，只发给「已经见过这个 NPC」的玩家。

### 只给一个玩家播动画

`bool playAnimationTo(int64_t id, std::string const& playerName, std::string const& animation, std::string const& stopExpression, int durationTicks)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>目标玩家名；**必须已经能看到这个 NPC**
  - `animation` / `stopExpression` / `durationTicks` : 同 `playAnimation`
- 返回值：成功 `true`；id 不存在、动画名为空、NPC 被关闭、玩家不在线或还没见过它都返回 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.playAnimationTo(id, "Steve", "animation.npc.wave", "", 0); // 补发给刚进视距的玩家
    ```

### 停止动画

`bool stopAnimation(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：已给全部观看者排入停止包 `true`；id 不存在或没有任何观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.stopAnimation(id);
    ```

### 停止某个玩家的动画

`bool stopAnimationTo(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>目标玩家名；必须已经能看到这个 NPC
- 返回值：成功 `true`；id 不存在、玩家不在线或没见过它 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.stopAnimationTo(id, "Steve");
    ```

### 播放原版表情

`bool playEmote(int64_t id, std::string const& pieceId, int lengthTicks = 0)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `pieceId` : `std::string const&`<br>表情片段 id（客户端 Emote 包的 `piece_id`）；空串失败
  - `lengthTicks` : `int`<br>`0` = 用客户端自己的时长；负数按 `0` 处理
- 返回值：已把表情包发给观看者 `true`；id 不存在、片段为空、没有观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.playEmote(id, "0a1b2c3d-0000-0000-0000-000000000000", 0);
    ```

### 注册出生回调

`void setEntitySpawnCallback(std::function<void(int64_t id, std::string const& playerName)> callback)`

- 参数：
  - `callback` : `std::function<void(int64_t id, std::string const& playerName)>`<br>NPC 对某个玩家「出生完成」时回调；传空函数 = 取消注册。多次调用会**替换**前一个
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    npc.setEntitySpawnCallback([&npc](int64_t id, std::string const& playerName) {
        // 玩家刚看到这只 NPC：补发状态 / 动画
        npc.playAnimationTo(id, playerName, "animation.npc.idle", "", 0);
    });
    ```

!!! tip "回调在 tick 末尾统一 drain"

    出生事件先入队，主线程 tick 末尾统一回调（不在生成包的调用栈里），所以回调里可以安全调用本域方法。
    每次重生成（位置 / 皮肤 / 缩放等结构变更、玩家重新进入视距）都会再触发一次 —— 正好用来补发状态。
    这是本页唯一的回调；脚本侧用 `playerNpcPollSpawns` 轮询同一份队列。

## 姿势、装备与骑乘

### 设置姿势

`bool setPose(int64_t id, int pose)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `pose` : `int`<br>客户端 `PoseIndex` 元数据值，越界自动夹紧到 `0 ~ 13`；`0` = 站立；`1` / `3` 还会各补一个姿态标志位（分别对应潜行位 / 睡觉位），其它值由客户端模型解释
- 返回值：成功 `true`；id 不存在 `false`。只对按姿势摆姿的模型有效
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setPose(id, 1); // 潜行
    npc.setPose(id, 0); // 站直
    ```

### 设置装备槽

`bool setEquipment(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `slot` : `int`<br>`0` = 主手、`1` = 副手、`2` = 头、`3` = 胸、`4` = 腿、`5` = 脚；其它值返回 `false`
  - `name` : `std::string const&`<br>物品 id（可省略 `minecraft:` 前缀）；**空串 = 清空该槽**（走重建）
  - `aux` : `int`<br>数据值 / 变种，一般 `0`
  - `nbt` : `std::string const&`<br>物品 NBT 的 SNBT 字符串，`""` = 无 NBT
- 返回值：成功 `true`；槽位越界或 id 不存在 `false`。设置非空物品只发装备增量包（不重建）；清空要等下一次重建
- 返回值类型：`bool`
- 示例：

    ```cpp
    // 主手钻石剑（带自定义名字的 SNBT）
    npc.setEquipment(id, 0, "minecraft:diamond_sword", 0,
                     "{display:{Name:'{\"text\":\"§b试炼之刃\"}'}}");
    npc.setEquipment(id, 2, "minecraft:golden_helmet", 0, ""); // 头盔
    npc.setEquipment(id, 1, "", 0, "");                        // 清空副手
    ```

### 骑到指定实体上

`bool setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `vehicleUniqueId` : `std::int64_t`<br>载具的 **uniqueId**；`0` = 解除骑乘
- 返回值：成功 `true`；id 不存在、载具实体不存在、载具就是自己（`vehicleUniqueId` 等于 NPC 自己的 uniqueId）都返回 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setRideUniqueId(id, someEntityUniqueId);
    npc.setRideUniqueId(id, 0); // 下车
    ```

### 骑到某个玩家身上

`bool setRidePlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `playerName` : `std::string const&`<br>玩家名，必须在线
- 返回值：成功 `true`；id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setRidePlayer(id, "Steve");
    ```

### 骑到某个自定义实体上

`bool setRideEntity(int64_t id, int64_t vehicleEntityId)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
  - `vehicleEntityId` : `int64_t`<br>**自定义实体的库内 id**（`ICustomEntity::create` 返回的那个，不是 uniqueId）
- 返回值：成功 `true`；id 不存在、自定义实体 id 找不到 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.setRideEntity(id, mountEntityId);
    ```

### 解除骑乘

`bool clearRide(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：成功 `true`；id 不存在 `false`。等价 `setRideUniqueId(id, 0)`，本来就脚着地也算成功
- 返回值类型：`bool`
- 示例：

    ```cpp
    npc.clearRide(id);
    ```

## 诊断

### 取运行态摘要

`std::string getDebugInfo(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>假玩家 id
- 返回值：一行诊断文字（名字、皮肤、坐标、维度、朝向、视距、开关、uniqueId / runtimeId、已见玩家数、Tab 待处理、白名单）；**id 不存在返回 `"not_found"`**
- 返回值类型：`std::string`
- 示例：

    ```cpp
    logger.info("{}", npc.getDebugInfo(id));
    // id=7 name='§e小美' skin=girl pos=(100.5,64.0,100.5) dim=0 yaw=180 view=96 enabled=1 ...
    ```

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

void spawn_shop_npc() {
    auto& lib = hologramlib::IHologramLib::getInstance();
    auto& npc = lib.playerNpcs();

    // 1) 皮肤：PNG 注册（重启后改成 registerSkinFromBlob 恢复）
    hologramlib::PlayerNpcSkin skin;
    skin.pngPath = "plugins/MyPlugin/skins/girl.png";
    skin.skinId  = "girl";
    skin.armSize = "slim";
    if (!npc.registerSkin(skin)) {
        return;
    }

    // 2) 创建：脚踩方块，面朝北（yaw = 180）
    hologramlib::PlayerNpcConfig cfg;
    cfg.name      = "§e小美";
    cfg.skinId    = "girl";
    cfg.x         = 100.5f;
    cfg.y         = 64.0f;
    cfg.z         = 100.5f;
    cfg.dimension = 0;
    cfg.yaw       = 180.0f;
    cfg.scale     = 1.0f;

    int64_t id = npc.create(cfg);
    if (id < 0) {
        return; // -3：皮肤没注册
    }

    // 3) 外观与装备
    npc.setNametag(id, "§e小美 §7<店员>");
    npc.setEquipment(id, 0, "minecraft:emerald", 0, "");
    npc.setPose(id, 0);

    // 4) 玩家一看到她就挥手
    npc.setEntitySpawnCallback([&npc](int64_t npcId, std::string const& playerName) {
        npc.playAnimationTo(npcId, playerName, "animation.npc.wave", "", 40);
    });

    // 5) 点她打开对话（点击走 ghost 通道，domain = "npc"）
    lib.setGhostInteractListener([&lib](hologramlib::GhostInteractEvent const& e) {
        if (e.action != 1 || e.domain != "npc") {
            return;
        }
        hologramlib::NpcDialogSpec dialog;
        dialog.npcName = "§e小美";
        dialog.dialogue = "欢迎光临，要点什么？";
        dialog.buttons.push_back({"§a看看货", {"say hi"}, "shop", 0});
        lib.npcDialogs().open(e.playerName, dialog);
    });
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
