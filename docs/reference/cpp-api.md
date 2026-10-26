# C++ 接口参考

## 1.1 入口

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
        ITradeMenu&     tradeMenus();      // 村民交易菜单（1.21.0）
        INpcDialogue&   npcDialogs();      // NPC 对话框（1.21.0）
        IContainerMenu& containerMenus();  // 虚拟容器 / 列表界面（1.21.0）
        IPlayerSensing& playerSensing();   // 感知域（1.21.0）
        IFakeInventory& fakeInventories(); // 背包虚容器（1.23.0）
        ISulfurDisplay& sulfurDisplays();  // 硫磺立方体展示（1.23.0）
        IViewOverride&  viewOverrides();   // 客户端视图覆盖（1.25.0）
        bool     isLseAvailable();        // LSE 兼容层是否已挂载
        uint32_t version();               // 0x012300 = 1.29.0
        int64_t  findNearestItemDisplay(float x, float y, float z, int dim, double maxDist); // 1.6.0
        void     setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.12.0
        void     clearGhostInteractListener();                                             // 1.12.0
        std::vector<std::string> pollGhostInteractions();                                  // 1.12.0 LSE 轮询版
        uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.19.1 多播
        bool     removeGhostInteractListener(uint64_t token);                               // 1.19.1
        uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener); // 1.27.0 真实实体交互（多播）
        bool     removeActorInteractListener(uint64_t token);                               // 1.27.0
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）；坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；发包在调用线程执行，建议主线程调用

## 1.2 IShapeDrawer（形状渲染）

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
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | 三轴旋转（度; 文本形状自动启用 useRotation + 双面渲染） |
| clearRotation | `(int64_t id) -> bool` | 清除旋转（恢复面向相机） |
| draw | `(int64_t id) -> bool` | 全维度可见者绘制 |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | 指定玩家 |
| drawToDimension | `(int64_t id, int dimId) -> bool` | 指定维度 |
| remove | `(int64_t id) -> bool` | 隐藏（保留数据） |
| update | `(int64_t id) -> bool` | 可见时原地重发（同 networkId 覆盖，无闪烁） |
| destroy | `(int64_t id) -> bool` | 销毁 |
| destroyAll | `() -> void` | 全部销毁 |
| exists | `(int64_t id) -> bool` | 存在性 |
| type | `(int64_t id) -> ShapeType` | 形状类型 |
| setBackgroundColor | `(int64_t id, float r, float g, float b, float a) -> bool` | 背景框颜色（1.26.0; 仅文本形状, 不设 = 客户端默认色） |
| clearBackgroundColor | `(int64_t id) -> bool` | 回客户端默认背景色（1.26.0） |
| setDepthTest | `(int64_t id, bool enabled) -> bool` | 穿墙可见性（1.26.0; true = 被方块/实体遮挡, false = 始终渲染） |

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

## 1.3 IHologramText（悬浮字全息）

整块多行文本单一背景框渲染（`\n` 合并所有行）；变更后调用 `refresh` 原地重绘（同 networkId 覆盖，无闪烁）。行索引 0 起。
**1.26.0 收缩后口径**：行只负责文本；颜色/缩放/背景框/穿墙/旋转都是**整块**属性 —— 行级样式差异请在文本内嵌 § 颜色代码。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create | `(float x, float y, float z) -> int64_t` | 创建 |
| | destroy | `(int64_t id) -> bool` | 销毁 |
| | destroyAll | `() -> void` | 全部销毁 |
| 行管理 | addLine | `(int64_t id, std::string const& text) -> bool` | 追加行 |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | 行文本 |
| | setLinePool | `(int64_t id, int line, std::vector<std::string> const& content, int intervalMs) -> bool` | 动态行（1.26.0）: 内容池按 `intervalMs` **时间取模轮播**（无状态, 多个浮字同池同相）; 池 ≤1 项 / 间隔≤0 = 不轮播; 空 = 清除池 |
| | setLineParseVariables | `(int64_t id, int line, bool enabled) -> bool` | 该行是否解析变量（默认 true; 关 = `{}` 按字面量显示）（1.26.0） |
| | removeLine | `(int64_t id, int line) -> bool` | 移除行 |
| | clearLines | `(int64_t id) -> bool` | 清空行 |
| | getLineCount | `(int64_t id) -> int` | 行数 |
| 颜色 | setColor | `(int64_t id, r, g, b, a: float) -> bool` | 整块文字颜色 |
| | setBackgroundColor | `(int64_t id, r, g, b, a: float) -> bool` | 背景框颜色（不设 = 客户端默认色; 1.26.0） |
| | clearBackgroundColor | `(int64_t id) -> bool` | 清除背景框颜色（1.26.0） |
| 样式 | setScale | `(int64_t id, float scale) -> bool` | 整块缩放（1.26.0; 替代原行级 setLineScale） |
| | setDepthTest | `(int64_t id, bool enabled) -> bool` | 穿墙可见性（1.26.0; true = 被方块/实体遮挡） |
| | setRotation | `(int64_t id, pitch, yaw, roll: float) -> bool` | 三轴固定朝向（度; 不再面向相机; 1.26.0） |
| | clearRotation | `(int64_t id) -> bool` | 恢复面向相机（1.26.0） |
| 位置 | setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | 跟随玩家（位置在调用时就地解析, 无自驱） |
| | clearFollowPlayer | `(int64_t id) -> bool` | 取消跟随 |
| | setDimension | `(int64_t id, int dimId) -> bool` | 迁移维度：已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁） |
| 显示 | draw / drawToDimension / drawToPlayer / remove | 同形状语义 | |
| | refresh | `(int64_t id) -> bool` | 重解析变量/跟随坐标并原地重发 |

