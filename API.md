# HologramLib API 参考

- API 版本：1.15.0（`HOLOGLIB_API_VERSION 0x011500`）
- 插件发布版本：`26.10.5`（规则：跟随 LeviLamina 版本前两段 + 尾号自增，如 `26.10.1 → 26.10.2 → 26.10.3`；LeviLamina 升级到 `26.11.x` 时从 `26.11.1` 重新起号）
- 唯一公开头：`include/hologramlib/HologramLib.h`

## API 稳定性契约

| 层级 | 冻结内容 | 演进规则 |
|------|----------|----------|
| C++ 接口 | 全部纯虚方法签名与语义（ABI 稳定：实现对象在 DLL 内创建，消费者只持引用） | 只在接口**尾部追加**方法、只新增函数；永不修改/删除 |
| C++ 宏 | `HOLOGLIB_API_VERSION`、`HOLOGLIB_API`、`hologramlib` 命名空间、枚举值 | 只追加枚举值 |
| LSE 命名空间 | 单一命名空间 `HologramLib` 全部函数名（四域前缀）、参数顺序、返回值类型 | 只增不改不删 |
| 版本协商 | `IHologramLib::version()`（BCD：0x010500 = 1.5.0） | 随发布递增 |

破坏兼容仅允许发生在大版本（2.0.0），届时提供全新接口名并长期保留旧接口最终冻结实现。

`src/` 目录一切内容均为内部实现，**不属于 API**。

---

## 1. C++ 接口

### 1.1 入口

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // 唯一 dllexport 工厂
        IShapeDrawer&  shapes();                     // 形状渲染
        IHologramText& holograms();                  // 悬浮字全息
        IItemDetail&   itemDetails();                // 物品详情
        IItemDisplay&  itemDisplays();               // FMBE 物品悬浮（1.6.0）
        ICustomEntity& customEntities();             // 自定义实体（1.10.0）
        IParticleShape& particleShapes();            // 通用粒子形状（1.14.0）
        bool     isLseAvailable();                   // LSE 兼容层是否已挂载
        uint32_t version();                          // 0x011400
        int64_t  findNearestItemDisplay(float x, float y, float z, int dim, double maxDist); // 1.6.0
        void     setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.12.0
        void     clearGhostInteractListener();                                             // 1.12.0
        std::vector<std::string> pollGhostInteractions();                                  // 1.12.0 LSE 轮询版
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）
- 坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；建议主线程调用（发包在调用线程执行）

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

`ShapeType` 枚举（与 LSE `getShapeType` 数值一致）：

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

> 注：旧版 `FilledQuad = 6` 已移除；数值 6 保留空洞，永不复用。

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
| | setDimension | `(int64_t id, int dimId) -> bool` | **1.12.0** 迁移维度：已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁） |
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

