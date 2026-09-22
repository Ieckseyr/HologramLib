# HologramLib API 参考

- API 版本：1.23.0（`HOLOGLIB_API_VERSION 0x011D00`）
- 唯一公开头：`include/hologramlib/HologramLib.h`

## API 稳定性契约

| 层级 | 冻结内容 | 演进规则 |
|------|----------|----------|
| C++ 接口 | 全部纯虚方法签名与语义（实现对象在 DLL 内创建，消费者只持引用） | 只在接口尾部追加；永不修改/删除 |
| C++ 宏 | `HOLOGLIB_API_VERSION`、`HOLOGLIB_API`、`hologramlib` 命名空间、枚举值 | 只追加枚举值 |
| LSE 命名空间 | 单一命名空间 `HologramLib` 全部函数名、参数顺序、返回值类型 | 只增不改不删 |
| 版本协商 | `IHologramLib::version()`（BCD：0x011D00 = 1.23.0） | 随发布递增 |

破坏兼容仅允许发生在大版本（2.0.0）。`src/` 目录一切内容均为内部实现，不属于 API。

---

## 1. C++ 接口

### 1.1 入口

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // 唯一 dllexport 工厂
        IShapeDrawer&   shapes();          // 形状渲染
        IHologramText&  holograms();       // 悬浮字全息
        IItemDetail&    itemDetails();     // 物品详情
        IItemDisplay&    itemDisplays();   // FMBE 物品悬浮（1.6.0）
        ICustomEntity&  customEntities();  // 自定义实体（1.10.0）
        IParticleShape& particleShapes();  // 通用粒子形状（1.14.0）
        IPlayerNpc&     playerNpcs();      // 假玩家 NPC（1.16.0）
        bool     isLseAvailable();        // LSE 兼容层是否已挂载
        uint32_t version();               // 0x011700
        int64_t  findNearestItemDisplay(float x, float y, float z, int dim, double maxDist); // 1.6.0
        void     setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.12.0
        void     clearGhostInteractListener();                                             // 1.12.0
        std::vector<std::string> pollGhostInteractions();                                  // 1.12.0 LSE 轮询版
        uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.19.1 多播
        bool     removeGhostInteractListener(uint64_t token);                               // 1.19.1
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）；坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；发包在调用线程执行，建议主线程调用

### 1.2 IShapeDrawer（形状渲染）

| 方法 | 签名 | 说明 |
|------|------|------|
| createText | `(float x, float y, float z, std::string const& text) -> int64_t` | 文本形状 |
| createLine | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 线段 |
| createBox | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 盒体（两点对角） |
| createCircle | `(x, y, z, scale: float) -> int64_t` | 圆 |
| createSphere | `(x, y, z, scale: float) -> int64_t` | 球 |
| createArrow | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 箭头 |
| setColor | `(int64_t id, float r, float g, float b, float a) -> bool` | RGBA |
| setScale | `(int64_t id, float scale) -> bool` | 缩放 |
| setDuration | `(int64_t id, float seconds) -> bool` | 存留时长 |
| setDimension | `(int64_t id, int dimId) -> bool` | 维度 |
| setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| setText | `(int64_t id, std::string const& text) -> bool` | 文本内容 |
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | 旋转（弧度） |
| clearRotation | `(int64_t id) -> bool` | 清除旋转 |
| draw | `(int64_t id) -> bool` | 全维度可见者绘制 |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | 指定玩家 |
| drawToDimension | `(int64_t id, int dimId) -> bool` | 指定维度 |
| remove | `(int64_t id) -> bool` | 隐藏（保留数据） |
| update | `(int64_t id) -> bool` | 可见时原地重发（同 networkId 覆盖，无闪烁） |
| destroy | `(int64_t id) -> bool` | 销毁 |
| destroyAll | `() -> void` | 全部销毁 |
| exists | `(int64_t id) -> bool` | 存在性 |
| type | `(int64_t id) -> ShapeType` | 形状类型 |

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

### 1.3 IHologramText（悬浮字全息）

整块多行文本单一背景框渲染（`\n` 合并所有行）；变更后调用 `refresh` 原地重绘（同 networkId 覆盖，无闪烁）。行索引 0 起。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create | `(float x, float y, float z) -> int64_t` | 创建 |
| | destroy | `(int64_t id) -> bool` | 销毁 |
| | destroyAll | `() -> void` | 全部销毁 |
| 行管理 | addLine | `(int64_t id, std::string const& text) -> bool` | 追加行 |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | 行文本 |
| | setLineScale | `(int64_t id, int line, float scale) -> bool` | 行缩放 |
| | removeLine | `(int64_t id, int line) -> bool` | 移除行 |
| | clearLines | `(int64_t id) -> bool` | 清空行 |
| | getLineCount | `(int64_t id) -> int` | 行数 |
| 颜色 | setColor | `(int64_t id, r, g, b, a: float) -> bool` | 整体纯色 |
| | setLineColor | `(int64_t id, int line, r, g, b, a: float) -> bool` | 行纯色 |
| | setLineGradient | `(int64_t id, int line, r1,g1,b1, r2,g2,b2: float) -> bool` | 行双色渐变 |
| | setLineRainbow | `(int64_t id, int line, float speed) -> bool` | 行彩虹 |
| 动画 | setLineScroll | `(int64_t id, int line, int direction, float speed) -> bool` | 滚动：0=无 1=左 2=右 |
| | setVerticalAnimation | `(int64_t id, int type, float speed, float range) -> bool` | 垂直：0=无 1=弹跳 2=滚动 |
| | setLineSpacing | `(int64_t id, float spacing) -> bool` | 行距 |
| 位置 | setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | 跟随玩家 |
| | clearFollowPlayer | `(int64_t id) -> bool` | 取消跟随 |
| | setDimension | `(int64_t id, int dimId) -> bool` | 迁移维度：已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁） |
| 显示 | draw / drawToDimension / drawToPlayer / remove | 同形状语义 | |
| | refresh | `(int64_t id) -> bool` | 重解析变量并原地重发 |
| 驱动 | tick | `(float deltaTime) -> void` | 动画推进（滚动/跟随） |

内置变量（PAPI 翻译后兜底解析）：`{time}` `{online}` `{tps}` `{player}`。
外部占位符 `%name%` / `{name}` 经 LseBridge 调 `MeowPAPI::translateString(WithPlayer)`（可选，缺席原样保留）。

### 1.4 IItemDetail（物品详情）

```cpp
// 在 (x,y,z) 显示 "本地化物品名 xN"（自动翻译; count<=1 无数量后缀）
// customText 非空则完全替代自动文本（支持 § 颜色码与 {变量}）
// 返回悬浮字 ID（可用 holograms() 继续精修）
int64_t show(int dimId, float x, float y, float z,
             std::string const& itemId, int aux, int count,
             std::string const& customText = "");
bool hide(int64_t id);
```

物品名解析链：`ItemRegistry（HashedString）→ ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → 兜底原样显示 ID（不阻断）。

### 1.5 IItemDisplay（FMBE 物品悬浮显示，1.6.0）

配置驱动的物品/方块悬浮展示（FMBE 狐狸+发包）。全部变换字段为**常量数字或 Molang 表达式字符串**；持久化由消费者负责（库只管渲染）。

```cpp
struct ItemDisplayConfig {
    std::string item{"minecraft:diamond"}; // 物品标识符
    int         itemAux{0};                // 附加值
    float       x{0}, y{64}, z{0};         // 世界坐标
    int         dimension{0};
    std::string offsetX{"0"}, offsetY{"-4"}, offsetZ{"0"};           // v.xpos/ypos/zpos 模型单位平移
    std::string baseOffsetX/Y/Z{"0"};                                 // v.xbasepos... 渲染像素偏移
    std::string rotX{"180"}, rotY{"0"}, rotZ{"180"};                 // 三轴旋转（度）
    std::string scale{"0.375"};                                       // 缩放（方块模式建议 0.5）
    std::string extendScale{"1"}, extendRotX{"-90"}, extendRotY{"0"}; // 方块模式二段变换
    int    mode{0};                        // 0=auto 1=item 2=block
    double viewDistance{64.0};             // <=0 无限制
    bool   enabled{true};
    std::string itemNbt{};                 // 物品附加数据（SNBT; 自定义名称等）
    bool   itemGlint{false};               // 附魔光效（BDS 原生路径注入 1 级锋利）
    float  hitboxWidth{0.0f};              // AABB 判定体积宽（R53; 0 = 无判定体积, 1.17.0）
    float  hitboxHeight{0.0f};             // AABB 判定体积高（R54; 0 = 无判定体积, 1.17.0）
};
```

| 方法 | 签名 | 说明 |
|------|------|------|
| create | `(ItemDisplayConfig const&) -> int64_t` | 创建（<0 失败） |
| destroy / destroyAll | `(int64_t) -> bool` / `() -> void` | 销毁 |
| exists | `(int64_t) -> bool` | 存在性 |
| get | `(int64_t, ItemDisplayConfig&) -> bool` | 拷贝输出当前配置 |
| setItem | `(int64_t, std::string const&, int) -> bool` | 换物品 |
| setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 仅改坐标 |
| setOffset / setBaseOffset | `(int64_t, ox, oy, oz: string) -> bool` | 平移（Molang） |
| setRotation | `(int64_t, rx, ry, rz: string) -> bool` | 三轴旋转（Molang） |
| setScale | `(int64_t, scale: string) -> bool` | 缩放（Molang） |
| setExtend | `(int64_t, scale, rx, ry: string) -> bool` | 方块二段变换 |
| setMode / setEnabled / setViewDistance | — | 行为 |
| rotateY | `(int64_t, float) -> bool` | 偏航叠加增量（常量/表达式自适应） |
| getAllIds | `() -> std::vector<int64_t>` | 全部 ID |
| createRandom | `(ItemDisplayConfig const&) -> int64_t` | 随机段 `[0x10000000, 0x7FFFFFFF)` 自动生成不重复 ID; 成功返回生成的 ID, <0 失败 |
| createWithId | `(ItemDisplayConfig const&, int64_t desiredId) -> int64_t` | 指定 ID 创建（持久化恢复用）; desiredId<=0 或已被占用返回 -2 |
| isIdUsed | `(int64_t) -> bool` | 查询 ID 是否在用 |
| scaleBy | `(int64_t, double factor) -> bool` | 相对缩放：现有 scale × factor; 常量直接相乘, 表达式包裹 `(expr)*factor`; factor<=0 返回 false |
| setItemWithNbt | `(int64_t, std::string const& item, int aux, std::string const& nbt) -> bool` | 换物品（带附加数据）：nbt 为 SNBT; 空串 = 清除; 解析失败按无 NBT 处理并告警 |
| setGlint | `(int64_t, bool on) -> bool` | 附魔光效开关：开 = BDS 原生 `saveEnchantsToUserData` 注入 1 级锋利; 幂等 |
| follow | `(int64_t, std::string const& playerName, float offX, float offY, float offZ) -> bool` | 1.17.0 展示跟随玩家：每 tick 同步目标实时坐标（含跨维度自动 respawn），对已见玩家发 MoveActorAbsolute（非 teleport，客户端插值）平滑位移，全程无 respawn；玩家下线自动解除（方块原地保留）；`setPosition` 手动设位解除跟随 |
| unfollow | `(int64_t) -> bool` | 1.17.0 解除跟随；无跟随关系返回 true |
| setHitbox | `(int64_t, float width, float height) -> bool` | 1.17.0 AABB 判定体积：对已见玩家广播 SetActorData(R53/R54)，即时无 respawn；数值持久入配置（respawn 自动按当前值发包）；0/0 恢复不可命中 |

可见性由库内 Level tick hook 自动同步（每 20 tick：同维度 + 可见距离内玩家自动生成/移除；玩家断线自动清理）；属性变更即时生效（对已见玩家原子 despawn→respawn）。`setMode` / `setViewDistance` 幂等（同值不标脏不 respawn）。

无感创建（`createSeamless`）、单次跳变缩放（`scaleTo`）与可见白名单目前仅 LSE 导出（§2.5），未上 C++ 接口。渐变动画由消费者基于 `scaleTo` 步进驱动（如 5 步 easeOutCubic）。

### 1.6 ICustomEntity（自定义实体，1.10.0）

AddActorPacket 直发客户端生成**纯视觉实体**，不占服务端实体系统。适合 NPC 壳 / 装饰生物 / 盔甲架布景等（无碰撞、无服务端逻辑；交互经 ghost 事件路由，见 1.7）。

```cpp
struct CustomEntityEquipment {
    std::string name;    // item name（空 = 空槽位）
    int         aux{0};
    std::string nbt{};   // SNBT 形式, 空 = 无附加 NBT
};