> **1.26.0 收缩（明记）**：`setLineScale` / `setLineColor` / `setLineGradient` / `setLineRainbow` /
> `setLineScroll` / `setVerticalAnimation` / `setLineSpacing` 与 `tick` 已移除 —— 这些在"整块单形状"
> 模型下不可实现或空转。原因与替代（§ 颜色代码 / setScale）见 [更新日志](../versions/changelog.md)
> 与 [版本与 ABI 对照](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)。
> **C++ ABI**：它们保留为**废弃空槽**（非纯虚 + 内联空实现 + `[[deprecated]]`，调用返回 `false` / 无动作），
> 虚表槽位与 1.25.0 一致 —— 未重编译的旧消费方二进制按旧槽位调用不会错位；LSE 侧导出照旧删除。

> **动态行（1.26.0；重构自 [Phantom](https://github.com/GroupMountain/Phantom)，LGPL-3.0，已按其许可标注来源）**：`setLinePool` = 内容池按 `intervalMs` **时间取模轮播**
> （无状态，多个浮字同池同相）；`setLineParseVariables` = 行级变量开关（默认开）。轮播/变量刷新由库内
> **0.5s 节流**驱动、**内容变了才重发**（区别于本版移除的自驱"偏移"——那时每拍都在动、但驱出来的东西
> 没有消费方）。变量集：`{player}` `{online}` `{time}` `{tps}` `{dimension}` `{x}` `{y}` `{z}` +
> 经 MeowPAPI 的外部占位符。**含 `{var}` 的文本按观看者各一份形状**（逐玩家解析），其刷新会逐玩家
> 原地重发（同 networkId，无闪烁）。

内置变量（PAPI 翻译后兜底解析）：`{time}` `{online}` `{tps}` `{player}`。
外部占位符 `%name%` / `{name}` 经 LseBridge 调 `MeowPAPI::translateString(WithPlayer)`（可选，缺席原样保留）。

## 1.4 IItemDetail（物品详情）

```cpp
// 在 (x,y,z) 显示 "本地化物品名 xN"（自动翻译; count<=1 无数量后缀）
// customText 非空则完全替代自动文本（支持 § 颜色码与 {变量}）
// 返回悬浮字 ID（可用 holograms() 继续精修）
int64_t show(int dimId, float x, float y, float z,
             std::string const            std::string const& itemId, int aux, int count, itemId, int aux, int count,
             std::string const            std::string const& customText = ""); customText = "");
bool hide(int64_t id);
```

物品名解析链：`ItemRegistry（HashedString）→ ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → 兜底原样显示 ID（不阻断）。

## 1.5 IItemDisplay（FMBE 物品悬浮显示，1.6.0）

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

## 1.6 ICustomEntity（自定义实体，1.10.0）

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

## 1.7 GhostInteractEvent（ghost 交互事件，1.12.0）

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

## 1.8 IParticleShape（通用协议层粒子形状，1.14.0）

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

## 1.9 IPlayerNpc（假玩家 NPC，1.16.0）

> **NPC 皮肤已可正常渲染（26.40.2 修复）**。本节全部接口可用且数据链路完整：皮肤注册（PNG / 在线采集 / 目录导入）、`getSkinBlob` / `registerSkinFromBlob` 导出恢复、NPC 创建/移动/朝向/缩放/视距/显隐、点击交互均正常。此前的症状（客户端不渲染皮肤、外观回退默认模型）根因在 PlayerList 皮肤条目的 `Id` / `FullId` 为空或残留原玩家缓存键，已修复。

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
| 轻量位置（26.40.8） | setPositionLight | `(int64_t id, float x, float y, float z, int dim) -> bool` | **只发 `MoveActorAbsolute` 增量**（不重建实体/不重发皮肤, 无闪烁）; 与 setRotationLight 同一条轻脏通道, 同一 tick 合并一条包; `dim` 与当前维度不同返回 false（跨维度仍走 setPosition 重建） |
| 皮肤注入（26.40.8; 仅 C++） | injectSkin | `(std::string const& viewerName, std::string const& targetName, std::string const& skinId) -> bool` | 用 **target 自己的 UUID / uniqueId** 发 `PlayerList(Add)` 就地更新皮肤条目（真实 xuid + trust 三态 + 皮肤对象 `OverridesPlayerAppearance=true`——覆盖客户端已装备皮肤的前提）; `viewerName` 空串 = 所有在线玩家（**含 target 本人**）; 条目按注册表原样下发; **不自动重发**（丢帧由消费方再调一次） |
| | injectSkinAll | `(std::string const& targetName, std::string const& skinId) -> bool` | 全员版（等价 `injectSkin("", targetName, skinId)`） |
| 动画（26.40.8; 仅 C++） | playAnimation | `(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 对"已见过该 NPC 的玩家"发 `AnimateEntityPacket`（controller 名按 NPC id 内自动唯一化, 资源包无需同名控制器）; `stopExpression` 空 = 常驻, `query.any_animation` = 立刻停, `durationTicks>0` 到期自动补停止包; 动画名需客户端可解析（原版或随资源包下发）; 无观察者返回 false |
| | playAnimationTo | `(int64_t id, std::string const& playerName, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 同上, 只发给该玩家（补发用; 未见过该 NPC 返回 false） |
| 出生回调（26.40.8; 仅 C++） | setEntitySpawnCallback | `(std::function<void(int64_t id, std::string const& playerName)>) -> void` | NPC 对某玩家**出生完成**回调（供补发动画/状态; EntitySpawnCallback 语义）; 与 customentity 域同款队列（+2 tick 发出） |
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

## 1.10 ITradeMenu（村民交易菜单，1.21.0；1.22.0 起纯展示；**1.24.0 起可纯协议真结算**）

`IHologramLib::tradeMenus()`。**手写包**打开交易界面：界面完全由本库构造的 `UpdateTradePacket` 驱动，载荷与 BDS 26.40 原生村民交易逐字节一致（`tests/check-trade-packet.bat` 整包对拍、`check-trade-offers.bat` 只对 Offers 段）。交易界面只靠这个包打开——原生流程里没有 `ContainerType=Trade` 的 `ContainerOpen`。

> **默认不读任何点击**；开了 `settleLocally` 才读**交易槽上的物品动作**与带自建配方 id 的 `CraftRecipe` 动作。
>
> **真结算（1.24.0）**：`TradeMenuSpec::settleLocally = true`（且 `usePacketOffers = true`）时，界面仍是自建 `UpdateTrade`（服务端不放交易表），但库接住客户端的付费放置 / 取回 / 成交请求并**自己落地**：从真实背包扣下付费暂存、成交时校验账本并按条目把产物写进背包、关界面把没花掉的付费退回；"点一下拿起"那份也由库托管（光标）。结果通过 `TradeSettlementEvent` 回传（C++ 监听器 / LSE `tradePollSettlements`）。判定逻辑独立在 `src/trade/TradeSettlementLogic.h`，离线自检 `tests/check-trade-settlement.bat`（43 项）。**已知限制**：客户端要求服务端在应答里回带交易槽的槽位更正，而该段在本协议版本与协议库线格式不一致（照协议库手写会打崩客户端）—— 放料被服务端受理，但客户端界面可能撤回它。**要稳定成交用 `usePacketOffers=false`（BDS 结算）或容器 UI。**
>
> 实测边界：触屏在纯协议层到不了成交（付费拖不进交易槽）——触屏用真实交易表路径或容器 UI；客户端**买不起就不发包**；账本按"类型 + 数量"扣（不细分交易槽位）；`numCrafts` 不展开（一次成交发一份产物）。

两条路径（`TradeMenuSpec::usePacketOffers`）：

- **`true` + `false`（默认，纯展示）**：只发自建 `UpdateTrade`，服务端不放交易表。玩家往付费槽放东西的请求会被 BDS 拒掉、物品弹回，界面停在"不可成交"（与参考实现 GMLIB `ChestUI` 同路）。
- **`true` + `true`（1.24.0，纯协议真结算）**：见上方说明。
- **`false`（真实交易表）**：给载体实体装真实交易表并调 BDS 的 `openTrading`，**玩家是真的在交易**（物品真的消耗；触屏可走完整流程）。`carrierUniqueIdOverride != 0` 时直接以该真实实体为交易对象；这条路上 `settleLocally` 无效。

载体实体：交易界面需要 `EntityUniqueId`，因此打开时会在玩家身后 5 格生成一个**隐身、仅该玩家可见**的假村民（`minecraft:villager_v2`），关闭即删除。载体是异步送达客户端的，所以 `UpdateTrade` 与经验条元数据都在几 tick 后发送（背靠背发时客户端还不认识该实体，界面绑不上去）。经验条不在包内，由载体实体的 `TradeTier` / `MaxTradeTier`（恒 4）/ `TradeExperience` 元数据驱动。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | open | `(std::string const& playerName, TradeMenuSpec const&) -> int64_t` | 同一玩家重复调用先关旧菜单；失败 -1 |
| | update / close / closeAll / isOpen / getAllIds | — | `update` 重发交易表（真实表路径会重开界面） |
| 内容（1.22.0 追加） | addOffer | `(int64_t menuId, TradeMenuOffer const&) -> bool` | 追加一条并**就地重发**交易表（不重开界面、不等延迟） |
| | setTier | `(int64_t menuId, int tier, int experience) -> bool` | 改显示栏值 / 经验条，同样就地重发 |
| 真结算（1.24.0） | setSettleLocally / isSettleLocally | `(int64_t, bool) -> bool` / `(int64_t) const -> bool` | 打开后随时切换真结算（初值在 `TradeMenuSpec::settleLocally`） |
| | addSettlementListener / removeSettlementListener | `(std::function<void(TradeSettlementEvent const&)>) -> uint64_t` / `(uint64_t) -> bool` | 成交 / 失败回传（C++ 侧） |
| | pollSettlements | `() -> std::vector<std::string>` | LSE 轮询（取走并清空）; 条目 `"player=Steve menuId=3 offer=1 ok=1 reason="` |

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

## 1.11 INpcDialogue（NPC 对话框，1.21.0）

`IHologramLib::npcDialogs()`。`NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON）下发，`NpcRequestPacket` 回传（点了第几个按钮 / 玩家关闭）。

- 按钮表完全由服务端逐玩家生成：按权限 / tag / 计分板 / 任务状态决定按钮是否出现，就是在调用本接口前自行算好 `buttons`，不需要客户端配合。
- 多层级对话 = 点击回传带回 `sceneName` + `buttonIndex` / `actionId`，调用方据此发送下一层。
- 载体实体是**纯协议合成**的 `minecraft:npc`（不进 BDS 实体系统），位置在世界下方 `y=-66`——客户端看不到实体，但界面里的头像照常渲染（改用隐形标志位反而会让头像一起消失）。只发给该玩家；点按钮 / 关闭 / 离线时发 `RemoveActor` 删掉。
- `NpcDialogSpec::rawActionJson` 非空时原样作为 `mActionJSON` 下发（原版按钮结构无公开文档，用于在游戏内实测字段格式）；`carrierIdentifier` 必须是 NPC 家族，换成村民等原版实体不行（实测要求）。
- **聊天框内显示的头像/模型可自定义（26.40.8）**：`NpcDialogSpec::avatarSkinVariant`（NPC **内置皮肤变体** 0..59，即 `NpcData.skin_list` 的 variant 值）—— 写进载体 ActorData 的 `SkinId(104)`，界面里的头像/模型换内置皮肤；`-1`（默认）不加该项（旧行为逐字节一致）。`NpcDialogSpec::avatarViewSpec` —— **在载体上叠一层 IViewOverride 语义**（spec 语法同 `viewEntity`）：`type=` 换载体类型（**实测: 僵尸/鸡这类非 NPC 家族照常弹界面**）、`name=` 换界面标题/交互文字。**`skin=` 分支已移除**（未发布批次内的收缩, 明记）: 它会把载体换成"玩家模型"（PlayerList+AddPlayer）, 实测客户端在该载体上点进菜单会关界面, 当前不可用；"让客户端认它是玩家"的路线由消费方自建（`playerNpcs()` + `npcUniqueIdOverride`）。要"整只模型完全自定义"也可用 `npcUniqueIdOverride` 路线：视图域 / 自定义实体生成展示实体、对话挂在它 id 上（1.27.0 的 `EntityView::skinId` 同-id 换模型也可用于那只展示实体）。LSE 侧就地修改用 `npcDialogSetAvatar`。

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

## 1.12 IContainerMenu（虚拟容器 / 列表界面，1.21.0）

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
    int openDelayTicks{7};             // 摆方块+方块实体后等多少 tick 再发 ContainerOpen
                                       // （GMLIB: 10 再等 4 补格 ≈700ms; 本库条目在方块实体 NBT 里
                                       // 只需 7 ≈350ms。实测 7 能开、6 开不了; 只影响"打开"那次）
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};       // 被点击的槽位（= items 的下标）
    bool        closed{false};  // true = 界面被关闭（此时 slot = -1）
};
```

## 1.13 感知域 / 千人千面 / 逐玩家变量全息（1.21.0）

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

## 1.14 IFakeInventory（背包虚容器，1.23.0）

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

## 1.15 ISulfurDisplay（硫磺立方体展示，1.23.0）

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

## 1.16 IViewOverride（客户端视图覆盖，1.25.0）

> **1.29.0（26.40.10，26.51 线随 `26.51.3`）补齐: `AddItemActor` 也纳入拦截。** 掉落物（item 实体）的出生包走的是
> `AddItemActor`(id 15) 而不是 `AddActor`(id 13)，此前只拦后者 → "把掉落物换类型/隐藏"只做一半:
> 设覆盖那一刻能把已经在看的玩家换掉，但引擎随后自己发的 `AddItemActor` 会把物品图标又塞回客户端
> （2026-10-16 抓包实证: `RemoveActor` + `AddActor(minecraft:cod)` 之后 13ms 就来一条同 uniqueId 的
> `AddItemActor`）。现在:
> * `hidden = true` → **丢掉**该掉落物的 `AddItemActor`（客户端从头到尾不会看到它）；
> * `identifier = "<生物>"` → 丢掉原包 + 用**同一个 runtimeId/uniqueId** 手写一只 `AddActor`（属性用
>   `health 20 / scale 1` 的已验证配方 —— **不照抄 ItemActor 的 `maxHealth = 0`**，那是另一处会渲染异常的点）；
> * 探测能力: LSE 侧新增 `viewItemSpawnHooked() -> bool`（老库没有这个名字，脚本 `ll.hasExported` 判 false）。
>
> 对 mob 的行为一字未改；对 item 实体是**修复**（此前半残）。

```cpp
auto& view = lib.viewOverrides();