### 1.5 IItemDisplay（FMBE 物品悬浮显示，1.6.0 追加）

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
    std::string itemNbt{};                 // 物品附加数据（SNBT; 1.8.0; 自定义名称等）
    bool   itemGlint{false};               // 附魔光效（1.9.0; BDS 原生路径注入 1 级锋利）
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
| createRandom | `(ItemDisplayConfig const&) -> int64_t` | **1.7.0** 随机 ID 创建：库在随机段 `[0x10000000, 0x7FFFFFFF)` 自动生成不重复 ID（查重 + 与自增段隔离）; 成功返回生成的 ID, <0 失败 |
| createWithId | `(ItemDisplayConfig const&, int64_t desiredId) -> int64_t` | **1.7.0** 指定 ID 创建：持久化恢复场景（如随机 ID 重启后原位还原）; desiredId<=0 或已被占用返回 -2 |
| createSeamless | `(ItemDisplayConfig const&, int mode, double viewDistance, std::string const& visiblePlayer) -> int64_t` | **1.12.0** 无感创建：mode/视距/白名单在首次 spawn 之前写入，全程只发一次 Add 序列（ItemPhys 无感创建等价）。mode: -1=不改（用 config 默认）1/2=指定; viewDistance: -1=不改; visiblePlayer 空串=不限。消除旧路径 create→白名单收窄（他人 Add→Remove）→脏 respawn（Remove+Add 换新 ID）的创建瞬间多波闪烁 |
| isIdUsed | `(int64_t) -> bool` | **1.7.0** 查询 ID 是否在用 |
| scaleBy | `(int64_t, double factor) -> bool` | **1.7.1** 相对缩放（放大/缩小）：在现有 scale 上乘 factor; factor>1 放大, 0<factor<1 缩小; 常量直接相乘, 表达式包裹 `(expr)*factor`; factor<=0 返回 false |
| setItemWithNbt | `(int64_t, std::string const& item, int aux, std::string const& nbt) -> bool` | **1.8.0** 换物品（带附加数据）：nbt 为 SNBT 字符串（自定义名称等用户数据）; 空串 = 清除附加数据; SNBT 解析失败按无 NBT 处理并告警 |
| setGlint | `(int64_t, bool on) -> bool` | **1.9.0** 附魔光效开关：开 = BDS 原生 `saveEnchantsToUserData` 注入 1 级锋利（客户端紫色光效）; 关 = 移除附魔; 幂等（值未变不重发） |
| scaleTo | `(int64_t, double targetScale) -> bool` | **1.12.0** 单次跳变缩放（纯 setter 基元）：以新 scale 常量重发完整 5 包方块动画序列（controller 名带 `.<id>` 后缀客户端同名原地覆盖），无 respawn、无 Remove/Add。每次调用后实体处于单一稳定配置（sleeping 矩阵与 swelling 同步基于新常量）——不存在逐帧双写入者竞争。仅方块路径（mode=0 auto/2）。targetScale<0.01 钳制为 0.01 |
| ~~animateScale~~ | — | **1.13.0 已移除**（前置纯洁性决策：动画调度归 LSE 消费者，库只提供基元）。渐变动画请由 LSE 基于 `scaleTo` 步进驱动（如 5 步 easeOutCubic） |
| findNearestItemDisplay（IHologramLib 单例） | `(float x, float y, float z, int dim, double maxDist) -> int64_t` | 最近查找（dim 匹配; maxDist<=0 无限制; 无匹配 -1） |

可见性由库内 Level tick hook 自动同步（每 20 tick：同维度 + 可见距离内玩家自动生成/移除；玩家断线自动清理）；属性变更即时生效（对已见玩家原子 despawn→respawn）。`setMode` / `setViewDistance` 幂等（**1.12.0**：同值不标脏不 respawn，防创建后闪换）。

> **FMBE 出现/消失动画（1.13.0 纯洁性决策）**：库内不内置动画调度（`animateScale` 已移除——其逐 tick swelling 覆写与安装序列 sleeping 矩阵每帧重申的 `v.scale` 以不同节奏交替竞争，方块位置补偿项随 `v.scale` 摆动 = 渐变全程抖动）。动画由 LSE 消费者基于 `scaleTo` 基元**步进驱动**：每步调用后实体处于单一稳定配置（无逐帧双写竞争，步进间无抖动）。典型序列（MeowLand 边界实证）：`createSeamless` 直接以缩小版 scale(0.1) 发包 → 延迟约 500ms → 5 步 easeOutCubic `scaleTo` 渐进放大到目标; 消失 5 步 `scaleTo` 渐进缩到 0.01 → `destroy`。