struct CustomEntityConfig {
    std::string identifier{"minecraft:armor_stand"}; // 实体类型标识符（短名自动补 minecraft:）
    float       x{0}, y{64}, z{0};                   // 世界坐标
    int         dimension{0};
    float       yaw{0}, pitch{0};                    // 朝向（度）
    std::string nametag{};                           // 头顶名字（支持 § 颜色码; 空 = 无）
    bool        nametagAlwaysShow{false};
    float       scale{1.0f};                         // 客户端有效域 0.0625~10, 自动钳制
    int         variant{0};                          // 变种（皮肤/亚种）
    int         markVariant{0};                      // 二级变种
    int         colorIndex{0};                       // 颜色索引（羊/项圈等染色实体）
    std::int64_t flags{0};                           // 原始 flags 位掩码（0x01=着火 0x20=隐身 等）
    bool        invisible{false};                    // 隐身便捷开关（发包时合成 0x20）
    double      viewDistance{64.0};                  // <=0 无限制
    bool        enabled{true};
    int         pose{0};                             // PoseIndex 0..13（0=Standing 3..13 坐姿/睡姿/跳舞等）
    CustomEntityEquipment equipment[6];              // 槽位: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet
    std::string ridePlayerName{};                    // 骑到指定玩家头上（SetActorLinkPacket; 须在线）
    int64_t     rideEntityId{0};                     // 骑到另一自定义实体上（对方库内 id 为载具）
};
```

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create / createRandom / createWithId | `(CustomEntityConfig const&[, int64_t desiredId]) -> int64_t` | 同 itemDisplay 语义（随机段 `[0x10000000, 0x7FFFFFFF)`; desiredId<=0 或占用返回 -2） |
| | destroy / destroyAll / exists / isIdUsed / getAllIds / get | — | 同 itemDisplay 语义 |
| 属性 | setIdentifier | `(int64_t, std::string const&) -> bool` | 换实体类型 |
| | setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 仅改坐标 |
| | setRotation | `(int64_t, float yaw, float pitch) -> bool` | 朝向（度） |
| | setNametag | `(int64_t, std::string const&) -> bool` | 空串清除 |
| | setScale | `(int64_t, float) -> bool` | <=0 拒绝 |
| | setVariant / setMarkVariant / setColorIndex | `(int64_t, int) -> bool` | 变种/染色 |
| | setFlags / setInvisible / setEnabled / setViewDistance | — | 行为 |
| | setPose | `(int64_t, int pose) -> bool` | PoseIndex 0..13 |
| | setEquipmentSlot | `(int64_t, int slot, std::string const& name, int aux, std::string const& nbt) -> bool` | 六槽装备; name 空清空槽位; nbt 为 SNBT |
| | findNearest | `(float x, float y, float z, int dim, double maxDist) -> int64_t` | 最近查找; 无匹配 -1 |
| 缩放 | scaleBy | `(int64_t, double factor) -> bool` | 相对缩放：现有 scale × factor，结果自动钳制 0.0625~10; factor<=0 返回 false |
| 可见性 | setVisiblePlayers | `(int64_t, std::vector<std::string> const&) -> bool` | 白名单（按 realName 匹配）; 空列表 = 清除限制 |
| | setVisiblePlayer / clearVisiblePlayers | `(int64_t, std::string const&) / (int64_t) -> bool` | 标量版 / 清除（恢复全员可见） |
| 逐客户端朝向（1.20.0, 仅 C++ 接口; LSE 导出暂未覆盖） | setPlayerRotation | `(int64_t, std::string const& playerName, float yaw, float pitch) -> bool` | **1.20.0**: 覆盖指定玩家收到的该实体朝向（出生包 AddActor 与增量包都按覆盖值下发）; 未覆盖的玩家仍用 config 朝向; 玩家离线/未见过该实体返回 false; 变更走轻脏增量（无闪烁）, 下一 tick 生效 |
| | clearPlayerRotation | `(int64_t, std::string const& playerName) -> bool` | 清除单个玩家的朝向覆盖 |
| | clearPlayerRotations | `(int64_t) -> bool` | 清除该实体全部玩家的朝向覆盖 |
| 诊断 | getDebugInfo | `(int64_t) -> std::string` | 运行态摘要; 未找到返回 `not_found` |
| 骑乘 | setRidePlayer | `(int64_t, std::string const& playerName) -> bool` | 骑到玩家头上（空名清除; 须在线） |
| | setRideEntity | `(int64_t, int64_t vehicleEntityId) -> bool` | 骑到另一自定义实体上; 0 清除 |
| | clearRide | `(int64_t) -> bool` | 解除骑乘链接 |
| 动画 | playAnimation | `(int64_t, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 播放原版动画（AnimateEntityPacket, 如 `animation.humanoid.base_pose`）; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止 |

属性变更经 tick 脏刷新合并为单次 respawn（无闪烁串台）；可见性由库内 Level tick hook 自动同步（同 itemDisplay 模式）。

### 1.7 GhostInteractEvent（ghost 交互事件，1.12.0）

客户端点击/攻击"协议上存在"的实体时，事件经 InventoryTransactionPacket 内嵌的 ItemUseOnActor 事务到达（协议 944 起 InteractPacket 不再承载实体交互）；库 hook 收包后将目标 runtimeId 反查回库内 id 并派发，实现**可点击 NPC / 全息菜单**。

```cpp
struct GhostInteractEvent {
    std::string playerName;   // 点击者（realName）
    int         action{0};    // 1=右键交互 2=左键攻击（见下方说明）
    std::string domain;       // "entity" / "itemDisplay" / "npc"
    int64_t     id{-1};       // 对应域的库内 id
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};
```

- `action`：1=右键交互 2=左键攻击（ItemUseOnActor 的 Interact/ItemInteract 映射为 1，Attack 映射为 2；与旧 InteractPacket 语义对齐，消费端无需区分来源）
- 消费方式二选一（可并存）：C++ 推送 `setGhostInteractListener`；LSE 轮询 `ghostPollInteractions`（取走并清空队列，队列上限 256 条、满时丢最旧）
- 恒调 origin，不改变 BDS 对未知 runtimeId 交互包的原版行为；无监听且无人轮询时仅做 runtimeId 段判别，近零开销

### 1.8 IParticleShape（通用协议层粒子形状，1.14.0）

点/线/矩形环/填充面/长方体框/六面/多面体 + 平移/平滑移动/旋转/自旋/缩放/跟随。批量并发发送：逐玩家 vanilla `SpawnParticleEffectPacket` 经 `NetworkSystem` 入队，BDS tick flush 自动聚合压缩单 Batch 数据报（与原版粒子广播同路径）；采样/视距裁剪/周期重发由库内 tick 自驱动，消费者只管创建与控制。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 创建 | createPoint | `(std::string const& owner, int dimId, float x, float y, float z, std::string const& effect, int intervalTicks, int lifetimeTicks) -> int64_t` | 单点 |
| | createLine | `(owner, dimId, x1,y1,z1, x2,y2,z2: float, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 线段 |
| | createRect | `(owner, dimId, float cx, cy, cz, w, h, int axis, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 矩形环线（axis: 0=XY 1=YZ 2=XZ） |
| | createPlane | 同 createRect | 填充平面网格 |
| | createBox | `(owner, dimId, cx, cy, cz, hx, hy, hz: float, step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 长方体 12 边线框（h* 为半尺寸） |
| | createBoxFaces | 同 createBox | 六面填充 |
| | createPoly | `(owner, dimId, std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 多面体（verts=(x,y,z)×N；edges=(i,j)×M；锚点=质心） |
| 控制 | setPos | `(int64_t id, float x, float y, float z) -> bool` | 平移锚点（=粒子移动；同时解除跟随） |
| | moveBy | `(int64_t id, float dx, float dy, float dz) -> bool` | 相对平移 |
| | moveTo | `(int64_t id, float x, float y, float z, int durationTicks) -> bool` | 平滑点对点移动（easeOutCubic；解除跟随；duration<=0 立即到达） |
| | setRot | `(int64_t id, float rx, float ry, float rz) -> bool` | 欧拉角（度，ZYX 序，绕锚点） |
| | spin | `(int64_t id, float sx, float sy, float sz) -> bool` | 自旋（度/tick；0,0,0 停止） |
| | setScale | `(int64_t id, float scale) -> bool` | 各向同性缩放 |
| | follow | `(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) -> bool` | 锚点跟随玩家+偏移（每 tick 更新，跨维度） |
| | unfollow | `(int64_t id) -> bool` | 解除跟随 |
| 渲染 | setEffect | `(int64_t id, std::string const& effect) -> bool` | 粒子效果名 |
| | setVisiblePlayers | `(int64_t id, std::vector<std::string> const& playerUuids) -> bool` | 白名单（UUID；空=维度全员） |
| | clearVisiblePlayers | `(int64_t id) -> bool` | 清除白名单 |
| | setInterval | `(int64_t id, int ticks) -> bool` | 周期整批重发间隔 |
| | setViewDistance | `(int64_t id, int blocks) -> bool` | 逐玩家 3D 裁剪；0=不裁剪 |
| | setLifetime | `(int64_t id, int ticks) -> bool` | 从现在起；0=永久 |
| 生命周期 | destroy / destroyAll / exists / getAllIds | 同其他域 | — |
| 诊断 | getDebugInfo | `(int64_t id) const -> std::string` | 运行态摘要（找不到返回 "not_found"） |

约定：

- `intervalTicks` = 周期整批重发间隔（粒子瞬态，靠重发维持常驻视觉）；`lifetimeTicks` 0 = 永久
- 采样点在构造时换算为局部坐标（锚点=形状几何参考点），平移/旋转/缩放作用于局部点，下次发射自动生效
- 与 LSE `particle*`（§2.8）共用同一 Manager，id 空间互通

C++ 消费示例：

```cpp
auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();
// 玩家脚下 16×16 粒子墙（XZ 平面填充, 跟随玩家）
auto id = ps.createPlane("MeowLand", 0, 0, 64, 0, 16, 16, 2, 1.0f,
                         "minecraft:endrod", 4, 0);
if (id > 0) {
    ps.follow(id, playerUuid, 0, -2, 0);
    ps.setVisiblePlayers(id, {playerUuid}); // 仅本人可见
}
```

### 1.9 IPlayerNpc（假玩家 NPC，1.16.0）

> **注意：NPC 皮肤暂无效（未解决）**。本节接口与数据链路均正常工作——皮肤注册（PNG / 在线采集 / 目录导入）、`getSkinBlob` / `registerSkinFromBlob` 导出恢复、NPC 创建/移动/朝向/缩放/视距/显隐、点击交互都可正常调用并生效；唯一失效的是最终显示：客户端不渲染所设置的皮肤，NPC 外观回退为默认模型。

纯协议假玩家：`PlayerListPacket(Add, 携带皮肤) → AddPlayerPacket → [20 tick] PlayerListPacket(Remove)`（假玩家短暂出现在 Tab 后移除，实体因皮肤已缓存持续渲染）。不占服务端实体系统；点击交互经 ghost 管线 `domain="npc"` 派发（§1.7）。

皮肤注册表全局共享：PNG 文件注册（GDI+ 解码 64×64/128×128，自定义 geometry/armSize）、目录批量导入（一个子文件夹 = PNG + 可选 `.json` 几何模型，缺省 = 标准玩家模型）或从在线玩家采集（`Player::mSkin → SerializedSkinImpl` 全字段拷贝：贴图/披风/动画贴图/几何/Persona 部件/染色 → 以 skinId 注册运行时快照，玩家之后换肤不影响）。解码/采集一次，多 NPC 复用零重复开销。**库不落盘**——持久化由消费方负责：注册/采集/导入成功后 `getSkinBlob` 导出全字段二进制快照存到自己的目录，重启时 `registerSkinFromBlob` 恢复，不依赖源 PNG/玩家在线。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 皮肤 | registerSkin | `(PlayerNpcSkin const& skin) -> bool` | PNG 注册（skinId 空 = 文件名; 重复覆盖） |
| | importSkins | `(std::string const& dirPath) -> int` | 目录批量导入（skinId = 子文件夹名; PNG 必需 + `.json` 几何可选; 返回导入数量, 目录无效 -1） |
| | captureSkin | `(std::string const& skinId, std::string const& playerName) -> bool` | 从在线玩家采集（不在线返回 false; 重复覆盖） |
| | getSkinBlob | `(std::string const& skinId, std::string& out) const -> bool` | 全字段序列化导出（消费方持久化用; 未注册 false） |
| | registerSkinFromBlob | `(std::string const& blob) -> bool` | blob 反序列化注册（与 getSkinBlob 配对; 格式非法 false） |
| | hasSkin / unregisterSkin / getSkinIds | — | unregister 有 NPC 引用时拒绝 |
| 生命周期 | create / createRandom / createWithId | `(PlayerNpcConfig const&) -> int64_t` | 失败: -1 常规 / -2 id 占用 / -3 皮肤未注册; 持久化由消费者负责 |
| | destroy / destroyAll / exists / get / isIdUsed / getAllIds | — | 同其他域 |
| 属性 | setPosition | `(int64_t id, float x, float y, float z, int dim) -> bool` | dim<0 仅改坐标 |
| | setRotation / setNametag / setSkin / setViewDistance / setEnabled | — | setSkin 未注册返回 false; 变更经 tick 脏刷新合并为单次 respawn |
| 朝向（1.20.0, 仅 C++ 接口; LSE 导出暂未覆盖） | setRotationLight | `(int64_t id, float yaw) -> bool` | 轻量朝向: 只发 MoveActorAbsolute 增量（不重建实体/不重发皮肤, 无闪烁）, 适合每 tick 跟踪 |
| | setPlayerRotation | `(int64_t id, std::string const& playerName, float yaw) -> bool` | 覆盖指定玩家收到的朝向（出生包与增量包都按覆盖值）; 未覆盖玩家用 config 朝向 |
| | clearPlayerRotation / clearPlayerRotations | `(int64_t[, std::string const&]) -> bool` | 清除单个 / 全部玩家的朝向覆盖 |
| 可见性 | setVisiblePlayers / clearVisiblePlayers / setVisiblePlayer | — | 玩家名白名单（空 = 全员） |
| 诊断 | getDebugInfo | `(int64_t id) const -> std::string` | 运行态摘要 |

```cpp
struct PlayerNpcSkin {
    std::string pngPath;                                        // PNG 路径（64/128）
    std::string skinId;                                         // 空 = 用文件名
    std::string geometry{"geometry.humanoid.custom"};          // 模型（resourcePatch）
    std::string armSize{"wide"};                                // "wide" / "slim"
    std::string geometryData;                                   // 1.18.0 完整几何 JSON（可选, 提供时启用自定义模型）
};

struct PlayerNpcConfig {
    std::string name{"NPC"};        // 显示名（nametag 同步）
    std::string skinId{"default"};   // 必须已注册, 否则创建失败 -3
    float x{0}, y{64}, z{0};
    int   dimension{0};
    float yaw{0};
    double viewDistance{96.0};      // <=0 无限制; 滞回: 退出需超 viewDistance+4
    bool  enabled{true};
};
```

---

### 1.10 ITradeMenu（村民交易菜单，1.21.0；**1.22.0 起为纯展示**）

`IHologramLib::tradeMenus()`。**手写包**打开交易界面：界面完全由本库构造的 `UpdateTradePacket` 驱动，载荷与 BDS 26.40 原生村民交易逐字节一致（`tests/check-trade-packet.bat` 整包对拍、`check-trade-offers.bat` 只对 Offers 段）。交易界面只靠这个包打开——原生流程里没有 `ContainerType=Trade` 的 `ContainerOpen`。

> **本域不做任何点击事件监听**：打开界面、摆出交易表就到此为止。客户端点了哪一条、往付费槽里放了什么，库一律不读、不拦、不回传（`26.40.3` 里短暂存在过的 `TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` 与六个监听方法已在 `26.40.4` 全部移除）。要"能点、点了有回调"的列表界面用 `IContainerMenu`（虚拟容器）——它的点击就是一次物品拾取，任何输入设备都会发包。

两条路径（`TradeMenuSpec::usePacketOffers`）：

- **`true`（默认，纯协议层）**：只发自建 `UpdateTrade`，服务端不放交易表。玩家往付费槽放东西的请求会被 BDS 拒掉、物品弹回，界面停在"不可成交"——纯展示正合适（与参考实现 GMLIB `ChestUI` 同路）。
- **`false`**：给载体实体装真实交易表并调 BDS 的 `openTrading`，**玩家是真的在交易**（物品真的消耗）。`carrierUniqueIdOverride != 0` 时直接以该真实实体为交易对象。

载体实体：交易界面需要 `EntityUniqueId`，因此打开时会在玩家身后 5 格生成一个**隐身、仅该玩家可见**的假村民（`minecraft:villager_v2`），关闭即删除。载体是异步送达客户端的，所以 `UpdateTrade` 与经验条元数据都在几 tick 后发送（背靠背发时客户端还不认识该实体，界面绑不上去）。经验条不在包内，由载体实体的 `TradeTier` / `MaxTradeTier`（恒 4）/ `TradeExperience` 元数据驱动。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | open | `(std::string const& playerName, TradeMenuSpec const&) -> int64_t` | 同一玩家重复调用先关旧菜单；失败 -1 |
| | update / close / closeAll / isOpen / getAllIds | — | `update` 重发交易表（真实表路径会重开界面） |
| 内容（1.22.0 追加） | addOffer | `(int64_t menuId, TradeMenuOffer const&) -> bool` | 追加一条并**就地重发**交易表（不重开界面、不等延迟） |
| | setTier | `(int64_t menuId, int tier, int experience) -> bool` | 改显示栏值 / 经验条，同样就地重发 |

```cpp
struct TradeMenuOffer {
    TradeMenuItem buyA, buyB, sell;   // buyB.type 空 = 无第二付费项
    int  tier{kTradeTierNovice};      // 1 基: 1=新手 … 5=大师；高于 spec.tier 即显示为未解锁
    int  maxUses{16};
    int  traderExp{0};
    bool locked{false};               // true = 无论 spec.tier 多高都显示为未解锁
};

struct TradeMenuSpec {
    std::string tradeType{"entity.villager.butcher"}; // 自由翻译键
    int         tier{kTradeTierNovice};               // 1 基, 超出夹紧
    int         experience{0};                        // 经验条当前经验
    bool        usePacketOffers{true};                // 见上: 纯协议层展示 / 真实交易表
    std::int64_t carrierUniqueIdOverride{0};          // 非 0 = 用该真实实体（不放自建 offers）
    std::vector<TradeMenuOffer> offers;
    std::string carrierIdentifier{"minecraft:villager_v2"}; // 或 "minecraft:wandering_trader"
};
```

```cpp
// 载体实体类型（TradeMenuSpec 尾部追加）: 默认村民; 流浪商人 = "minecraft:wandering_trader"
// （界面外观/头图随类型变化; 两条路径都适用, 真实交易表路径装表方式相同）
spec.carrierIdentifier = "minecraft:wandering_trader";
```

### 1.11 INpcDialogue（NPC 对话框，1.21.0）

`IHologramLib::npcDialogs()`。`NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON）下发，`NpcRequestPacket` 回传（点了第几个按钮 / 玩家关闭）。

- 按钮表完全由服务端逐玩家生成：按权限 / tag / 计分板 / 任务状态决定按钮是否出现，就是在调用本接口前自行算好 `buttons`，不需要客户端配合。
- 多层级对话 = 点击回传带回 `sceneName` + `buttonIndex` / `actionId`，调用方据此发送下一层。
- 载体实体是**纯协议合成**的 `minecraft:npc`（不进 BDS 实体系统），位置在世界下方 `y=-66`——客户端看不到实体，但界面里的头像照常渲染（改用隐形标志位反而会让头像一起消失）。只发给该玩家；点按钮 / 关闭 / 离线时发 `RemoveActor` 删掉。
- `NpcDialogSpec::rawActionJson` 非空时原样作为 `mActionJSON` 下发（原版按钮结构无公开文档，用于在游戏内实测字段格式）；`carrierIdentifier` 必须是 NPC 家族，换成村民等原版实体不行（实测要求）。

```cpp
struct NpcDialogButton {
    std::string label;
    std::vector<std::string> commands; // 展示在按钮上; 合成 NPC 不由 BDS 代跑, 命令回传给调用方
    std::string actionId;              // 服务器侧标识, 点击回传原样带回（对接对话树/任务）
    int         mode{0};               // 0=普通按钮 1=关闭 2=打开
};

struct NpcDialogClickEvent {
    std::string playerName;
    int64_t     dialogId{-1};
    std::string sceneName;
    int         buttonIndex{-1};  // -1 = 非按钮事件
    std::string actionId;
    bool        closed{false};
    std::vector<std::string> commands; // 被点击按钮上挂的命令（按 buttonIndex 回填）
};
```

### 1.12 IContainerMenu（虚拟容器 / 列表界面，1.21.0）

`IHologramLib::containerMenus()`。方案逐条复刻参考实现 GMLIB 的 `ChestUI`：

1. `UpdateBlockPacket` 在玩家头上摆一个**客户端侧箱子方块**（只发给该玩家，服务端世界里没有它）
2. `BlockActorDataPacket` 摆该方块的方块实体 NBT：`id=Chest` / `CustomName=标题` / `x,y,z` / `pairx,pairz,pairlead`（大箱子）/ `Items=[条目物品]` —— 条目物品走这份 NBT，**不是**逐格 `InventorySlot`
3. 等 10 tick 发 `ContainerOpen`（`ContainerType::Container(0)` + 方块坐标 + 目标实体 `-1`）
**按槽动态刷新用 `setItem(menuId, slot, item)`**：只发一条 `InventorySlotPacket`（真实箱子同步内容用的同一种包），
客户端就地换掉那一格——不重发方块实体、不重发 `ContainerOpen`，**无延迟、无闪烁**；`item.type` 为空即清空该槽。
改整页内容才用 `update(menuId, spec)`。

**翻页/进子菜单/返回用 `update(menuId, spec)`**: 复用同一个载体方块, 不拆界面、不重摆方块、不等待,
只重发方块实体 NBT（新条目/标题）+ `ContainerOpen` 让客户端重读。走 `open()` 重开每次都要付一次打开延迟
（它会先关旧菜单、恢复真方块, 再重摆、再等 `openDelayTicks`）。

4. 玩家点击 → 客户端发 `ItemStackRequest(147)` → 槽位命中本容器 → 回调（槽位号 = 条目下标）。命中容器的动作**只回传、不拦**：服务端没有这个容器，放行后 BDS 自己就会失败、客户端把预测撤回（物品闪一下回原位，无错误提示），回调照常收到——与参考实现 GMLIB 一致。实测教训: 若库自己代答一条 `ItemStackNetResult` 3（请求不允许），客户端会**弹错误提示**
5. 关闭 → `ContainerClosePacket` → 把真方块改回去 → 回调 `closed = true`

**服务端根本没有这个容器**：物品只是"摆在那里"，玩家拿走/移动都不会真的改变任何东西（天然只读）。适合任务列表、成就列表、商店预览这类"只展示 + 点击回调"的界面。

大小容器：`rows = 3` → 单箱子 **27 格**（方块实体不带配对键）；`rows = 6` → 大箱子 **54 格**（相邻两个箱子方块 + `pairx`/`pairz`/`pairlead` 配对，前 27 格进 lead 半、后 27 格进副半，各半的 `Slot` 都是 0..26）。方块位置取玩家脚上方 5 格并优先挑空气位（免得盖掉真实方块），关闭时按该位置真方块的网络 id 改回。容器 id 用 101..199 显示区间，与 BDS 真实容器的动态 id（1..100）不冲突。

```cpp
struct ContainerMenuSpec {
    int rows{3};                       // 3 = 小箱子 27 格, 6 = 大箱子 54 格
    std::vector<ContainerMenuItem> items; // 下标即槽位号（大箱子 0..53）
    bool useMinecart{false};           // false(默认) = GMLIB 方块方案; true = 旧矿车路径（客户端不认, 仅对比）
    std::string title{"虚拟容器"};      // 箱子界面标题（方块实体的 CustomName）
    int openDelayTicks{10};            // 摆方块+方块实体后等多少 tick 再发 ContainerOpen
                                       // （GMLIB: 10 再等 4 补格 ≈700ms; 本库条目在方块实体 NBT 里
                                       // 只需要 10 ≈500ms）。调小可更快, 下限要实机扫。只影响"打开"那次
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};       // 被点击的槽位（= items 的下标）
    bool        closed{false};  // true = 界面被关闭（此时 slot = -1）
};
```

### 1.13 感知域 / 千人千面 / 逐玩家变量全息（1.21.0）

**感知域 `IHologramLib::playerSensing()`**：挂钩 `PlayerAuthInputPacket`（每玩家每 tick）逐包捕获 `InputMode`。
`inputDeviceOf(playerName)` 返回 `ClientInputDevice`（`Unknown/KeyboardMouse/Touch/Gamepad/MotionController`），
`isTouch(playerName)` 便捷判断。玩家离线即清；未上报 = `Unknown`。典型用途：按设备路由 UI
（触屏走容器列表，键鼠/手柄走交易界面 —— 触屏在纯协议层交易里走不到成交是实测结论）。

**千人千面** `ICustomEntity` 追加：`setPlayerNametag(id, player, text)`（空 = 清覆盖）、
`setPlayerScale(id, player, scale)`（<=0 = 清覆盖）、`setPlayerEquipmentSlot(id, player, slot, name, aux, nbt)`
（slot 0..5，name 空 = 该槽回退 config，即时单发无闪烁）、`clearPlayerAppearance(id, player)`。
机制与 `setPlayerRotation` 相同：覆盖按玩家 uuid 保存，出生包与增量包都按覆盖值下发，未覆盖玩家用 config 值。

**逐玩家变量全息**：全息文本含 `{var}` 占位符且绘制目标不是单玩家时，库自动切换为**逐观看者形状**
——每个在线玩家收到按自己名字解析的独立文本（`{player}` 对每人正确）；离线/离开维度的观看者形状
就地销毁。无变量时仍是共享单形状（性能不变）。文本动画（滚动/弹跳）由库内 `ServerLevelTickEvent`
监听自驱，无需消费方调用 tick。

> 曾短暂实现过**逐客户端音效**与**短命飘字**，经讨论已整体移除（见 README 更新日志）。

---

### 1.14 IFakeInventory（背包虚容器，1.23.0）

`IHologramLib::fakeInventories()`。**协议层改写客户端看到的玩家背包**：一条 `InventoryContentPacket(49)`
（`ContainerId = Inventory(0)`、0..35 号槽位）把整份内容换成调用方给的那一份；单格改动走 `InventorySlotPacket(50)`。
服务端背包一个字都不动 —— 物品是"看起来有"。

> **物品序列化交给 BDS**：物品描述符的 User Data 用的是 u16 名字长度 / 4 字节整数的那套 NBT 方言，
> 与 sculk `CompoundTag` 的 varint 方言不同；手写会写错（实测把客户端卡死）。所以本域用 BDS 的
> `InventoryContentPacket` / `InventorySlotPacket` 构造包体（容器名字节 0 + storage 空描述符, 与抓包一致）。

参考实现 GMLIB 的 `ChestUI` 填玩家物品栏用的就是同一条路（它逐格写 `InventorySlot`，容器 id 同样是
`Inventory(0)` + `InventoryContainer(29)`）；本域的整份下发用一条 `InventoryContent` 搞定。

**功能项与虚拟容器一致**：玩家点自己背包里的伪造物品 → 回调报槽位号，物品不会真的被拿走 ——
客户端按伪造内容发出请求，服务端真实槽位对不上 → BDS 判失败 → 客户端把预测撤回（与虚拟容器同一套
表现，库不需要代答）；库随后把那一格重发一次，让伪造内容不被这次回滚冲掉。

**与交易菜单 / 虚拟容器共存**：打开交易界面或箱子界面时 BDS 会重发玩家背包内容（伪造内容被覆盖），
所以本域默认按 `refreshIntervalTicks` 周期重发（默认 20 tick = 1s），也可随时 `refresh()`。
界面上看到的背包区域因此始终是伪造内容，点击照常回调。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 下发 | apply | `(std::string const& playerName, FakeInventorySpec const&) -> bool` | 整份伪造内容（覆盖该玩家此前的）; 玩家不在线 false |
| | setSlot | `(std::string const& playerName, int slot, ContainerMenuItem const&) -> bool` | 单格改动（一条 `InventorySlot`，无延迟）; 未 apply 过返回 false |
| | refresh | `(std::string const& playerName) -> bool` | 立即重发当前伪造内容 |
| | clear / clearAll | `(std::string const& playerName) -> bool` / `() -> void` | 撤销伪造 + 停周期重发，并把**真实**背包重发一遍 |
| 查询 | isActive / getActivePlayers | `(std::string const&) -> bool` / `() -> std::vector<std::string>` | |
| 回传 | addClickListener / removeClickListener | `(std::function<void(FakeInventoryClickEvent const&)>) -> uint64_t` | 多播；点伪造成非空的格子才回调 |

```cpp
inline constexpr int kFakeInventorySlots = 36; // 0..8 快捷栏, 9..35 主背包

struct FakeInventorySpec {
    std::vector<ContainerMenuItem> items;   // 下标即槽位; type 空 = 该格留空
    int refreshIntervalTicks{20};           // 0 = 不周期重发
};

struct FakeInventoryClickEvent {
    std::string playerName;
    int         slot{-1};                   // 被点击的伪造槽位
};
```

**边界（实测前先写清楚，免得误用）**：

- 伪造只影响客户端显示。玩家"真正能用/能吃"的仍是服务端真实物品。
- 若某格真实物品与伪造物品恰好一致，那一次操作会被服务端当真执行 —— 想让某格纯展示，别把它伪造成与真实物品相同的东西。
- 该格真实物品因为别的原因改变（捡东西 / 用物品 / 别的插件改背包）时，周期重发会把伪造内容重新盖回去。
- 护甲（`ArmorContainer` 6）/ 副手（`OffhandContainer` 34）不在本域范围内，它们各有独立的容器枚举。

### 1.15 ISulfurDisplay（硫磺立方体展示，1.23.0）

`IHologramLib::sulfurDisplays()`。第二种摆放方式（与 `IItemDisplay` 的"狐狸 + 物品"并列）：
生成一只 `minecraft:sulfur_cube`，把要展示的方块/物品**装进它的主手** —— 行为包里立方体就是靠
`slot.weapon.mainhand` + `behavior.equip_item` 拿着"吞下去"的方块（客户端按装备渲染）。

外观档位 `minecraft:sulfur_cube_archetype` 是行为包里 `client_sync: true` 的 enum 属性，走**手写的
`ChangeMobProperty`(182)** 下发（26.40 线格式: Actor Id(int64 压缩) → Property Name(string) → Bool →
String → Int32(压缩) → Float）。`AddActor` 的 `PropertySyncData` 只装得下按索引的 int/float，装不下 enum，
所以属性必须在实体已被客户端认识之后发 —— 库里在 spawn 后自动补发，respawn 后重放。

本域**完全委托给 `ICustomEntity`**（立方体就是一只自定义实体），缩放 / 视距 / 可见玩家白名单 / 逐客户端
朝向等能力直接继承；新增的协议内容只有 `ChangeMobProperty`。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生成 | create | `(SulfurDisplaySpec const&) -> int64_t` | 返回展示 id（<0 = 失败） |
| **吞方块** | setBlock | `(int64_t id, ContainerMenuItem const&) -> bool` | 换"吞下去"的方块（改主手装备 → 重建一次实体） |
| 外观 | setArchetype | `(int64_t id, std::string const&) -> bool` | 换外观档位（`ChangeMobProperty`；空串 = 清掉属性；**不重建实体**） |
| **隐身** | setInvisible | `(int64_t id, bool) -> bool` | 隐身开关（spec 上的 `invisible` 参数同义） |
| | setScale | `(int64_t id, float) -> bool` | 缩放（0.0625~10） |
| 生命周期 | destroy / destroyAll / exists / getAllIds / entityIdOf | — | `entityIdOf` 返回背后的自定义实体 id（诊断用） |

```cpp
struct SulfurDisplaySpec {
    float       x{0}, y{64}, z{0};
    int         dim{0};
    ContainerMenuItem block;                    // **吞下去的方块**（核心参数; 走主手装备）
    std::string archetype{"regular"};           // none/regular/bouncy/sticky/hot/explosive/light/...
    int         variant{2};                     // 1=小 / 2=中（含方块那一档）
    bool        invisible{true};                // **隐身参数**（**默认开**: 实测隐身时方块照常渲染 → 只留内容）
    float       scale{1.0f};
    double      viewDistance{0.0};
    std::vector<std::string> visiblePlayers;    // 空 = 全员可见
};
```

**隐身默认开（实测）**：26.40 本机客户端上确认 —— 立方体隐身时，主手里"吞下去的方块"照常渲染，
所以 `invisible` 默认 `true`，默认观感就是"一个方块浮在那里"（立方体本体不可见）；要看立方体本体就设 `false`。
（NPC 载体的头像是会被隐身一起抹掉的，两个实体行为不同，不要套用。）

**"吞生物"没有做**：协议层实体没有 AI，真正吞并/消化做不到；试过让另一个实体骑在立方体上做近似，
骑乘位置与碰撞都调不出"被吞进去"的观感（实测不成立），已整体移除 —— 本域只做方块与隐身。

**另外**: `ICustomEntity::setMobProperty(id, name, value)` / `clearMobProperties(id)` 同批开放 ——
凡是 `client_sync` 的实体属性都能按名下发（硫磺立方体的档位就是这么实现的）。

## 2. LSE 接口（ll.import 统一命名空间）

LegacyRemoteCall（lrca）在场时自动导出。**单命名空间 `HologramLib`**，前缀区分能力域：

| 前缀域 | 能力 | 对应 C++ 接口 | 函数数 |
|--------|------|---------------|--------|
| `shape*` | 形状渲染 | IShapeDrawer | 36 |
| `holo*` | 悬浮字全息 | IHologramText | 26 |
| `gradient*` | 渐变线 | GradientLineManager | 11 |
| `itemDetail*` | 物品详情 | IItemDetail | 2 |
| `itemDisplay*` | FMBE 物品悬浮 | IItemDisplay | 33 |
| `entity*` | 自定义实体 | ICustomEntity | 33 |
| `ghost*` | 交互事件轮询 | IHologramLib | 2 |
| `particle*` | 通用粒子形状系统 | ParticleShapeManager | 22 |
| `playerNpc*` | 假玩家 NPC（含皮肤注册/采集/目录导入） | IPlayerNpc | 25 |

缺席时安全降级（`ll.import` 得 null）。

类型记法：`f`=浮点 `i`=整数 `s`=字符串 `b`=布尔 `[i]`=int64 数组。除注明外 id 均为 int64。

### 2.1 shape*（形状，36 函数）

**创建**

| 函数 | 签名 |
|------|------|
| shapeCreateText | `(x: f, y: f, z: f, text: s) -> i` |
| shapeCreateLine | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| shapeCreateBox | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| shapeCreateCircle | `(x: f, y: f, z: f, scale: f) -> i` |
| shapeCreateSphere | `(x: f, y: f, z: f, scale: f) -> i` |
| shapeCreateArrow | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |

**属性**

| 函数 | 签名 |
|------|------|
| shapeSetText | `(id, text: s) -> b` |
| shapeGetText | `(id) -> s` |
| shapeSetLocation | `(id, x: f, y: f, z: f) -> b` |
| shapeGetLocation | `(id) -> [f,f,f]` |
| shapeSetColor | `(id, r: f, g: f, b: f, a: f) -> b` |
| shapeGetColor | `(id) -> [f,f,f,f]` |
| shapeSetScale | `(id, scale: f) -> b` |
| shapeSetDuration | `(id, seconds: f) -> b` |
| shapeSetDimension | `(id, dimId: i) -> b` |
| shapeSetRotation | `(id, pitch: f, yaw: f, roll: f) -> b` |
| shapeClearRotation | `(id) -> b` |
| shapeGetRotation | `(id) -> [f,f,f]` |
| shapeGetShapeType | `(id) -> i`（0..5，见 ShapeType） |

**显示**

| 函数 | 签名 |
|------|------|
| shapeDraw | `(id) -> b` |
| shapeDrawToPlayer | `(id, playerName: s) -> b` |
| shapeDrawToDimension | `(id, dimId: i) -> b` |
| shapeDrawBatch | `(ids: [i]) -> b` |
| shapeRemove | `(id) -> b` |
| shapeRemoveToPlayer | `(id, playerName: s) -> b` |
| shapeRemoveToDimension | `(id, dimId: i) -> b` |
| shapeUpdate | `(id) -> b` |
| shapeUpdateToPlayer | `(id, playerName: s) -> b` |
| shapeUpdateToDimension | `(id, dimId: i) -> b` |

**生命周期与查询**

| 函数 | 签名 |
|------|------|
| shapeDestroy | `(id) -> b` |
| shapeDestroyAll | `() -> nil` |
| shapeDestroyBatch | `(ids: [i]) -> b` |
| shapeFindTextByLocation | `(x: f, y: f, z: f, radius: f) -> [i]` |
| shapeFindTextByLocationAndContent | `(x: f, y: f, z: f, radius: f, text: s) -> i` |
| shapeGetAllShapeIds | `() -> [i]` |
| shapeExists | `(id) -> b` |

### 2.2 holo*（悬浮字，26 函数）

| 函数 | 签名 |
|------|------|
| holoCreate | `(x: f, y: f, z: f) -> i` |
| holoDestroy | `(id) -> b` |
| holoDestroyAll | `() -> nil` |
| holoAddLine | `(id, text: s) -> b` |
| holoSetLineText | `(id, lineIndex: i, text: s) -> b` |
| holoSetLineScale | `(id, lineIndex: i, scale: f) -> b` |
| holoRemoveLine | `(id, lineIndex: i) -> b` |
| holoClearLines | `(id) -> b` |
| holoGetLineCount | `(id) -> i` |
| holoSetColor | `(id, r,g,b,a: f) -> b` |
| holoSetLineColor | `(id, lineIndex: i, r,g,b,a: f) -> b` |
| holoSetLineGradient | `(id, lineIndex: i, r1,g1,b1,r2,g2,b2: f) -> b` |
| holoSetLineRainbow | `(id, lineIndex: i, speed: f) -> b` |
| holoSetLineScroll | `(id, lineIndex: i, direction: i, speed: f) -> b`（方向 0=无 1=左 2=右） |
| holoSetVerticalAnimation | `(id, type: i, speed: f, range: f) -> b`（0=无 1=弹跳 2=滚动） |
| holoSetLineSpacing | `(id, spacing: f) -> b` |
| holoSetLocation | `(id, x: f, y: f, z: f) -> b` |
| holoSetDimension | `(id, dimId: i) -> b`（迁移维度; 已绘制时原地重发, 无闪烁） |
| holoSetFollowPlayer | `(id, playerName: s, offsetY: f) -> b` |
| holoClearFollowPlayer | `(id) -> b` |
| holoTick | `(deltaTime: f) -> nil`（动画驱动） |
| holoDraw | `(id) -> b` |
| holoDrawToDimension | `(id, dimId: i) -> b` |
| holoDrawToPlayer | `(id, playerName: s) -> b` |
| holoRemove | `(id) -> b` |
| holoRefresh | `(id) -> b`（重解析变量并原地重发） |

### 2.3 gradient*（渐变线，11 函数）

| 函数 | 签名 |
|------|------|
| gradientCreate | `(x1,y1,z1: f, x2,y2,z2: f, segments: i) -> i` |
| gradientSetGradient | `(id, r1,g1,b1,r2,g2,b2: f) -> b` |
| gradientSetRainbow | `(id, speed: f) -> b` |
| gradientSetColor | `(id, r,g,b,a: f) -> b` |
| gradientSetEndpoints | `(id, x1,y1,z1: f, x2,y2,z2: f) -> b` |
| gradientDraw | `(id) -> b` |
| gradientDrawToDimension | `(id, dimId: i) -> b` |
| gradientRemove | `(id) -> b` |
| gradientDestroy | `(id) -> b` |
| gradientDestroyAll | `() -> nil` |
| gradientTick | `(deltaTime: f) -> nil`（彩虹动画驱动） |

### 2.4 itemDetail*（物品详情，2 函数）

| 函数 | 签名 |
|------|------|
| itemDetailShow | `(dimId: i, x,y,z: f, itemId: s, aux: i, count: i, customText: s) -> i` |
| itemDetailHide | `(id) -> b` |

`itemDetailShow`：在 (x,y,z) 显示"本地化物品名 xN"（count<=1 无数量后缀）；`customText` 传 `""` 用自动文本，非空则完全替代（支持 § 颜色码与 `{变量}`）。返回悬浮字 ID，可继续用 `holo*` 精修。

### 2.5 itemDisplay*（FMBE 物品悬浮显示，33 函数）

FMBE（狐狸+发包）技术：隐形狐狸手持物品渲染任意物品/方块的悬浮展示。三轴旋转/函数平移/缩放全部支持 **Molang 表达式**（如 `"math.sin(query.life_time*90)*360"`）。

| 函数 | 签名 |
|------|------|
| itemDisplayCreate | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i) -> i` |
| itemDisplayCreateAdvanced | `(x,y,z: f, dim: i, itemId: s, aux: i, offX,offY,offZ: s, rotX,rotY,rotZ: s, scale: s) -> i` |
| itemDisplayCreateSeamless | `(x,y,z: f, dim: i, itemId: s, aux: i, offX,offY,offZ: s, rotX,rotY,rotZ: s, scale: s, mode: i, viewDistance: f, visiblePlayer: s) -> i`（无感创建: mode/视距/白名单在首次 spawn 之前写入, 全程只发一次 Add 序列; 创建后无需 setMode/setViewDistance/setVisiblePlayer。mode: -1=默认 auto 1=item 2=block; viewDistance: -1=默认 0=不限; visiblePlayer 空串=全员） |
| itemDisplayCreateRandom | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i) -> i`（随机段 `[0x10000000,0x7FFFFFFF)` 不重复 ID） |
| itemDisplayCreateWithId | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i, desiredId: i) -> i`（指定 ID 创建, 持久化恢复用; <=0 或已占用返回 -2） |
| itemDisplayDestroy | `(id) -> b` |
| itemDisplayDestroyAll | `() -> nil` |
| itemDisplayExists | `(id) -> b` |
| itemDisplayIsIdUsed | `(id) -> b` |
| itemDisplayScaleBy | `(id, factor: f) -> b`（相对缩放: factor>1 放大, 0<factor<1 缩小） |
| itemDisplaySetItemWithNbt | `(id, itemId: s, aux: i, nbt: s) -> b`（换物品带附加数据: nbt 为 SNBT; 空串清除） |
| itemDisplaySetGlint | `(id, on: b) -> b`（附魔光效开关: BDS 原生路径注入 1 级锋利, 客户端紫色光效） |
| itemDisplaySetVisiblePlayers | `(id, playerNames: [s]) -> b`（可见玩家白名单: 按玩家名/LSE `player.realName` 匹配; 空列表 = 全员可见; 维度/视距条件仍叠加） |
| itemDisplayClearVisiblePlayers | `(id) -> b`（清除白名单, 恢复全员可见） |
| itemDisplaySetVisiblePlayer | `(id, playerName: s) -> b`（标量版单玩家白名单; JS 侧推荐） |
| itemDisplayGetInfo | `(id) -> s`（诊断探针: 返回运行态摘要; 未找到返回 `not_found`） |
| itemDisplayScaleTo | `(id, targetScale: f) -> b`（单次跳变缩放: 以新 scale 常量重发完整方块动画序列, 无 respawn、无 Remove/Add; 渐变动画由消费者步进驱动（如 5 步 easeOutCubic）。仅方块模式; scale<0.01 钳制为 0.01） |
| itemDisplayGetAllIds | `() -> [i]` |
| itemDisplaySetItem | `(id, itemId: s, aux: i) -> b`（换物品; 清除附加数据） |
| itemDisplaySetPosition | `(id, x,y,z: f, dim: i) -> b`（dim<0 仅改坐标） |
| itemDisplaySetOffset | `(id, ox,oy,oz: s) -> b`（模型单位平移; Molang） |
| itemDisplaySetBaseOffset | `(id, ox,oy,oz: s) -> b`（渲染像素基础偏移; Molang） |
| itemDisplaySetRotation | `(id, rx,ry,rz: s) -> b`（三轴旋转/度; Molang） |
| itemDisplaySetScale | `(id, scale: s) -> b`（Molang） |
| itemDisplaySetExtend | `(id, scale,rx,ry: s) -> b`（方块模式二段变换） |
| itemDisplaySetMode | `(id, mode: i) -> b`（0=auto 1=item 2=block） |
| itemDisplaySetEnabled | `(id, enabled: b) -> b` |
| itemDisplaySetViewDistance | `(id, dist: f) -> b`（<=0 无限制） |
| itemDisplayRotateY | `(id, delta: f) -> b`（偏航叠加增量） |
| itemDisplayFindNearest | `(x,y,z: f, dim: i, maxDist: f) -> i`（最近查找; 无匹配 -1） |
| itemDisplayFollow | `(id, playerName: s, offX,offY,offZ: f) -> b`（1.17.0 展示跟随玩家: 每 tick 同步目标实时坐标（含跨维度自动 respawn）, 对已见玩家发 MoveActorAbsolute（非 teleport, 客户端插值）平滑位移, 全程无 respawn 无闪烁; 玩家下线自动解除跟随（方块原地保留）; `itemDisplaySetPosition` 手动设位解除跟随） |
| itemDisplayUnfollow | `(id) -> b`（1.17.0 解除跟随; 无跟随关系返回 true） |
| itemDisplaySetHitbox | `(id, width: f, height: f) -> b`（1.17.0 AABB 判定体积: 对已见玩家广播 SetActorDataPacket(R53=Width R54=Height), 即时无 respawn; 数值持久入配置, respawn/新观察者自动按当前值发包; 0/0 = 恢复不可命中。开启后客户端射线可命中 → 攻击经 `ghost*` 路由回库内 id） |

渲染模式：`auto`（默认）按物品 3D/2D 自动选择；`item` 平面物品渲染（rotY 内部自动 +205 补偿狐狸头朝向）；`block` 3D 方块渲染（完整旋转矩阵路径 + 二段扩展变换）。可见性由库内 Level tick hook 自动同步。

### 2.6 entity*（自定义实体，33 函数）

AddActorPacket 直发客户端生成纯视觉实体（NPC 壳/装饰生物/盔甲架布景; 无碰撞、不可真实交互——点击经 `ghost*` 路由）。属性变更经 tick 脏刷新合并为单次 respawn（无闪烁）。

**创建与生命周期**

| 函数 | 签名 |
|------|------|
| entityCreate | `(identifier: s, x,y,z: f, dim: i) -> i` |
| entityCreateAdvanced | `(identifier: s, x,y,z: f, dim: i, yaw: f, pitch: f, scale: f, nametag: s) -> i` |
| entityCreateRandom | `(identifier: s, x,y,z: f, dim: i) -> i`（随机段 ID） |
| entityCreateWithId | `(identifier: s, x,y,z: f, dim: i, desiredId: i) -> i`（指定 ID 创建; <=0 或占用返回 -2） |
| entityDestroy | `(id) -> b` |
| entityDestroyAll | `() -> nil` |
| entityExists | `(id) -> b` |
| entityIsIdUsed | `(id) -> b` |
| entityGetAllIds | `() -> [i]` |

**属性**

| 函数 | 签名 |
|------|------|
| entitySetIdentifier | `(id, identifier: s) -> b`（换实体类型; 短名自动补 `minecraft:`） |
| entitySetPosition | `(id, x,y,z: f, dim: i) -> b`（dim<0 仅改坐标） |
| entitySetRotation | `(id, yaw: f, pitch: f) -> b`（度） |
| entitySetNametag | `(id, text: s) -> b`（支持 § 颜色码; 空串清除） |
| entitySetScale | `(id, scale: f) -> b`（<=0 拒绝; 自动钳制 0.0625~10） |
| entitySetVariant | `(id, variant: i) -> b` |
| entitySetMarkVariant | `(id, markVariant: i) -> b` |
| entitySetColorIndex | `(id, colorIndex: i) -> b`（羊/项圈等染色实体） |
| entitySetFlags | `(id, flags: i) -> b`（原始位掩码 0x01=着火 0x20=隐身 等） |
| entitySetInvisible | `(id, on: b) -> b`（隐身便捷开关, 幂等） |
| entitySetEnabled | `(id, enabled: b) -> b` |
| entitySetViewDistance | `(id, dist: f) -> b`（<=0 无限制） |
| entitySetPose | `(id, pose: i) -> b`（PoseIndex 0..13; 盔甲架坐姿/睡姿/跳舞等） |
| entitySetEquipmentSlot | `(id, slot: i, name: s, aux: i, nbt: s) -> b`（槽位 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; name 空清空; nbt 为 SNBT） |
| entityFindNearest | `(x,y,z: f, dim: i, maxDist: f) -> i`（最近查找; 无匹配 -1） |

**扩展能力**

| 函数 | 签名 |
|------|------|
| entityScaleBy | `(id, factor: f) -> b`（相对缩放: 现有 scale × factor, 自动钳制 0.0625~10） |
| entitySetVisiblePlayers | `(id, playerNames: [s]) -> b`（可见玩家白名单; 空列表 = 全员可见） |
| entitySetVisiblePlayer | `(id, playerName: s) -> b`（单玩家白名单标量版; JS 侧推荐） |
| entityClearVisiblePlayers | `(id) -> b`（恢复全员可见） |
| entityGetInfo | `(id) -> s`（诊断探针; 未找到返回 `not_found`） |
| entitySetRidePlayer | `(id, playerName: s) -> b`（实体骑到指定玩家头上; 空名清除; 玩家须在线） |
| entitySetRideEntity | `(id, vehicleEntityId: i) -> b`（骑到另一自定义实体上; 0 清除） |
| entityClearRide | `(id) -> b`（解除骑乘链接） |
| entityPlayAnimation | `(id, animation: s, stopExpression: s, durationTicks: i) -> b`（播放原版动画, 如 `animation.humanoid.base_pose`; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止） |
| entitySetPlayerNametag | `(id, playerName: s, text: s) -> b`（1.22.0; 只改该玩家看到的名字牌; 空串 = 清除覆盖回到 config） |
| entitySetPlayerScale | `(id, playerName: s, scale: f) -> b`（1.22.0; 只改该玩家看到的缩放; <=0 = 清除覆盖） |
| entitySetPlayerEquipmentSlot | `(id, playerName: s, slot: i, name: s, aux: i, nbt: s) -> b`（1.22.0; 0=主手 1=副手 2=头 3=胸 4=腿 5=脚; name 空 = 回退 config） |
| entityClearPlayerAppearance | `(id, playerName: s) -> b`（1.22.0; 清除该玩家的名字/缩放/装备覆盖） |

### 2.7 ghost*（交互事件轮询，2 函数）

| 函数 | 签名 |
|------|------|
| ghostPollInteractions | `() -> [s]`（取走并清空待处理交互队列; 队列上限 256 条, 满时丢最旧） |
| ghostClearInteractions | `() -> nil`（丢弃队列中全部待处理事件） |

事件条目为可解析字符串，格式：

```
player=<realName> action=<1..6> domain=<entity|itemDisplay|npc> id=<库内id> pos=(x,y,z)
```

- `action`：1=Interact（右键交互）2=Attack（攻击/左键）3=StopRiding 4=InteractUpdate 5=NpcOpen 6=OpenInventory
- `pos` 仅在客户端携带坐标时出现（InteractUpdate 等）
- 示例：`player=Steve action=2 domain=entity id=42` —— 玩家 Steve 左键点击了库内 id=42 的自定义实体

LSE 轮询示例：

```js
const poll = ll.import("HologramLib", "ghostPollInteractions");
setInterval(() => {
    for (const line of poll()) {
        const m = line.match(/player=(\S+) action=(\d+) domain=(\S+) id=(-?\d+)/);
        if (!m) continue;
        const [, player, action, domain, id] = m;
        if (action === "2" && domain === "entity") { /* 左键点击 NPC */ }
    }
}, 200);
```

### 2.8 particle*（通用粒子形状系统，22 函数）

形状（点/线/矩形环/填充面/长方体框/六面/多面体）+ 变换（平移/平滑移动/旋转/自旋/缩放/跟随玩家）+ 多玩家白名单 + 逐玩家视距裁剪，全部由 C++ tick 自驱动。发送走 vanilla 通道，BDS tick flush 自动聚合压缩为单 Batch 数据报。

**核心概念**：
- **形状**：创建时按世界坐标定义，内部换算为局部坐标（锚点 = 线中点/矩形中心/盒中心/多面体质心），采样点缓存复用
- **变换**：`world = anchor + R(欧拉ZYX, 度) · (scale · local)`；`setPos` 即"粒子移动"（下次发射按新位置整批重发）；`moveTo` 为平滑点对点移动（锚点 easeOutCubic 插值逼近目标，单点/整面（plane/rect）/整体形状（box/poly）均随锚点整体移动）；`spin` 为度/tick 自旋；`follow` 时锚点每 tick = 玩家位置 + 偏移（跨维度自动跟随；moveTo/setPos 会解除跟随）
- **可见性**：白名单（uuid CSV）空 = 形状所在维度全员；逐玩家 3D 视距裁剪（`viewDistance` 0 = 不裁剪）
- **生命周期**：`lifetimeTicks` 0 = 永久；`setLifetime` 从现在起重新计时；粒子瞬态 → 每 `intervalTicks` 整批重发维持常驻视觉
- **平面轴**（rect/plane `axis` 参数）：0=XY（w 沿 X、h 沿 Y）、1=YZ（w 沿 Z、h 沿 Y）、2=XZ（w 沿 X、h 沿 Z）

| 函数 | 签名 |
|------|------|
| particleCreatePoint | `(owner: s, dimId: i, x: f, y: f, z: f, effect: s, interval: i, lifetime: i) -> id` |
| particleCreateLine | `(owner: s, dimId: i, x1,y,z1: f, x2,y2,z2: f, step: f, effect: s, interval: i, lifetime: i) -> id`（锚点=中点, 旋转绕中点） |
| particleCreateRect | `(owner: s, dimId: i, cx,cy,cz: f, w: f, h: f, axis: i, step: f, effect: s, interval: i, lifetime: i) -> id`（矩形环线） |
| particleCreatePlane | 同 rect 参数（填充平面网格） |
| particleCreateBox | `(owner: s, dimId: i, cx,cy,cz: f, hx,hy,hz: f, step: f, effect: s, interval: i, lifetime: i) -> id`（长方体 12 边线框, 半尺寸） |
| particleCreateBoxFaces | 同 box 参数（六面填充） |
| particleCreatePoly | `(owner: s, dimId: i, verts: s, edges: s, step: f, effect: s, interval: i, lifetime: i) -> id`（verts `"x,y,z;x,y,z"`世界坐标锚点=质心; edges `"i-j"`顶点索引对） |
| particleSetPos | `(id: i64, x, y, z: f) -> b`（平移锚点; 解除跟随） |
| particleMoveBy | `(id: i64, dx, dy, dz: f) -> b`（相对平移） |
| particleMoveTo | `(id: i64, x, y, z: f, durationTicks: i) -> b`（平滑点对点移动, easeOutCubic; 解除跟随; duration<=0 = 立即到达） |
| particleSetRot | `(id: i64, rx, ry, rz: f) -> b`（欧拉角, 度, ZYX 序, 绕锚点） |
| particleSpin | `(id: i64, sx, sy, sz: f) -> b`（自旋速率, 度/tick; 0,0,0 停止） |
| particleSetScale | `(id: i64, s: f) -> b` |
| particleFollow | `(id: i64, uuid: s, offX, offY, offZ: f) -> b`（锚点跟随玩家+偏移, 每 tick） |
| particleUnfollow | `(id: i64) -> b` |
| particleSetEffect | `(id: i64, effect: s) -> b` |
| particleSetVisible | `(id: i64, playersCsv: s) -> b`（`"uuid1,uuid2"` 白名单; 空串 = 维度全员） |
| particleSetInterval | `(id: i64, ticks: i) -> b` |
| particleSetViewDistance | `(id: i64, blocks: i) -> b`（逐玩家 3D 裁剪; 0 = 不裁剪） |
| particleSetLifetime | `(id: i64, ticks: i) -> b`（从现在起; 0 = 永久） |
| particleDestroy | `(id: i64) -> b` |
| particleGetInfo | `(id: i64) -> s`（诊断探针） |

LSE 示例（旋转自旋立方体 + 跟随玩家的光圈）：

```js
const create = ll.import("HologramLib", "particleCreateBox");
const spin   = ll.import("HologramLib", "particleSpin");
const follow = ll.import("HologramLib", "particleFollow");
const rot    = ll.import("HologramLib", "particleSetRot");
const vis    = ll.import("HologramLib", "particleSetVisible");

// 1. 自旋立方体线框（绕中心旋转）
const box = create("demo", 0, 100.5, 64.5, 100.5, 3, 3, 3, 1, "minecraft:endrod", 2, 0);
spin(box, 0, 3, 0);            // 每 tick 绕 Y 转 3 度
vis(box, player.uuid);          // 仅该玩家可见

// 2. 跟随玩家脚下的光圈（plane XZ + follow）
const ring = ll.import("HologramLib", "particleCreateRect");
const halo = ring("demo", 0, 0, 0, 0, 6, 6, 2, 1.5, "minecraft:endrod", 2, 0);
follow(halo, player.uuid, 0, 0.3, 0); // 锚点 = 玩家位置 + 偏移, 每 tick 更新

// 手动旋转 45 度（覆盖自旋累计值）
rot(box, 45, 0, 0);
```

### 2.9 playerNpc*（假玩家 NPC，25 函数）

> **注意：NPC 皮肤暂无效（未解决）**。下列函数全部可正常调用（注册/采集/导入/换肤/导出都返回正常结果），NPC 也能正常创建、移动、朝向、缩放、视距与显隐控制；但客户端不会渲染所设置的皮肤，外观回退为默认模型。

纯协议假玩家（不占服务端实体系统）：`PlayerList(Add, 皮肤) → AddPlayer → [20t] PlayerList(Remove)`；点击交互经 ghost 管线 `domain="npc"`（§2.7 轮询）。皮肤三来源：PNG 文件注册 / 目录批量导入（一个子文件夹 = PNG + 可选 `.json` 模型）/ 从在线玩家采集（全字段运行时快照，玩家之后换肤不影响）。库不落盘，LSE 侧如需跨重启保留请重新注册/采集。

| 函数 | 签名 |
|------|------|
| playerNpcRegisterSkin | `(pngPath: s, skinId: s, geometry: s, armSize: s) -> b`（skinId 空 = 文件名; geometry 空 = geometry.humanoid.custom; armSize: wide/slim） |
| playerNpcImportSkins | `(dirPath: s) -> i`（目录批量导入: 一个子文件夹 = 一套皮肤, PNG 必需 + `.json` 几何可选; skinId = 文件夹名; 返回导入数量, 目录无效 -1） |
| playerNpcCaptureSkin | `(skinId: s, playerName: s) -> b`（从在线玩家采集当前皮肤, 全字段快照注册, 其后换肤不影响） |
| playerNpcHasSkin | `(skinId: s) -> b` |
| playerNpcUnregisterSkin | `(skinId: s) -> b`（有 NPC 引用时拒绝） |
| playerNpcGetSkinIds | `() -> [s]` |
| playerNpcCreate | `(x, y, z: f, dim: i, name: s, skinId: s) -> id`（失败 <0: -2 id 占用, -3 皮肤未注册） |
| playerNpcCreateRandom | 同 create 参数（随机段 ID） |
| playerNpcCreateWithId | `(x, y, z: f, dim: i, name: s, skinId: s, desiredId: i64) -> id`（持久化恢复用） |
| playerNpcDestroy | `(id: i64) -> b` |
| playerNpcDestroyAll | `() -> void` |
| playerNpcExists | `(id: i64) -> b` |
| playerNpcIsIdUsed | `(id: i64) -> b` |
| playerNpcGetAllIds | `() -> [i64]` |
| playerNpcSetPos | `(id: i64, x, y, z: f, dim: i) -> b`（dim<0 仅改坐标） |
| playerNpcSetRotation | `(id: i64, yaw: f) -> b` |
| playerNpcSetNametag | `(id: i64, text: s) -> b`（空串清除） |
| playerNpcSetSkin | `(id: i64, skinId: s) -> b`（换肤 = respawn） |
| playerNpcSetViewDistance | `(id: i64, dist: f) -> b`（<=0 无限制） |
| playerNpcSetEnabled | `(id: i64, enabled: b) -> b` |
| playerNpcSetVisiblePlayers | `(id: i64, players: [s]) -> b`（空列表 = 全员可见） |
| playerNpcClearVisiblePlayers | `(id: i64) -> b` |
| playerNpcSetVisiblePlayer | `(id: i64, playerName: s) -> b` |
| playerNpcGetDebugInfo | `(id: i64) -> s`（诊断探针） |

LSE 示例（采集玩家皮肤并创建 NPC）：

```js
const capture = ll.import("HologramLib", "playerNpcCaptureSkin");
const create  = ll.import("HologramLib", "playerNpcCreate");
const pos     = ll.import("HologramLib", "playerNpcSetPos");

// 1. 采集在线玩家 Steve 的当前皮肤（含几何/披风/动画, 快照注册为 "steve_skin"）
capture("steve_skin", "Steve");

// 2. 用采集皮肤创建假玩家（主世界出生点旁）
const npc = create(100.5, 64, 100.5, 0, "Steve的分身", "steve_skin");
// 移动 / 换肤 / 白名单
pos(npc, 105.5, 64, 100.5, -1);
```

PNG 注册示例（自定义皮肤 + slim 手臂）：

```js
const reg = ll.import("HologramLib", "playerNpcRegisterSkin");
reg("E:/server/skins/girl.png", "girl", "", "slim");
```

---

### 2.10 trade*（村民交易菜单，纯展示；1.22.0）

打开界面 + 摆出交易表，**没有任何回调**（域契约见 1.10）。

| 函数 | 签名 | 说明 |
|------|------|------|
| tradeOpen | `(playerName: s, tradeType: s, tier: i, experience: i, realTrades: b, carrierIdentifier: s) -> i` | 打开菜单。`realTrades = false`（推荐）= 纯协议层展示；`true` = 真实交易表（玩家真的能成交、物品真的消耗）。`carrierIdentifier` 空 = 村民, 可填 `minecraft:wandering_trader` |
| tradeAddOffer | `(id: i, buyAType: s, buyACount: i, buyBType: s, buyBCount: i, sellType: s, sellCount: i, sellName: s, tier: i, locked: b) -> b` | 追加一条并**就地重发**（不重开界面）；`buyBType` 空 = 无第二付费项 |
| tradeSetTier | `(id: i, tier: i, experience: i) -> b` | 改显示栏值 / 经验条（同样就地重发） |
| tradeClose / tradeCloseAll / tradeIsOpen / tradeGetIds | — | 生命周期与查询 |

```js
const tradeOpen     = ll.import("HologramLib", "tradeOpen");
const tradeAddOffer = ll.import("HologramLib", "tradeAddOffer");
const id = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
tradeAddOffer(id, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§b钻石", 1, false);
```

### 2.11 container*（虚拟容器 / 列表界面，1.21.0；1.22.0 补 LSE）

| 函数 | 签名 | 说明 |
|------|------|------|
| containerOpen | `(playerName: s, title: s, rows: i) -> i` | 打开一个空容器（`rows` 3 = 27 格, 6 = 54 格）。打开有固有延迟（默认 7 tick）, 之后才能填格 |
| containerSetItem | `(id: i, slot: i, type: s, count: i, damage: i, name: s, loreCsv: s) -> b` | 单格刷新（一条 `InventorySlot`, 无延迟无闪烁）；`type` 空 = 清空该槽；`loreCsv` 用 `\n` 或 `\|` 分行, 空 = 无描述行 |
| containerSetTitle | `(id: i, title: s) -> b` | 就地换标题（重发方块实体 + ContainerOpen, 不重摆方块） |
| containerClose / containerCloseAll / containerIsOpen / containerGetIds | — | 生命周期与查询 |
| containerPollClicks | `() -> [s]` | **取走并清空**待处理的点击/关闭；每条 `player=X menuId=N slot=S closed=0\|1` |
| containerClearClicks | `() -> void` | 丢弃队列里全部待处理点击 |

```js
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
const id = containerOpen("Steve", "§8任务列表", 3);
containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取|§8第二行");
// 定时器里取点击（也可用 ll.on 之类的守护循环）:
for (const line of containerPollClicks()) { /* 切分 player= / menuId= / slot= / closed= */ }
```

点击语义：点击就是一次物品拾取 —— 任何输入设备都会发包（实测 27 格与大容器第二半区都正常）。
服务端并没有这个容器，所以物品不会被真的拿走，库只回传槽位号（`slot` = 条目下标，`closed=1` 时 `slot=-1`）。

### 2.12 npcDialog*（NPC 对话框，1.21.0；1.22.0 补 LSE）

| 函数 | 签名 | 说明 |
|------|------|------|
| npcDialogOpen | `(playerName: s, npcName: s, sceneName: s, dialogue: s, buttonsSpec: s, carrierIdentifier: s) -> i` | 打开对话（同一玩家重复调用先关上一层）；按钮串格式见下 |
| npcDialogUpdate | `(id: i, dialogue: s, buttonsSpec: s) -> b` | 就地换正文与按钮（npcName / sceneName / 载体类型保留不变）；**只适合界面仍开着时的微调**，见下方实测限制 |
| npcDialogClose / npcDialogCloseAll / npcDialogIsOpen / npcDialogGetIds | — | 生命周期与查询 |
| npcDialogPollClicks | `() -> [s]` | 取走并清空；每条 `player=X dialogId=N scene=S button=I actionId=A closed=0\|1 commands=...` |
| npcDialogClearClicks | `() -> void` | 丢弃队列 |

按钮串格式：按钮之间用 `;` 分隔（空段跳过），字段用 `|` 分隔 —— `label|actionId|mode|命令1,命令2`（后三项可省）。
`mode`：0=普通按钮 1=关闭 2=打开。

**实测限制**：客户端点任意按钮后界面会自行收起，而 `npcDialogUpdate` 只重发一次 `NpcDialoguePacket(Open)`,
客户端不会因此重新弹出 —— 换下一层对话必须重新 `npcDialogOpen`（内部删旧载体、建新载体、再发一次 Open）。

```js
const npcDialogOpen       = ll.import("HologramLib", "npcDialogOpen");
const npcDialogPollClicks = ll.import("HologramLib", "npcDialogPollClicks");
const id = npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？", "§a接受|main#0|0|say 你好;;§c离开|main#1|1|", "");
```

### 2.13 sensing*（感知域 / 客户端设备，1.21.0；1.22.0 补 LSE）

数据源是客户端每 tick 上报的 `PlayerAuthInput`（`InputMode`），库逐包捕获后在内存里记一份 —— 没有事件、不用轮询。

| 函数 | 签名 | 说明 |
|------|------|------|
| sensingDeviceOf | `(playerName: s) -> s` | `keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown` |
| sensingIsTouch | `(playerName: s) -> b` | 是否触屏（最常用的那个） |
| sensingDeviceCode | `(playerName: s) -> i` | 0=Unknown 1=键鼠 2=触屏 3=手柄 4=体感（与 C++ 枚举同值） |

```js
const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
if (sensingIsTouch("Steve")) { /* 触屏: 用容器列表 */ } else { /* 键鼠: 用交易菜单 */ }
```

### 2.14 fakeInv*（背包虚容器，1.23.0）

| 函数 | 签名 | 说明 |
|------|------|------|
| fakeInvApply | `(playerName: s, itemsSpec: s, refreshIntervalTicks: i) -> b` | 整份下发。`itemsSpec` 每条 `槽位\|物品id\|数量\|名字\|描述行~描述行`，条目之间用 `;` 或换行分隔；`refreshIntervalTicks` 0 = 关周期重发 |
| fakeInvSetSlot | `(playerName: s, slot: i, type: s, count: i, damage: i, name: s, loreCsv: s) -> b` | 单格改动（`type` 空 = 清空该格） |
| fakeInvRefresh | `(playerName: s) -> b` | 立即重发伪造内容（打开界面后 / 发现被覆盖时用） |
| fakeInvClear / fakeInvClearAll | `(playerName: s) -> b` / `() -> void` | 撤销伪造并恢复真实背包 |
| fakeInvIsActive / fakeInvGetPlayers | `(playerName: s) -> b` / `() -> [s]` | 查询 |
| fakeInvPollClicks | `() -> [s]` | 取走并清空；每条 `player=X slot=S` |
| fakeInvClearClicks | `() -> void` | 丢弃队列 |

```js
const fakeInvApply = ll.import("HologramLib", "fakeInvApply");
const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
// 快捷栏 0 号一把剑、1 号 64 个钻石；槽位 9 = 主背包第一格
fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§c虚容之剑|§7攻击力 +7;1|minecraft:diamond|64|§b虚容钻石", 20);
for (const line of fakeInvPollClicks()) { /* "player=Steve slot=0" */ }
```

### 2.15 sulfur*（硫磺立方体展示，1.23.0）

| 函数 | 签名 | 说明 |
|------|------|------|
| sulfurCreate | `(x: f, y: f, z: f, dim: i, blockType: s, blockCount: i, blockDamage: i, blockName: s, archetype: s, invisible: b) -> i` | 生成一只"吞着方块"的立方体; `archetype` 空串 = 不下发属性; `invisible` = 立方体本体是否隐身（默认 true → 只看到被吞的方块） |
| sulfurSetBlock | `(id: i, type: s, count: i, damage: i, name: s) -> b` | 换吞下去的方块/物品（会重建一次实体） |
| sulfurSetArchetype | `(id: i, archetype: s) -> b` | 换外观档位（空串 = 清掉属性; 不重建实体） |
| sulfurSetInvisible / sulfurSetScale | `(id: i, on: b) / (id: i, scale: f) -> b` | 隐身 / 缩放 |
| sulfurDestroy / sulfurDestroyAll | — | 销毁（`sulfurDestroy` 单个 / `sulfurDestroyAll` 全部） |
| sulfurExists / sulfurGetIds / sulfurGetEntityId | — | 查询（`sulfurGetEntityId` 是背后的自定义实体 id） |

```js
const sulfurCreate      = ll.import("HologramLib", "sulfurCreate");
const sulfurSetArchetype = ll.import("HologramLib", "sulfurSetArchetype");
const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6书架", "regular");
sulfurSetArchetype(id, "sticky");   // 换外观档位（不重建实体）
```

## 3. 消费者版本协商示例

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x010500, "need HologramLib >= 1.5.0");

auto& lib = hologramlib::IHologramLib::getInstance();
if (lib.version() < 0x010500) { /* 运行时能力协商 */ }
```