// 实体: 换类型 / 换皮肤（我们的模型）/ 换名字牌 / 对该玩家隐藏（playerName 空串 = 所有玩家）
hologramlib::EntityView cow;
cow.identifier = "minecraft:cow";   // 短名会自动补 minecraft: 前缀
cow.hasNametag = true;
cow.nametag    = "§c奶牛";          // 出现 hasNametag 才改名字牌（值给空串 = 清空名字）
cow.nametagAlwaysShow = true;
view.overrideEntity("Steve", uniqueId, cow);

hologramlib::EntityView gone;
gone.hidden = true;                 // 让 Steve 看不到这只实体（阻断它的出生与更新包）
view.overrideEntity("Steve", uniqueId, gone);

// 非玩家实体: 换成"我们的模型"（已注册皮肤, 含自定义几何）—— **同一个 runtimeId/uniqueId**
hologramlib::EntityView skinned;
skinned.skinId = "ms_demo";         // IPlayerNpc 注册表里的皮肤 id
skinned.scale  = 0.8f;              // 可选: 模型缩放（客户端碰撞箱等比; 0 = 不缩放）
view.overrideEntity("", mobUniqueId, skinned);
// 客户端看到的是我们的模型; 服务端那边还是那只生物 —— 点它/打它/瞄准都是它本人。
// 撤销: view.clearEntity("", mobUniqueId);（库会重发真实生物的出生包, 立刻恢复原样）