### 1.6 ICustomEntity（自定义实体，1.10.0 追加）

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
    // ── 1.12.0 追加 ──
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
| | setScale | `(int64_t, float) -> bool` | <=0 拒绝；经 UpdateAttributesPacket 轻脏增量刷新（零闪烁） |
| | setVariant / setMarkVariant / setColorIndex | `(int64_t, int) -> bool` | 变种/染色 |
| | setFlags / setInvisible / setEnabled / setViewDistance | — | 行为 |
| | setPose | `(int64_t, int pose) -> bool` | PoseIndex 0..13（盔甲架/玩家姿态） |
| | setEquipmentSlot | `(int64_t, int slot, std::string const& name, int aux, std::string const& nbt) -> bool` | 六槽装备; name 空清空槽位; nbt 为 SNBT |
| | findNearest | `(float x, float y, float z, int dim, double maxDist) -> int64_t` | 最近查找; 无匹配 -1 |
| 缩放 | scaleBy | `(int64_t, double factor) -> bool` | **1.12.0** 相对缩放：现有 scale × factor，结果自动钳制 0.0625~10; factor<=0 返回 false |
| 可见性 | setVisiblePlayers | `(int64_t, std::vector<std::string> const&) -> bool` | **1.12.0** 白名单（按 realName 匹配）; 空列表 = 清除限制 |
| | setVisiblePlayer / clearVisiblePlayers | `(int64_t, std::string const&) / (int64_t) -> bool` | **1.12.0** 标量版 / 清除（恢复全员可见） |
| 诊断 | getDebugInfo | `(int64_t) -> std::string` | **1.12.0** 运行态摘要; 未找到返回 `not_found` |
| 骑乘 | setRidePlayer | `(int64_t, std::string const& playerName) -> bool` | **1.12.0** 实体骑到玩家头上（SetActorLinkPacket; 空名清除; 须在线） |
| | setRideEntity | `(int64_t, int64_t vehicleEntityId) -> bool` | **1.12.0** 骑到另一自定义实体上; 0 清除 |
| | clearRide | `(int64_t) -> bool` | **1.12.0** 解除骑乘链接 |
| 动画 | playAnimation | `(int64_t, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | **1.12.0** 播放原版动画（AnimateEntityPacket, 如 `animation.humanoid.base_pose`）; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止 |

属性变更经 tick 脏刷新合并为单次 respawn（无闪烁串台）；可见性由库内 Level tick hook 自动同步（同 itemDisplay 模式）。

### 1.7 GhostInteractEvent（ghost 交互事件，1.12.0）

客户端会对"协议上存在"的实体发 InteractPacket；库 hook `ServerNetworkHandler::$handle` 收包后，将目标 runtimeId 反查回库内 id 并派发，实现**可点击 NPC / 全息菜单**。

```cpp
struct GhostInteractEvent {
    std::string playerName;   // 点击者（realName）
    int         action{0};    // InteractPacket Action 原始值
    std::string domain;       // "entity" / "itemDisplay"
    int64_t     id{-1};       // 对应域的库内 id
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};
```

- `action`：1=Interact 2=Attack 3=StopRiding 4=InteractUpdate 5=NpcOpen 6=OpenInventory
- 消费方式二选一（可并存）：C++ 推送 `setGhostInteractListener`；LSE 轮询 `ghostPollInteractions`（取走并清空队列，队列上限 256 条、满时丢最旧）
- 恒调 origin，不改变 BDS 对未知 runtimeId 交互包的原版行为；无监听且无人轮询时仅做 runtimeId 段判别，近零开销

### 1.8 IParticleShape（通用协议层粒子形状，1.14.0 追加 / 1.15.0 升级）

点/线/矩形环/填充面/长方体框/六面/多面体 + 平移/平滑移动/旋转/自旋/缩放/跟随。批量并发发送（1.15.0）：逐玩家 vanilla `SpawnParticleEffectPacket` 经 `NetworkSystem` 入队，BDS tick flush 自动聚合压缩单 Batch 数据报（与原版粒子广播同路径）；采样/视距裁剪/周期重发由库内 tick 自驱动，消费者只管创建与控制。访问方式：`IHologramLib::getInstance().particleShapes()`。

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
| | moveTo | `(int64_t id, float x, float y, float z, int durationTicks) -> bool` | 1.15.0 平滑点对点移动（easeOutCubic；解除跟随；duration<=0 立即到达） |
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
- 与 LSE `particle*`（§2.8，21 函数）共用同一 Manager，id 空间互通

C++ 消费示例：

```cpp
#include "hologramlib/HologramLib.h"

auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();
// 玩家脚下 16×16 粒子墙（XZ 平面填充, 跟随玩家）
auto id = ps.createPlane("MeowLand", 0, 0, 64, 0, 16, 16, 2, 1.0f,
                         "minecraft:endrod", 4, 0);
if (id > 0) {
    ps.follow(id, playerUuid, 0, -2, 0);
    ps.setVisiblePlayers(id, {playerUuid}); // 仅本人可见
}
```

---

## 2. LSE 接口（ll.import 统一命名空间）

LegacyRemoteCall（lrca）在场时自动导出。**单命名空间 `HologramLib`**，前缀区分能力域：

| 前缀域 | 能力 | 对应 C++ 接口 | 函数数 |
|--------|------|---------------|--------|
| `shape*` | 形状渲染 | IShapeDrawer | 36 |
| `holo*` | 悬浮字全息 | IHologramText | 26 |
| `gradient*` | 渐变线 | GradientLineManager | 11 |
| `itemDetail*` | 物品详情 | IItemDetail | 2 |
| `itemDisplay*` | FMBE 物品悬浮 | IItemDisplay | 30 |
| `entity*` | 自定义实体 | ICustomEntity | 33 |
| `ghost*` | 交互事件轮询 | IHologramLib | 2 |
| `particle*` | 通用粒子形状系统 | ParticleShapeManager | 20 |

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
| holoSetDimension | `(id, dimId: i) -> b`（**1.12.0** 迁移维度; 已绘制时原地重发, 无闪烁） |
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

### 2.5 itemDisplay*（FMBE 物品悬浮显示，1.6.0 追加，30 函数; `itemDisplayAnimateScale` 已于 1.13.0 移除）

FMBE（狐狸+发包）技术：隐形狐狸手持物品渲染任意物品/方块的悬浮展示。三轴旋转/函数平移/缩放全部支持 **Molang 表达式**（如 `"math.sin(query.life_time*90)*360"`）。

| 函数 | 签名 |
|------|------|
| itemDisplayCreate | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i) -> i` |
| itemDisplayCreateAdvanced | `(x,y,z: f, dim: i, itemId: s, aux: i, offX,offY,offZ: s, rotX,rotY,rotZ: s, scale: s) -> i` |
| itemDisplayCreateSeamless | `(x,y,z: f, dim: i, itemId: s, aux: i, offX,offY,offZ: s, rotX,rotY,rotZ: s, scale: s, mode: i, viewDistance: f, visiblePlayer: s) -> i`（**1.12.0** 无感创建: mode/视距/白名单在首次 spawn 之前写入, 全程只发一次 Add 序列——ItemPhys 无感创建等价; 创建后无需 setMode/setViewDistance/setVisiblePlayer, 无白名单收窄 Add→Remove、无脏 respawn Remove+Add 闪烁。mode: -1=默认 auto 1=item 2=block; viewDistance: -1=默认 0=不限; visiblePlayer 空串=全员。批量边界渲染等"创建瞬间可见"场景首选） |
| itemDisplayCreateRandom | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i) -> i`（**1.7.0** 库自动生成随机段 `[0x10000000,0x7FFFFFFF)` 不重复 ID, 成功返还 ID 值; <0 失败） |
| itemDisplayCreateWithId | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i, desiredId: i) -> i`（**1.7.0** 指定 ID 创建, 持久化恢复用; desiredId<=0 或已占用返回 -2） |
| itemDisplayDestroy | `(id) -> b` |
| itemDisplayDestroyAll | `() -> nil` |
| itemDisplayExists | `(id) -> b` |
| itemDisplayIsIdUsed | `(id) -> b`（**1.7.0** 查询 ID 是否在用） |
| itemDisplayScaleBy | `(id, factor: f) -> b`（**1.7.1** 相对缩放: factor>1 放大, 0<factor<1 缩小; 常量直接乘, 表达式包裹乘法） |
| itemDisplaySetItemWithNbt | `(id, itemId: s, aux: i, nbt: s) -> b`（**1.8.0** 换物品带附加数据: nbt 为 SNBT 字符串, 携带自定义名称等; 空串清除） |
| itemDisplaySetGlint | `(id, on: b) -> b`（**1.9.0** 附魔光效开关: BDS 原生路径注入 1 级锋利, 客户端紫色光效） |
| itemDisplaySetVisiblePlayers | `(id, playerNames: [s]) -> b`（**1.10.0** 可见玩家白名单: 仅名单内玩家可见, 按玩家名/LSE `player.realName` 匹配; 空列表 = 清除限制 = 全员可见; 维度/视距条件仍叠加生效） |
| itemDisplayClearVisiblePlayers | `(id) -> b`（**1.10.0** 清除白名单, 恢复全员可见） |
| itemDisplaySetVisiblePlayer | `(id, playerName: s) -> b`（**1.10.1** 标量版单玩家白名单, 语义同上; 规避数组编组差异, JS 侧推荐） |
| itemDisplayGetInfo | `(id) -> s`（**1.10.1** 诊断探针: 返回运行态摘要 `id/dim/pos/mode/enabled/item/view/filter/shown`; 未找到返回 `not_found`） |
| itemDisplayScaleTo | `(id, targetScale: f) -> b`（**1.12.0** 单次跳变缩放·纯 setter 基元: 以新 scale 常量重发完整 5 包方块动画序列（controller 名带 `.<id>` 后缀客户端同名原地覆盖）, 无 respawn、无 Remove/Add。每步调用后实体处于单一稳定配置（sleeping 矩阵与 swelling 同步基于新常量）——无逐帧双写竞争。**1.13.0 起动画调度归 LSE**：渐变动画由消费者以 `scaleTo` 步进驱动（如 5 步 easeOutCubic setInterval, MeowLand 边界实证方案）; `itemDisplayAnimateScale`（1.10.2 C++ 端渐变调度）已于 1.13.0 移除。仅方块模式; scale<0.01 钳制为 0.01） |
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