// 玩家实体专用的两种"千人千面"（由 Steve 一个人看到）
hologramlib::EntityView skin;
skin.asPlayer = "Alex";             // 让 Steve 看到这只玩家变成 Alex 的样子（皮肤）
view.overrideEntity("Steve", playerUniqueId, skin);

hologramlib::EntityView asMob;
asMob.identifier = "minecraft:cow"; // 玩家模型渲染不了生物 → 库走"替换": 同一 runtimeId/uniqueId 发一只牛
view.overrideEntity("Steve", playerUniqueId, asMob);

// 方块: 把 (100,64,100) 在他客户端显示成钻石块
hologramlib::BlockView block;
block.type = "minecraft:diamond_block";
view.overrideBlock("Steve", 100, 64, 100, block);

view.clearEntity("Steve", uniqueId);   // 撤销
view.clearBlock("Steve", 100, 64, 100);
view.clearAll("Steve");                // 撤销这名玩家的全部覆盖（空串 = 全部玩家）
std::string s = view.describeFor("Steve"); // "entities=1 blocks=2"
```

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `overrideEntity(playerName, entityUniqueId, view)` | bool | false = 玩家不在线。实体还没出生也能先设（库在它出生时套用）。五项（`identifier` / `asPlayer` / `skinId` / `hidden` / `hasNametag`）都没设 = 当作撤销 |
| `overrideBlock(playerName, x, y, z, view)` | bool | false = 玩家不在线 / 类型名不在方块注册表里。type 给空串 = 撤销 |
| `clearEntity(playerName, entityUniqueId)` | bool | 撤销单个实体（返回是否原来有覆盖） |
| `clearBlock(playerName, x, y, z)` | bool | 撤销单个方块，并把**真实方块**推给该玩家 |
| `clearAll(playerName)` | void | 清掉该玩家（空串 = 全部）的覆盖表 |
| `describeFor(playerName)` | string | 该玩家当前生效的覆盖条数摘要（含全局那一份）；带拦截钩子计数与 `sendFail` |
| `startControl(controllerName, targetName, follow)` | bool | 让 controller 的移动/转向输入**驱动 target 的真实移动**（服务端位移）; `follow=true` 时 controller 被锁到 target 上一起走（附身观感） |
| `stopControl(controllerName)` | bool | 停止操控（任一方下线也会自动停） |

**机制（只拦不发）**：库拦截服务端**本来要发给这名玩家的包**，但**只决定放行 / 丢弃** ——
**从不修改引擎包字段**。需要"改"的内容一律：**丢掉原包 + 库自己手写协议包**补发
（sculk 构造 → 回读校验 → 原始字节发送）。换类型 = 丢掉原出生包、用同一 `runtimeId`/`uniqueId`
自己发一只（其余字段照抄原包）; 名字牌 = 原包照发、库自己的 `SetActorData` 紧随其后压过去;
方块覆盖 = 丢掉原 `UpdateBlock`、库自己发一条带覆盖网络 id 的。服务端世界 / 存档 / 碰撞 /
其他插件看到的都是真实内容。拦截挂在 `NetworkSystem::send` / `sendToMultiple`（BDS 按收件人发结构化包的
汇合点；实体出生包不走 `LoopbackPacketSender`，那是原先失效的原因），**外加一层心跳**：服务器每 tick
自查，实体"重新进入视野"、玩家换区块时把覆盖重新推一遍 —— 拦截漏掉的场合由它兜住。

**换"我们的模型"（`skinId`, 1.27.0; 只对非玩家实体）**：库吃掉这只实体的出生包, 用**同一个
`runtimeId`/`uniqueId`** 发 `PlayerList(Add)`（携带该皮肤）+ `AddPlayer` —— 客户端把它当玩家实体渲染
（自定义几何/贴图都走玩家皮肤通道, 不需要资源包）; 因为 **id 没变**, 服务端仍认它是那只生物,
**点它 / 打它 / 瞄准都是它本人**（伤害、掉落、其他插件看到的全是真身）。配套两件事: ①该实体的
`MoveActor*`(18/111) 对这些观看者被吃掉, 位置/朝向由库每 tick 用 `MoveActorAbsolute` 推（与
`IPlayerNpc::setPositionLight` 同一条轻推通道, 只在动了才发）; ②`PlayerList` 条目 ~20 tick 后摘掉
（不会一直挂在 Tab 里, 实体保留）。撤销 = `clearEntity`（重发真实生物的出生包）。
皮肤必须先在 `IPlayerNpc` 注册表里（`registerSkin` / `captureSkin` / `registerSkinFromBlob`）。

**自己不看自己**：覆盖目标是玩家时，**该玩家本人的客户端不参与覆盖**——不发/不吃关于他自己实体的包
（`RemoveActor` / 同 runtimeId 的 `AddActor` / 位移打回本地玩家会让客户端错乱甚至卡死，2026-10-09 实证）。
**例外：换肤类（`asPlayer`）含本人**——`PlayerList(Add)` 发给本人是"自己看自己被换肤"的正路
（第三人称/第一人称自视都变；同 `IPlayerNpc::injectSkin` 的约定）。

**逐玩家隔离与幂等**：覆盖表按玩家名 + 一份全局表；`sendToClients` / 广播这类"同一包发给多人"的路径
会展开成逐收件人判定 —— 只为让"放行 / 丢弃"逐人生效；因为从不改字段，不存在"后一个收件人看到前一个
覆盖"的串味。同一条包被多条发包路径各判一次，结果与判一次相同。库内没有任何覆盖时，钩子不做任何解析
就直接放行。

**玩家实体（1.25.0 支持）**：
- `asPlayer = "<在线玩家名>"` → 给该观看者重发一条 `PlayerList(Add)`（用该实体自己的 UUID + 源玩家的皮肤）
  —— 与原版"皮肤更新"同机制，**只发给该观看者**；撤销 = 用他自己的皮肤再发一次，立刻恢复。
  对*任何*实体只有玩家实体有效（皮肤按 UUID 认）。
- `identifier = "<生物>"` → 玩家模型渲染不了生物，所以库走**替换**（不是代理）：吃掉他的 `AddPlayer`，用**同一个 `runtimeId`/`uniqueId`** 发一只 `AddActor`；他的 `MovePlayer` 对这名观看者也吃掉，位置/朝向由库按他的输入包用 `MoveActorAbsolute` 推。**id 没变 → 服务端仍认他是那个真玩家，打他就是打他本人。** 撤销时把替代实体移走，**并把真身补发回该观看者**：`RemoveActor` → 用他自己身份的 `PlayerList(Add)`（皮肤走玩家皮肤通道）→ `AddPlayer`（位置/朝向取引擎当前值、游戏模式按他输入包里的 PlayMode）。所以撤销是**立刻**生效的，不用重进 / 换维度。皮肤数据读不到时库不动客户端（宁可先维持现状，也不做"先删后补不回来"）。

**26.40 做不到的**：逐玩家的**缩放 / 发光 / 隐身**没进 API —— 元数据表里没有 `scale`，
也没有 `glowing` / `invisible` 旗标（基岩版隐身是效果，走 `MobEffectPacket`）。
自定义实体的逐客户端缩放 / 装备槽请用 `ICustomEntity` 的 per-viewer 覆盖（1.21.0）。

**限制（写清楚免得误用）**：
- **换类型**只对非玩家生物生效（玩家出生包 `AddPlayer` 里没有类型字段；玩家实体的名字牌 / 隐藏照常可覆盖）。
- **换类型 / 隐藏 / 撤销全部立刻生效**：库里对这只实体做一次"移除 + 按目标状态重发一只"（同一个
  `runtimeId` 与 `uniqueId`，所以服务端后续的移动/状态更新照常作用在它身上）。代价是客户端那只实体
  会被重建 —— 只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步才回来。
- 撤销时离得太远（> 64 格）或已消失的实体不重建（客户端上本来也没有它）。玩家实体走"补发真身"那条路（见上）；补发出来的真身**手持物品是空的、能力是默认档**，引擎随后自己会纠正（切换物品会推 `MobEquipment`，其余状态变更会推 `SetActorData`）。
- 方块覆盖在区块重发后由库自动补发。
- 覆盖只存在内存里（重启服务器即失效）；按玩家名的覆盖在该玩家重进后仍然有效。
- 名字牌常显状态没有读口：撤销名字牌覆盖时按"不常显"推回真实名字，服务端下次因别的原因重发元数据会带回真实值。

**诊断**：`describeFor()` 的字符串里带拦截钩子的计数（`hooks(calls=… spawn=… meta=… block=… chunk=…)`）
与 sculk 发送失败数（`sendFail=`）——现象是"设了没反应"时先看它：`calls` 一直是 0 说明汇合点没被走到
（功能此时由心跳兜底，仍然生效，只是不即时）；`sendFail` 大于 0 说明有包构造不合法（应上报）。

判定逻辑（坐标打包 / 区块归属 / 幂等判断 / 名字牌计划 / 类型名清洗）独立在
`src/view/ViewOverrideLogic.h`，离线可测：`tests\check-view-override.bat`（59 项）。

---

## 1.17 IItemActor / IFormControl / ILocatorBar（26.51.6）

四个接口都很小，各自的说明见 [掉落物](../domains/item-actor.md) / [表单控制](../domains/form.md) / [方位条](../domains/locator-bar.md) / [玩家出站包控制](../domains/visibility.md)，这里只列签名。

**IItemActor** —— 协议层掉落物（`AddItemActor` / `SetActorMotion` / `TakeItemActor` / `RemoveActor`）：

```cpp
struct ItemActorSpec {
    std::string itemId; int aux{0}; int count{1};
    float x{0}, y{0}, z{0}; int dimension{0};
    float vx{0}, vy{0}, vz{0};
};