渲染模式：`auto`（默认）按物品 3D/2D 自动选择；`item` 平面物品渲染（wiki.scale/wiki.posrot 路径，rotY 内部自动 +205 补偿狐狸头朝向）；`block` 3D 方块渲染（完整旋转矩阵路径 + 二段扩展变换）。可见性由库内 Level tick hook 自动同步（同维度 + 可见距离内玩家自动生成/移除，玩家加入/断线自动清理）。

### 2.6 entity*（自定义实体，1.10.0 追加，33 函数）

AddActorPacket 直发客户端生成纯视觉实体（NPC 壳/装饰生物/盔甲架布景; 无碰撞、不可真实交互——点击经 `ghost*` 路由）。属性变更经 tick 脏刷新合并为单次 respawn（无闪烁）。

**创建与生命周期**

| 函数 | 签名 |
|------|------|
| entityCreate | `(identifier: s, x,y,z: f, dim: i) -> i` |
| entityCreateAdvanced | `(identifier: s, x,y,z: f, dim: i, yaw: f, pitch: f, scale: f, nametag: s) -> i` |
| entityCreateRandom | `(identifier: s, x,y,z: f, dim: i) -> i`（随机段 `[0x10000000,0x7FFFFFFF)` ID, 成功返还 ID 值; <0 失败） |
| entityCreateWithId | `(identifier: s, x,y,z: f, dim: i, desiredId: i) -> i`（指定 ID 创建, 持久化恢复用; <=0 或占用返回 -2） |
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
| entitySetScale | `(id, scale: f) -> b`（<=0 拒绝; 客户端有效域 0.0625~10 自动钳制） |
| entitySetVariant | `(id, variant: i) -> b` |
| entitySetMarkVariant | `(id, markVariant: i) -> b` |
| entitySetColorIndex | `(id, colorIndex: i) -> b`（羊/项圈等染色实体） |
| entitySetFlags | `(id, flags: i) -> b`（原始位掩码 0x01=着火 0x20=隐身 等） |
| entitySetInvisible | `(id, on: b) -> b`（隐身便捷开关, 幂等） |
| entitySetEnabled | `(id, enabled: b) -> b` |
| entitySetViewDistance | `(id, dist: f) -> b`（<=0 无限制） |
| entitySetPose | `(id, pose: i) -> b`（**1.12.0** PoseIndex 0..13; 盔甲架坐姿/睡姿/跳舞等, 经 respawn 生效） |
| entitySetEquipmentSlot | `(id, slot: i, name: s, aux: i, nbt: s) -> b`（**1.12.0** 槽位 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; name 空清空; nbt 为 SNBT） |
| entityFindNearest | `(x,y,z: f, dim: i, maxDist: f) -> i`（最近查找; 无匹配 -1） |