IItemActor& itemActors();
int64_t spawn(ItemActorSpec const& spec);   // 只发给同维度; 该维度没人在线 = -1
bool    setMotion(int64_t id, float vx, float vy, float vz); // 飞行途中改速度/方向
bool    moveTo(int64_t id, float x, float y, float z);
bool    take(int64_t id, std::string const& playerName);     // 播拾取动画后删除, 不给物品
bool    destroy(int64_t id);
void    destroyAll();
bool    exists(int64_t id) const;
std::vector<int64_t> getAllIds() const;
```

物品走数字 id + count + aux，**不带 UserData**（原因见 [掉落物](../domains/item-actor.md)）。

**IFormControl** —— 强制关掉客户端表单（`ClientboundCloseForm`）：

```cpp
IFormControl& formControl();
bool closeForm(std::string const& playerName);
int  closeAllForms();
```

**ILocatorBar** —— 方位条航点（`LocatorBar`）：

```cpp
ILocatorBar& locatorBar();
bool setWaypoint(playerName, key, x, y, z, dimension, color = -1, visible = true);
bool removeWaypoint(playerName, key);
int  clearWaypoints(playerName);
int  waypointCount(playerName) const;
bool hideFromOthers(playerName, hidden);   // 转给视图覆盖域的 hidden 通道
bool isHiddenFromOthers(playerName) const;
```

**IPlayerVisibility** —— 玩家出站包控制（`AddPlayer` / `PlayerList` / `PlayerSkin` / `MovePlayer` /
`MoveActorAbsolute` / `SetActorMotion` / `SetActorData` / `Animate` / `ActorEvent` / `MobEffect`）：

```cpp
struct PlayerPacketMask {
    bool spawn{false}; bool tab{false}; bool motion{false}; bool state{false}; bool skin{false};
};

IPlayerVisibility& playerVisibility();
bool setMask(playerName, PlayerPacketMask const& mask, bool alsoSelf = false);
bool setHidden(playerName, hidden);   // 整体隐身（复用视图覆盖域 hidden 通道）
bool restore(playerName);             // 清掩码 + 清隐身 + 立刻补发出生包
PlayerPacketMask maskOf(playerName) const;
bool isHidden(playerName) const;
```

**既有接口的尾部追加**：`IContainerMenu::blockEvent(menuId, b0, b1)`（`BlockEvent`，假箱子的方块事件）、
`IPlayerNpc::playEmote(id, pieceId, lengthTicks)`（`Emote`，原版表情动作）。

这一版虚表尾部变长（3 个新接口 + 两个方法），`HOLOGLIB_ABI_STAMP` 仍为 1，
但用旧头文件编译的消费方要重新编译；基线是 `abi/v1.32.0.json`。