**1.12.0 追加（对称性补全 / 新能力）**

| 函数 | 签名 |
|------|------|
| entityScaleBy | `(id, factor: f) -> b`（相对缩放: 现有 scale × factor, 自动钳制 0.0625~10） |
| entitySetVisiblePlayers | `(id, playerNames: [s]) -> b`（可见玩家白名单; 空列表 = 清除限制 = 全员可见） |
| entitySetVisiblePlayer | `(id, playerName: s) -> b`（单玩家白名单标量版; JS 侧推荐） |
| entityClearVisiblePlayers | `(id) -> b`（恢复全员可见） |
| entityGetInfo | `(id) -> s`（诊断探针: 运行态摘要; 未找到返回 `not_found`） |
| entitySetRidePlayer | `(id, playerName: s) -> b`（实体骑到指定玩家头上; 空名清除; 玩家须在线） |
| entitySetRideEntity | `(id, vehicleEntityId: i) -> b`（骑到另一自定义实体上; 0 清除） |
| entityClearRide | `(id) -> b`（解除骑乘链接） |
| entityPlayAnimation | `(id, animation: s, stopExpression: s, durationTicks: i) -> b`（播放原版动画, 如 `animation.humanoid.base_pose`; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止） |

### 2.7 ghost*（交互事件轮询，1.12.0 追加，2 函数）

| 函数 | 签名 |
|------|------|
| ghostPollInteractions | `() -> [s]`（取走并清空待处理交互队列; 队列上限 256 条, 满时丢最旧） |
| ghostClearInteractions | `() -> nil`（丢弃队列中全部待处理事件） |

事件条目为可解析字符串，格式：

```
player=<realName> action=<1..6> domain=<entity|itemDisplay> id=<库内id> pos=(x,y,z)
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

### 2.8 particle*（通用协议层粒子形状系统，1.14.0 / 1.15.0 升级，21 函数）

**批量并发发送 + 全形状 + 智能控制**：供所有插件使用的通用粒子系统。形状（点/线/矩形环/填充面/长方体框/六面/多面体）+ 变换（平移/平滑移动/旋转/自旋/缩放/跟随玩家）+ 多玩家白名单 + 逐玩家视距裁剪，全部由 C++ tick 自驱动。发送走 vanilla 通道：逐玩家构造 `SpawnParticleEffectPacket` 经 `NetworkSystem` 入队，BDS tick flush 自动聚合压缩为单 Batch 数据报（与原版粒子广播同路径），客户端单数据报整批收到；配合白名单与视距裁剪，实际发包量远低于 LSE 逐粒子 `spawnParticle` 的全员序列发送。

**核心概念**：
- **形状**：创建时按世界坐标定义，内部换算为局部坐标（锚点 = 线中点/矩形中心/盒中心/多面体质心），采样点缓存复用；坐标全链路 float，支持任意浮点位置
- **变换**：`world = anchor + R(欧拉ZYX, 度) · (scale · local)`；`setPos` 即"粒子移动"（下次发射按新位置整批重发）；`moveTo`（1.15.0）为平滑点对点移动——锚点 easeOutCubic 插值逼近目标，单点/整面（plane/rect）/整体形状（box/poly）均随锚点整体移动；`spin` 为度/tick 自旋（tick 内累积，与发射节奏解耦）；`follow` 时锚点每 tick = 玩家位置 + 偏移（跨维度自动跟随；moveTo/setPos 会解除跟随）
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
| particleMoveTo | `(id: i64, x, y, z: f, durationTicks: i) -> b`（1.15.0 平滑点对点移动, easeOutCubic; 解除跟随; duration<=0 = 立即到达） |
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

MeowLand 领地渲染即基于本系统：边框 = rect 环线（2D）/ box 线框（3D）+ 四角柱 line；粒子墙 = plane，JS 每 300ms 量化窗口 `particleSetPos` 平移（墙跟随玩家移动，粒子移动即整批重发在新位置）。

---

## 3. 消费者版本协商示例

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x010500, "need HologramLib >= 1.5.0");

auto& lib = hologramlib::IHologramLib::getInstance();
if (lib.version() < 0x010500) { /* 运行时能力协商 */ }
```
