// HologramLib.h - 统一悬浮显示库对外 C++ 接口（唯一公开头）
//
// 消费方式（native 插件）:
//   1. xmake: add_includedirs("../HologramLib/include") + add_linkdirs(...) + add_links("HologramLib")
//   2. #include "hologramlib/HologramLib.h"
//   3. auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
//
// LSE 脚本经 ll.import("HologramLib", "shape*/holo*/gradient*/itemDetail*") 统一命名空间调用，
// 由库内 LseBridge 在运行时检测 LegacyRemoteCall 是否存在（可选，无前置依赖）。
//
// - 接口对象全部在 HologramLib.dll 内创建/销毁, 消费者只持有引用, 不跨边界 new/delete
// - 所有字符串 UTF-8
// - 接口方法线程安全（内部互斥）; 发包在调用线程执行, 建议主线程调用
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// 库 API 版本（与 IHologramLib::version() 同值; 编码规律与完整对照见 VERSION-HISTORY.md）
//   中间字节 = 次版本号, 按十六进制递增: 1.15.0 -> 0x011500, 1.19.0 -> 0x011900,
//   1.20.0 -> 0x011A00, 1.21.0 -> 0x011B00（补丁位通常为 00）
// 消费方可用于编译期静态断言最低版本要求。
// 注意: 只有**正式发布新版本**才推高本宏; 在同一条尚未发布的线上继续加能力域时不改变它 ——
// 本值 0x011B00 随 26.40.3 正式发布（此前已发布的最高值是 26.40.2 的 0x011A00）。
#define HOLOGLIB_API_VERSION 0x011B00

#ifdef HOLOGLIB_EXPORTS
#define HOLOGLIB_API __declspec(dllexport)
#else
#define HOLOGLIB_API __declspec(dllimport)
#endif

namespace hologramlib {

// 单玩家朝向（度; 逐客户端朝向功能用）
// Bedrock yaw 约定: 0°=南(+Z) 90°=西(-X) 180°=北(-Z) 270°=东(+X); 前向 = (-sin yaw, cos yaw)
struct PerPlayerRotation {
    float yaw{0};
    float pitch{0};
};

// 形状类型（与 LSE 导出的 DebugShape::getShapeType 数值一致）
enum class ShapeType : int {
    Text   = 0,
    Line   = 1,
    Box    = 2,
    Circle = 3,
    Sphere = 4,
    Arrow  = 5
};

// ─────────────────────────────────────────────
// 形状渲染（原 DebugShape-Protocol 能力域）
// 坐标为世界坐标; 颜色各分量 0.0~1.0
// ─────────────────────────────────────────────
class IShapeDrawer {
public:
    virtual ~IShapeDrawer() = default;

    // 创建（返回形状 ID, <=0 为失败）
    virtual int64_t createText(float x, float y, float z, std::string const& text)          = 0;
    virtual int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2)  = 0;
    virtual int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2)   = 0;
    virtual int64_t createCircle(float x, float y, float z, float scale)                    = 0;
    virtual int64_t createSphere(float x, float y, float z, float scale)                    = 0;
    virtual int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2) = 0;

    // 属性
    virtual bool setColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool setScale(int64_t id, float scale)                        = 0;
    virtual bool setDuration(int64_t id, float seconds)                   = 0;
    virtual bool setDimension(int64_t id, int dimId)                      = 0;
    virtual bool setLocation(int64_t id, float x, float y, float z)       = 0;
    virtual bool setText(int64_t id, std::string const& text)             = 0;
    virtual bool setRotation(int64_t id, float pitch, float yaw, float roll) = 0; // 弧度; billboard 模式前先 setRotation 固定朝向
    virtual bool clearRotation(int64_t id)                               = 0;

    // 显示控制
    virtual bool draw(int64_t id)                                        = 0; // 全维度可见者
    virtual bool drawToPlayer(int64_t id, std::string const& playerName) = 0;
    virtual bool drawToDimension(int64_t id, int dimId)                  = 0;
    virtual bool remove(int64_t id)                                      = 0; // 隐藏（保留数据）
    virtual bool update(int64_t id)                                      = 0; // 可见时原地重发（同 networkId 覆盖, 无闪烁）

    // 生命周期
    virtual bool destroy(int64_t id) = 0;
    virtual void destroyAll()        = 0;
    virtual bool exists(int64_t id)  = 0;
    virtual ShapeType type(int64_t id) = 0;
};

// ─────────────────────────────────────────────
// 悬浮字 / 全息（多行、渐变、滚动、跟随、动态变量）
// 实现"整块文本单一背景框"渲染（非逐字符分框）
// ─────────────────────────────────────────────
class IHologramText {
public:
    virtual ~IHologramText() = default;

    virtual int64_t create(float x, float y, float z)      = 0;
    virtual bool    destroy(int64_t id)                    = 0;
    virtual void    destroyAll()                           = 0;

    // 行管理（行索引 0 起）
    virtual bool addLine(int64_t id, std::string const& text)              = 0;
    virtual bool setLineText(int64_t id, int lineIndex, std::string const& text) = 0;
    virtual bool setLineScale(int64_t id, int lineIndex, float scale)      = 0;
    virtual bool removeLine(int64_t id, int lineIndex)                     = 0;
    virtual bool clearLines(int64_t id)                                    = 0;
    virtual int  getLineCount(int64_t id)                                  = 0;

    // 颜色（纯色/双色渐变/彩虹）
    virtual bool setColor(int64_t id, float r, float g, float b, float a)  = 0;
    virtual bool setLineColor(int64_t id, int lineIndex, float r, float g, float b, float a) = 0;
    virtual bool setLineGradient(
        int64_t id,
        int     lineIndex,
        float   r1,
        float   g1,
        float   b1,
        float   r2,
        float   g2,
        float   b2
    )                                                                                     = 0;
    virtual bool setLineRainbow(int64_t id, int lineIndex, float speed)     = 0;

    // 动画（滚动方向 0=无 1=左 2=右; 垂直动画 0=无 1=弹跳 2=滚动）
    virtual bool setLineScroll(int64_t id, int lineIndex, int direction, float speed) = 0;
    virtual bool setVerticalAnimation(int64_t id, int type, float speed, float range) = 0;
    virtual bool setLineSpacing(int64_t id, float spacing)                  = 0;

    // 位置与跟随
    virtual bool setLocation(int64_t id, float x, float y, float z)         = 0;
    virtual bool setFollowPlayer(int64_t id, std::string const& playerName, float offsetY) = 0;
    virtual bool clearFollowPlayer(int64_t id)                              = 0;

    // 显示
    virtual bool draw(int64_t id)                                           = 0;
    virtual bool drawToDimension(int64_t id, int dimId)                     = 0;
    virtual bool drawToPlayer(int64_t id, std::string const& playerName)    = 0;
    virtual bool remove(int64_t id)                                         = 0;
    virtual bool refresh(int64_t id)                                        = 0; // 重新解析变量并原地重发

    // 动画推进（滚动偏移/跟随位置更新; 不自动重绘, 由调用方按需 refresh）
    virtual void tick(float deltaTime)                                      = 0;

    // ── 1.12.0 追加（冻结契约: 只在尾部追加）──
    // 迁移维度: 已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁）
    virtual bool setDimension(int64_t id, int dimId)                        = 0;


};

// ─────────────────────────────────────────────
// 物品详情显示（原 itemdetail 能力域: 掉落物/商店等场景的"物品名 ×数量"悬浮）
// ─────────────────────────────────────────────
class IItemDetail {
public:
    virtual ~IItemDetail() = default;

    // 在指定位置显示物品详情（自动翻译物品名; count<=1 时不带数量后缀）
    // 返回详情 ID（内部即悬浮字 ID, 可继续用 holograms() 精修）
    virtual int64_t show(
        int                 dimId,
        float               x,
        float               y,
        float               z,
        std::string const&  itemId,
        int                 aux,
        int                 count,
        std::string const&  customText = ""
    ) = 0;

    // customText 非空时完全替代自动文本（支持 § 颜色码与 {变量}）
    virtual bool hide(int64_t id) = 0;
};

// ─────────────────────────────────────────────
// 物品悬浮显示（FMBE 狐狸+发包技术; 1.6.0 追加）
// 用隐形狐狸手持物品渲染任意物品/方块的悬浮展示,
// 三轴旋转/函数平移/缩放全部支持 Molang 表达式
// ─────────────────────────────────────────────
struct ItemDisplayConfig {
    std::string item{"minecraft:diamond"}; // 显示的物品标识符
    int         itemAux{0};                // 物品附加值（data 值）
    float       x{0}, y{64}, z{0};         // 世界坐标
    int         dimension{0};              // 维度 ID
    // 平移（函数平移：常量数字或 Molang 表达式）
    std::string offsetX{"0"};              // v.xpos   模型单位平移 X
    std::string offsetY{"-4"};             // v.ypos   模型单位平移 Y（物品模式默认 -4）
    std::string offsetZ{"0"};              // v.zpos   模型单位平移 Z
    std::string baseOffsetX{"0"};          // v.xbasepos 渲染像素基础偏移 X
    std::string baseOffsetY{"0"};          // v.ybasepos 渲染像素基础偏移 Y
    std::string baseOffsetZ{"0"};          // v.zbasepos 渲染像素基础偏移 Z
    // 三轴旋转（度, 支持 Molang 表达式）
    std::string rotX{"180"};               // v.xrot 俯仰（物品模式默认 180 = 水平放置）
    std::string rotY{"0"};                 // v.yrot 偏航（物品模式内部自动 +205 补偿狐狸头朝向）
    std::string rotZ{"180"};               // v.zrot 翻滚（物品模式默认 180 = 正面朝上）
    // 缩放
    std::string scale{"0.375"};            // v.scale（物品模式默认 0.375; 方块模式建议 0.5）
    // 方块模式附加变换（仅 blockMode 生效）
    std::string extendScale{"1"};          // v.extend_scale 二段缩放
    std::string extendRotX{"-90"};         // v.extend_xrot   二段旋转 X
    std::string extendRotY{"0"};           // v.extend_yrot   二段旋转 Y
    // 行为
    int    mode{0};                        // 0=auto（按物品 3D/2D 自动） 1=item 2=block
    double viewDistance{64.0};             // 可见距离（方块; <=0 无限制）
    bool   enabled{true};
    // 物品附加数据（SNBT 字符串; 空 = 无; 1.8.0 追加）
    // 携带自定义名称等用户数据; 由消费者从手持物品快照或手写 SNBT
    std::string itemNbt{};
    // 附魔光效开关（1.9.0 追加）: true = 经 BDS 原生 saveEnchantsToUserData
    // 注入 1 级锋利（仅取光效）; 与 itemNbt 独立叠加
    bool itemGlint{false};
    // ── 1.17.0 追加（冻结契约: 结构尾部追加）──
    // AABB 判定体积（SetActorData R53=Width R54=Height; 0/0 = 无判定体积不可命中,
    // 默认值与历史行为一致）。> 0 时客户端射线可命中 → 攻击经 ghost 交互路由回库内 id
    float hitboxWidth{0.0f};
    float hitboxHeight{0.0f};
};

class IItemDisplay {
public:
    virtual ~IItemDisplay() = default;

    // 生命周期（id 驱动; 创建失败返回 < 0; 持久化由消费者负责）
    virtual int64_t create(ItemDisplayConfig const& config) = 0;
    virtual bool    destroy(int64_t id)                     = 0;
    virtual void    destroyAll()                            = 0;
    virtual bool    exists(int64_t id) const                = 0;
    virtual bool    get(int64_t id, ItemDisplayConfig& out) const = 0; // 拷贝输出当前配置

    // 属性（变换字段为常量数字或 Molang 表达式字符串）
    virtual bool setItem(int64_t id, std::string const& item, int aux) = 0;
    virtual bool setPosition(int64_t id, float x, float y, float z, int dim) = 0; // dim<0 仅改坐标
    virtual bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) = 0;
    virtual bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) = 0;
    virtual bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) = 0;
    virtual bool setScale(int64_t id, std::string const& scale) = 0;
    virtual bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) = 0;
    virtual bool setMode(int64_t id, int mode)          = 0;
    virtual bool setEnabled(int64_t id, bool enabled)   = 0;
    virtual bool setViewDistance(int64_t id, double dist) = 0;
    virtual bool rotateY(int64_t id, float delta)       = 0; // 在现有 rotY 上叠加增量

    virtual std::vector<int64_t> getAllIds() const      = 0;

    // ── 1.7.0 追加（冻结契约: 只在尾部追加）──
    // 随机 ID 创建: ID 由库在随机段 [0x10000000, 0x7FFFFFFF) 自动生成（查重保证不与现有冲突,
    // 且与自增段长期隔离）; 成功返回生成的 ID, 失败返回 < 0
    virtual int64_t createRandom(ItemDisplayConfig const& config) = 0;
    // 指定 ID 创建: 用于持久化恢复（如随机 ID 重启后原位还原）; desiredId <= 0 或已被占用返回 -2
    virtual int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId) = 0;
    // 查询 ID 是否在用
    virtual bool isIdUsed(int64_t id) const = 0;

    // ── 1.7.1 追加（冻结契约: 只在尾部追加）──
    // 相对缩放（放大/缩小）: 在现有 scale 上乘以 factor
    // 常量缩放直接相乘, 表达式缩放包裹 (expr)*factor
    // factor<=0 或 id 不存在返回 false
    virtual bool scaleBy(int64_t id, double factor) = 0;

    // ── 1.8.0 追加（冻结契约: 只在尾部追加）──
    // 换物品（带附加数据）: nbt 为 SNBT 字符串（附魔/自定义名称等用户数据）,
    // 空串 = 清除附加数据; SNBT 解析失败按无 NBT 处理并告警; id 不存在返回 false
    virtual bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt) = 0;

    // ── 1.9.0 追加（冻结契约: 只在尾部追加）──
    // 附魔光效开关: 开 = BDS 原生路径注入 1 级锋利（客户端紫色光效）,
    // 关 = 移除附魔; 幂等（值未变不重发）; id 不存在返回 false
    virtual bool setGlint(int64_t id, bool on) = 0;

    // ── 1.17.0 追加（冻结契约: 只在尾部追加）──
    // 展示跟随玩家: 每 tick 读取目标玩家实时坐标（含跨维度自动 respawn）,
    // 对已见玩家发 MoveActorAbsolute（非 teleport 标志, 客户端插值）驱动狐狸载体
    // 平滑位移 —— 全程无 respawn、无 Remove/Add、无闪烁。
    // 目标玩家下线自动解除跟随（方块原地保留）; setPosition 手动设位解除跟随。
    // playerName 按玩家名（Player::getRealName 即 LSE realName）匹配; id 不存在返回 false
    virtual bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ) = 0;
    // 解除跟随（方块保留在当前位置）; 无跟随关系时返回 true
    virtual bool unfollow(int64_t id) = 0;
    // AABB 判定体积: 对已见玩家广播 SetActorDataPacket(R53/R54), 轻量即时无 respawn;
    // 数值持久入配置（后续 respawn 自动按当前值发包）; width/height=0 恢复不可选中;
    // id 不存在返回 false
    virtual bool setHitbox(int64_t id, float width, float height) = 0;
};

// ─────────────────────────────────────────────
// 自定义实体协议层生成（任意类型实体; 1.10.0 追加）
// AddActorPacket 直发客户端生成纯视觉实体, 不占服务端实体系统;
// 适合 NPC 壳 / 装饰生物 / 盔甲架布景等（不可交互, 无碰撞）
// ─────────────────────────────────────────────
struct CustomEntityEquipment {
    std::string name;    // item name（空 = 空槽位）
    int         aux{0};
    std::string nbt{};   // SNBT 形式, 空 = 无附加 NBT
};

struct CustomEntityConfig {
    std::string identifier{"minecraft:armor_stand"}; // 实体类型标识符（短名自动补 minecraft:）
    float       x{0}, y{64}, z{0};                   // 世界坐标
    int         dimension{0};                        // 维度 ID
    float       yaw{0}, pitch{0};                    // 朝向（度; 头/身旋转同值）
    std::string nametag{};                           // 头顶名字（支持 § 颜色码; 空 = 无）
    bool        nametagAlwaysShow{false};            // 名字常显开关（默认 false = 准星对准才显示）
    float       scale{1.0f};                         // 实体缩放（客户端有效域 0.0625~10, 自动钳制）
    int         variant{0};                          // 变种（皮肤/亚种, 依实体定义）
    int         markVariant{0};                      // 二级变种
    int         colorIndex{0};                       // 颜色索引（羊/项圈等染色实体）
    std::int64_t flags{0};                           // 原始 flags 位掩码（0x01=着火 0x20=隐身 等）
    bool        invisible{false};                    // 隐身便捷开关（与 flags 独立, 发包时合成 0x20）
    double      viewDistance{64.0};                  // 可见距离（方块; <=0 无限制）
    bool        enabled{true};
    int         pose{0};                             // 盔甲架 / 玩家 PoseIndex (ActorDataIDs::PoseIndex) 0=Standing 1=NoBasePlate 2=ShowArms 3..13 坐姿/睡姿/跳舞
    // 装备槽位（slot 编码与 MobEquipmentPacket 一致: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet）
    CustomEntityEquipment equipment[6];
    // ── 1.12.0 追加（冻结契约: 结构尾部追加）──
    // 骑乘链接目标（SetActorLinkPacket; 两者互斥, 后设置者生效; 空名/0 = 无链接）
    std::string ridePlayerName{};   // 实体骑到指定玩家头上（玩家为载具; 须在线）
    int64_t     rideEntityId{0};    // 实体骑到另一自定义实体上（对方库内 id 为载具）
};

class ICustomEntity {
public:
    virtual ~ICustomEntity() = default;

    // 生命周期（id 驱动; 创建失败返回 < 0; 持久化由消费者负责）
    virtual int64_t create(CustomEntityConfig const& config) = 0;
    virtual int64_t createRandom(CustomEntityConfig const& config) = 0;  // 随机段 ID
    virtual int64_t createWithId(CustomEntityConfig const& config, int64_t desiredId) = 0; // <=0/占用返回 -2
    virtual bool    destroy(int64_t id)                     = 0;
    virtual void    destroyAll()                            = 0;
    virtual bool    exists(int64_t id) const                = 0;
    virtual bool    get(int64_t id, CustomEntityConfig& out) const = 0;  // 拷贝输出当前配置
    virtual bool    isIdUsed(int64_t id) const              = 0;
    virtual std::vector<int64_t> getAllIds() const          = 0;

    // 属性（变更经 tick 脏刷新合并为单次 respawn, 无闪烁串台）
    virtual bool setIdentifier(int64_t id, std::string const& identifier) = 0;
    virtual bool setPosition(int64_t id, float x, float y, float z, int dim) = 0; // dim<0 仅改坐标
    virtual bool setRotation(int64_t id, float yaw, float pitch) = 0;
    virtual bool setNametag(int64_t id, std::string const& text) = 0;  // 空串清除
    virtual bool setScale(int64_t id, float scale)          = 0;       // <=0 拒绝
    virtual bool setVariant(int64_t id, int variant)        = 0;
    virtual bool setMarkVariant(int64_t id, int markVariant) = 0;
    virtual bool setColorIndex(int64_t id, int colorIndex)  = 0;
    virtual bool setFlags(int64_t id, std::int64_t flags)   = 0;       // 原始位掩码
    virtual bool setInvisible(int64_t id, bool on)          = 0;       // 便捷开关（幂等）
    virtual bool setEnabled(int64_t id, bool enabled)       = 0;
    virtual bool setViewDistance(int64_t id, double dist)   = 0;
    // 盔甲架/玩家姿态（PoseIndex 0..13）
    virtual bool setPose(int64_t id, int pose)              = 0;
    // 装备槽位: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet；name 空清空槽位
    virtual bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) = 0;

    virtual int64_t findNearest(float x, float y, float z, int dim, double maxDist) const = 0; // 无匹配 -1

    // ── 1.12.0 追加（冻结契约: 只在尾部追加）──
    // 相对缩放: 在现有 scale 上乘以 factor（结果自动钳制 0.0625~10）
    // factor<=0 或 id 不存在返回 false
    virtual bool scaleBy(int64_t id, double factor) = 0;
    // 可见玩家白名单（仅指定玩家可见; 按 Player::getRealName 即 LSE realName 匹配）
    // setVisiblePlayers 空列表 = 清除限制 = 全员可见
    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) = 0;
    virtual bool clearVisiblePlayers(int64_t id) = 0;
    virtual bool setVisiblePlayer(int64_t id, std::string const& playerName) = 0;
    // 诊断探针: 返回实体运行态摘要字符串（找不到返回 "not_found"）
    virtual std::string getDebugInfo(int64_t id) const = 0;
    // 骑乘链接（SetActorLinkPacket; 变更经 respawn 重放链接; 两者互斥, 后设者生效）
    virtual bool setRidePlayer(int64_t id, std::string const& playerName) = 0; // 骑到指定玩家头上（须在线）
    virtual bool setRideEntity(int64_t id, int64_t vehicleEntityId) = 0;      // 骑到另一自定义实体上
    virtual bool clearRide(int64_t id) = 0;
    // 播放原版动画（AnimateEntityPacket; controller 名库内按实体 id 自动唯一化）
    // stopExpression 空串 = 常驻; durationTicks>0 时到期自动停止; id 不存在返回 false
    // 注意: 一次性发包, 不持久化 —— 无观察者时返回 false; 新观察者的补发由消费方经 spawn 回调自行处理
    virtual bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks
    ) = 0;
    // 对指定玩家单发该实体的动画包（spawn 回调里补发用; 玩家未在线/未见过该实体返回 false）
    virtual bool playAnimationTo(
        int64_t id, std::string const& playerName,
        std::string const& animation, std::string const& stopExpression, int durationTicks
    ) = 0;
    // ── 1.19.0: 实体 spawn 时机通知（纯通知, 无库内状态; 补发决策与数据归消费方）──
    // 实体对某玩家 spawn/respawn 完成后回调; 消费方可在此用 playAnimationTo 补发自己的持久数据
    using EntitySpawnCallback = std::function<void(int64_t id, std::string const& playerName)>;
    virtual void setEntitySpawnCallback(EntitySpawnCallback callback) = 0; // 传 nullptr 清除

    // ── 1.20.0 追加: 逐客户端朝向（每个观察者看到不同朝向; "看向自己"玩法）──
    // 覆盖指定玩家收到的该实体 yaw/pitch（AddActor 出生包与后续增量包都按覆盖值下发）;
    // 未覆盖的玩家仍用 config 朝向; 玩家离线/未见过该实体返回 false。
    // 变更走轻脏增量（不发 RemoveActor, 无闪烁）, 需在下一 tick 生效。
    virtual bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw, float pitch) = 0;
    // 清除单个玩家的朝向覆盖（回到 config 朝向）
    virtual bool clearPlayerRotation(int64_t id, std::string const& playerName) = 0;
    // 清除该实体全部玩家的朝向覆盖（关闭逐客户端朝向时调用）
    virtual bool clearPlayerRotations(int64_t id) = 0;

    // ── 1.21.0 追加: 千人千面（按观看者覆盖外观字段; "千人千面实体"）──
    // 同一套机制与逐客户端朝向一致: 覆盖值按玩家 uuid 保存, 出生包与增量包都按覆盖值下发;
    // 未覆盖的玩家仍用 config 值。装备变更即时单发（不 respawn, 无闪烁）。
    // 覆盖名字（text 空 = 清除该玩家的覆盖, 回到 config 名字牌）
    virtual bool setPlayerNametag(int64_t id, std::string const& playerName, std::string const& text) = 0;
    // 覆盖缩放（<=0 = 清除覆盖; 有效域 0.0625~10）
    virtual bool setPlayerScale(int64_t id, std::string const& playerName, float scale) = 0;
    // 覆盖单个装备槽（slot: 0=主手 1=副手 2=头 3=胸 4=腿 5=脚; name 空 = 该槽回退 config）
    virtual bool setPlayerEquipmentSlot(
        int64_t id, std::string const& playerName, int slot, std::string const& name, int aux, std::string const& nbt
    ) = 0;
    // 清除该玩家对实体的全部外观覆盖（名字/缩放/装备; 朝向覆盖单独由 clearPlayerRotation 管理）
    virtual bool clearPlayerAppearance(int64_t id, std::string const& playerName) = 0;
};
// ─────────────────────────────────────────────
// 通用协议层粒子形状系统（1.14.0 追加; 1.15.0 发送通道升级 + moveTo）
// 点/线/矩形环/填充面/长方体框/六面/多面体 + 平移/平滑移动/旋转/自旋/缩放/跟随;
// 批量并发发送: 逐玩家 vanilla SpawnParticleEffectPacket 经 NetworkSystem 入队,
// BDS tick flush 自动聚合压缩单 Batch 数据报（与原版粒子广播同路径）;
// 采样/视距裁剪/周期重发由库内 tick 自驱动, 消费者只管创建与控制
// ─────────────────────────────────────────────
class IParticleShape {
public:
    virtual ~IParticleShape() = default;

    // ── 创建（世界坐标; 返回形状 id, <0 = 失败; lifetimeTicks 0 = 永久）──
    // intervalTicks = 周期整批重发间隔（粒子瞬态, 靠重发维持常驻视觉）
    virtual int64_t createPoint(
        std::string const& owner, int dimId,
        float x, float y, float z,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    virtual int64_t createLine(
        std::string const& owner, int dimId,
        float x1, float y1, float z1, float x2, float y2, float z2, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    // axis: 0=XY 1=YZ 2=XZ（w/h 沿平面两轴; rect=环线, plane=填充网格）
    virtual int64_t createRect(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    virtual int64_t createPlane(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    // hx/hy/hz = 半尺寸; box=12 边线框, boxFaces=六面填充
    virtual int64_t createBox(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    virtual int64_t createBoxFaces(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;
    // 多面体: verts 为顶点数组 (x,y,z)*N; edges 为顶点索引对 (i,j)*M
    // （锚点 = 质心, 旋转绕质心; 本地拷贝存储, 调用后可释放）
    virtual int64_t createPoly(
        std::string const& owner, int dimId,
        std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) = 0;

    // ── 智能控制 ──
    // 平移锚点（= 粒子移动: 下次发射整批重发在新位置）; 同时解除跟随
    virtual bool setPos(int64_t id, float x, float y, float z) = 0;
    virtual bool moveBy(int64_t id, float dx, float dy, float dz) = 0;
    // 欧拉角（度, ZYX 序, 绕形状锚点）
    virtual bool setRot(int64_t id, float rx, float ry, float rz) = 0;
    // 自旋速率（度/tick; 0,0,0 停止）
    virtual bool spin(int64_t id, float sx, float sy, float sz) = 0;
    virtual bool setScale(int64_t id, float scale) = 0;
    // 锚点跟随玩家位置 + 偏移（每 tick 自动更新, 跨维度自动跟随）
    virtual bool follow(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) = 0;
    virtual bool unfollow(int64_t id) = 0;

    // ── 渲染 / 可见性 / 生命周期 ──
    virtual bool setEffect(int64_t id, std::string const& effect) = 0;
    // 白名单（玩家 UUID）; 空列表 = 形状所在维度全员可见
    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids) = 0;
    virtual bool clearVisiblePlayers(int64_t id) = 0;
    virtual bool setInterval(int64_t id, int ticks)        = 0;
    virtual bool setViewDistance(int64_t id, int blocks)   = 0; // 逐玩家 3D 裁剪; 0 = 不裁剪
    virtual bool setLifetime(int64_t id, int ticks)        = 0; // 从现在起; 0 = 永久

    // ── 生命周期 ──
    virtual bool destroy(int64_t id)        = 0;
    virtual void destroyAll()               = 0;
    virtual bool exists(int64_t id) const   = 0;
    virtual std::vector<int64_t> getAllIds() const = 0;
    // 诊断探针: 运行态摘要字符串（找不到返回 "not_found"）
    virtual std::string getDebugInfo(int64_t id) const = 0;

    // ── 1.15.0 追加（冻结契约: 只在尾部追加）──
    // 平滑点对点移动: 锚点 easeOutCubic 插值逼近目标（单点/整面/整体形状均适用）; 解除跟随
    // durationTicks <= 0 = 立即到达（等价 setPos）
    virtual bool moveTo(int64_t id, float x, float y, float z, int durationTicks) = 0;
};

// ─────────────────────────────────────────────
// ghost 交互事件（1.12.0）: 玩家点击库内协议层实体（CustomEntity / ItemDisplay）
// 客户端会对"协议上存在"的实体发 InteractPacket; 库 hook 收包后将 runtimeId
// 反查回库内 id 并派发, 实现"可点击 NPC / 全息菜单"
// ─────────────────────────────────────────────
struct GhostInteractEvent {
    std::string playerName;   // 点击者（realName）
    int         action{0};    // InteractPacket Action 原始值（1=Interact 2=Attack 3=StopRiding 4=InteractUpdate 5=NpcOpen 6=OpenInventory）
    std::string domain;       // "entity" / "itemDisplay" / "npc"
    int64_t     id{-1};       // 对应域的库内 id
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};

// ─────────────────────────────────────────────
// 假玩家 NPC（自定义皮肤; 1.16.0 追加）
// PlayerListPacket + AddPlayerPacket 纯协议假玩家:
// 不占服务端实体系统（无 AI/碰撞/寻路/存档开销）,
// 完整 3D 玩家模型 + 任意自定义皮肤, 点击经 ghost 交互派发
// ─────────────────────────────────────────────

// 皮肤注册入参（PNG 文件路径方式; 几何/手臂尺寸等可自定义）
struct PlayerNpcSkin {
    std::string pngPath{};     // PNG 文件路径（64x64 / 128x128）
    std::string skinId{};      // 注册名（空 = 用 pngPath 文件名）
    // 几何自定义（resourcePatch; 默认标准玩家模型）
    std::string geometry{"geometry.humanoid.custom"};
    // 手臂尺寸: "wide"（粗） / "slim"（细）; 默认 wide
    std::string armSize{"wide"};
    // 完整几何 JSON 内容（1.18.0; 可选, 提供时启用自定义模型, identifier 自动从 JSON 提取）
    std::string geometryData{};
};

struct PlayerNpcConfig {
    std::string name{"NPC"};         // 显示名（nametag 同步; 即 PlayerList 条目名）
    std::string skinId{"default"};   // 已注册皮肤 ID（未注册 = 创建失败 -3）
    float       x{0}, y{64}, z{0};    // 世界坐标
    int         dimension{0};        // 维度 ID
    float       yaw{0};              // 朝向（度; 头/身旋转同值）
    double      viewDistance{96.0};  // 可见距离（方块; <=0 无限制）
    float       scale{1.0f};          // 模型缩放（1.19.0; 0.0625~10 客户端硬限, 碰撞箱等比）
    bool        enabled{true};
};

// 注意（26.40, 未解决）：本域全部接口可用且数据链路完整——皮肤注册/在线采集/目录导入、
// getSkinBlob/registerSkinFromBlob 导出恢复、创建/移动/朝向/缩放/视距/显隐/交互都正常，
// 但客户端目前不渲染所设置的皮肤，NPC 外观回退为默认模型（即"设置生效、显示无效"）。
class IPlayerNpc {
public:
    virtual ~IPlayerNpc() = default;

    // ── 皮肤注册表（全局; NPC 引用 skinId, 解码一次多处复用）──
    // PNG 注册（64/128; 失败返回 false 并给出错误; 重复注册覆盖）
    virtual bool registerSkin(PlayerNpcSkin const& skin) = 0;
    // 从在线玩家采集当前皮肤（含几何/披风/动画全部字段）→ 以 skinId 永久注册
    // 玩家不在线返回 false; 重复 skinId 覆盖
    virtual bool captureSkin(std::string const& skinId, std::string const& playerName) = 0;
    virtual bool hasSkin(std::string const& skinId) const = 0;
    virtual bool unregisterSkin(std::string const& skinId) = 0; // 有 NPC 引用时拒绝并返回 false
    virtual std::vector<std::string> getSkinIds() const = 0;

    // ── 生命周期（id 驱动; 创建失败返回 < 0; 持久化由消费者负责）──
    // 返回: <0 失败 -3=皮肤未注册
    virtual int64_t create(PlayerNpcConfig const& config) = 0;
    virtual int64_t createRandom(PlayerNpcConfig const& config) = 0;  // 随机段 ID
    virtual int64_t createWithId(PlayerNpcConfig const& config, int64_t desiredId) = 0; // <=0/占用返回 -2
    virtual bool    destroy(int64_t id) = 0;
    virtual void    destroyAll() = 0;
    virtual bool    exists(int64_t id) const = 0;
    virtual bool    get(int64_t id, PlayerNpcConfig& out) const = 0;   // 拷贝输出当前配置
    virtual bool    isIdUsed(int64_t id) const = 0;
    virtual std::vector<int64_t> getAllIds() const = 0;

    // ── 属性（变更经 tick 脏刷新合并为单次 respawn, 无闪烁串台）──
    virtual bool setPosition(int64_t id, float x, float y, float z, int dim) = 0; // dim<0 仅改坐标
    virtual bool setRotation(int64_t id, float yaw) = 0;
    virtual bool setNametag(int64_t id, std::string const& text) = 0;  // 空串清除
    // 换皮肤 = Remove + PlayerList Add + AddPlayer（协议限制, 有一次重入; 未注册返回 false）
    virtual bool setSkin(int64_t id, std::string const& skinId) = 0;
    virtual bool setViewDistance(int64_t id, double dist) = 0;
    virtual bool setScale(int64_t id, float scale) = 0; // 1.19.0; <0.0625 或 >10 拒绝
    virtual bool setEnabled(int64_t id, bool enabled) = 0;

    // ── 可见玩家白名单（仅指定玩家可见; 按 Player::getRealName 即 LSE realName 匹配）──
    // setVisiblePlayers 空列表 = 清除限制 = 全员可见
    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) = 0;
    virtual bool clearVisiblePlayers(int64_t id) = 0;
    virtual bool setVisiblePlayer(int64_t id, std::string const& playerName) = 0;

    // 诊断探针: 返回 NPC 运行态摘要字符串（找不到返回 "not_found"）
    virtual std::string getDebugInfo(int64_t id) const = 0;

    // ── 1.18.0 追加: 目录批量导入皮肤（一个子文件夹 = 一套皮肤）──
    // 子文件夹内: PNG 贴图（必需）+ .json 几何模型（可选, 缺省 = 标准玩家模型）
    // skinId = 子文件夹名; 返回导入数量（目录无效返回 -1）
    virtual int importSkins(std::string const& dirPath) = 0;
    // 皮肤全字段序列化导出（消费方持久化用; 未注册返回 false）
    virtual bool getSkinBlob(std::string const& skinId, std::string& out) const = 0;
    // blob 反序列化注册（与 getSkinBlob 配对; 格式非法返回 false）
    virtual bool registerSkinFromBlob(std::string const& blob) = 0;

    // ── 1.20.0 追加: 轻量朝向更新 + 逐客户端朝向 ──
    // 只发朝向增量包（MoveActorAbsolute, 不重建实体/不重发皮肤, 无闪烁）;
    // 与 setRotation（走 respawn）区别: 适合每 tick 跟踪式改朝向
    virtual bool setRotationLight(int64_t id, float yaw) = 0;
    // 覆盖指定玩家收到的该 NPC 朝向（出生包与增量包都按覆盖值下发）;
    // 未覆盖的玩家仍用 config 朝向; 玩家离线/未见过该 NPC 返回 false。下一 tick 生效。
    virtual bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw) = 0;
    // 清除单个玩家的朝向覆盖（回到 config 朝向）
    virtual bool clearPlayerRotation(int64_t id, std::string const& playerName) = 0;
    // 清除该 NPC 全部玩家的朝向覆盖（关闭逐客户端朝向时调用）
    virtual bool clearPlayerRotations(int64_t id) = 0;
};

// ─────────────────────────────────────────────
// 村民交易菜单（协议层; 1.21.0 追加）
//
// 界面完全由我们自己构造的 UpdateTradePacket 打开, 载荷与 BDS 26.40 原生交易逐字节一致
// （整包对拍: tests/check-trade-packet.bat; Offers 单独对拍: tests/check-trade-offers.bat）。
//   · 交易类型是自由字符串（原生值为翻译键, 如 entity.villager.butcher / entity.villager.priest）,
//     可填任意自定义值 —— 客户端按翻译键显示, 未知键原样显示
//   · 显示栏值用 1 基: 1=新手 2=学徒 3=老手 4=专家 5=大师（wire 上是 0..4, 本接口替你换算）
//   · 某条交易的 tier 高于 spec.tier → 客户端把它显示为未解锁（原版同款机制）;
//     想强制某条显示为锁, 直接给该条 `locked = true`
//   · 经验条不在包内: 由载体实体的 TradeTier/MaxTradeTier/TradeExperience 元数据驱动
//     （三项都是 Int, 与真实村民生成包实测一致; MaxTradeTier 恒为 4）
//
// 点击回传（"点了哪一条"）: 交易条目在新交易界面里就是配方, 客户端点它时会在
// ItemStackRequest(147) 里带一个 CraftRecipe 动作, 其配方 id 被回填成
// TradeClickEvent::recipeNetId / offerIndex（协议层路径下 netId 由本库分配, 映射精确）;
// 付费/产物槽上的动作另外各回传一次（TradeClickEvent 的 slot/container）。
//
// 载体实体: 交易界面需要 EntityUniqueId, 打开菜单时会自动在玩家身后 5 格生成一个
// **隐身、仅该玩家可见**的假村民, 关闭菜单即删除（对该玩家以外完全不可见, 不占实体系统）。
// 载体是异步送达客户端的, 所以 UpdateTrade 会在几 tick 后才发（否则界面绑不上实体）。
// ─────────────────────────────────────────────

// 显示栏值（tier）的取值范围, 1 基
inline constexpr int kTradeTierNovice = 1; // 新手
inline constexpr int kTradeTierMaster = 5; // 大师（上限）

// 一条交易的物品（付费或产出）
struct TradeMenuItem {
    std::string              type;  // minecraft:xxx（空 = 无此物品, 例如没有第二付费项）
    int                      count{1};
    int                      damage{0};
    std::string              name;  // 自定义名（空 = 无; 走 item NBT 的 tag.display.Name）
    std::vector<std::string> lore;  // 描述行（走 tag.display.Lore）
    bool                     isBlock{false};         // 方块类物品需带 Block 复合（实测原生行为）
    int                      blockVersion{18168865}; // 26.40 实测值
};

struct TradeMenuOffer {
    TradeMenuItem buyA;            // 付费物品 A（必填）
    TradeMenuItem buyB;            // 付费物品 B（type 空 = 无）
    TradeMenuItem sell;            // 产出物品
    // 该条解锁的显示栏值, 1 基 (1=新手 … 5=大师); 高于 spec.tier 即显示为未解锁。
    // 超出 [1,5] 会被夹紧。locked = true 时忽略本字段。
    int           tier{kTradeTierNovice};
    int           maxUses{16};
    int           traderExp{0};    // 本条给村民的经验
    // true = 无论 spec.tier 多高, 这条都显示为未解锁（按 spec.tier 自动再抬一级实现）
    bool          locked{false};
};

struct TradeMenuSpec {
    std::string                 tradeType{"entity.villager.butcher"};
    // 村民显示栏值, 1 基: 1=新手 2=学徒 3=老手 4=专家 5=大师（超出会被夹紧）
    int                         tier{kTradeTierNovice};
    int                         experience{0};     // 经验条当前经验
    bool                        displayOnly{true}; // true = 拦截成交（物品不消耗, 只回调点击）
    // 非 0 = 直接用该实体（真实村民的 uniqueId）作为交易对象, 且**不发自建 offers**:
    // 改为调用 BDS 自己的 Player::openTrading, 于是客户端看到的交易表就是服务端持有的那一份。
    // 这是解决"物品放不进交易槽"的关键 —— 我们发的 UpdateTrade 只改客户端, 服务端持有的仍是
    // 该实体自身的交易表, 两边不一致时客户端发来的放入请求会被 BDS 拒掉（实测症状）。
    // 0 = 用自建载体 + 自建 offers（旧路径, 仅供对比）。
    // true（默认）= 纯协议层: 只发我们自己构造的 UpdateTrade。界面完全由这个包打开, 服务端不放
    // 交易表 —— 玩家放料/成交的请求会走 ItemStackRequest(147), 由本域回调上报后交给调用方决定,
    // BDS 那边没有对应容器, 所以物品不会真的消耗（天然只读, 与参考实现 GMLIB ChestUI 同路）。
    // false = 给载体装真实交易表 + 走 BDS 自己的 openTrading —— 客户端与服务端持有同一份交易表,
    // 成交由 BDS 完成（物品真的消耗）。此时点击回调仍然可用。
    bool                        usePacketOffers{true};
    std::int64_t                carrierUniqueIdOverride{0};
    std::vector<TradeMenuOffer> offers;
    // 只对纯协议层路径（usePacketOffers = true）有意义: true（默认）= **由库自己接住"把付费
    // 物品放进交易槽"的动作并回成功应答**。为什么必须这样: 纯协议层路径下服务端没有这个交易
    // 容器, 放行让 BDS 处理会被它拒掉 → 客户端把物品弹回 → 交易界面永远进不到可成交状态,
    // 触屏玩家因此点不出任何"点击条目"的信号（实测: 几十次点击只有拖动付费那一次产生了包）。
    // 接住之后客户端会保留这次"放进去了"的预测, 界面得以走到成交那一步, 成交请求带回配方 id
    // → offerIndex 精确。客户端侧这层预测只是显示, 关闭菜单时库会刷新该玩家背包清掉它
    // （服务端物品从未真的移动）。false = 不接住（旧行为, 触屏走不通）。
    // **追加在尾部**: 保持既有字段偏移不变。
    bool                        acceptPaymentPlacement{true};
    // 载体实体类型(1.21.0 追加): "minecraft:villager_v2"(默认) 或
    // "minecraft:wandering_trader"(流浪商人 —— 界面外观与生物头图随类型变化)。
    // 两条路径都适用; 真实交易表路径下流浪商人同样持有 EconomyTradeableComponent, 装表方式相同。
    // **追加在尾部**: 保持既有字段偏移不变。
    std::string                 carrierIdentifier{"minecraft:villager_v2"};
};

struct TradeClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    // 被点击的交易下标。三个来源按优先级回填:
    //   ① 客户端点了配方列表里的某一条 → ItemStackRequest 里的 CraftRecipe 动作带该条的
    //      recipeNetId, 减掉本域分配的基准值即得下标（最可靠, 纯协议层路径下由我们分配）
    //   ② 配方列表条目挂在该实体的实体容器(LevelEntityContainer)上, 每条占 3 槽
    //      （付费A/付费B/产物）→ offerIndex = slot / 3
    //   ③ 命中"当前选中条目"的付费/产物槽（31/32/33/47/48/49）时无法定位条目 → -1,
    //      这时用 recipeNetId + slot + container 自行判定
    int         offerIndex{-1};
    int         slot{-1};         // 命中的**交易侧**槽位（命中付费/产物槽时优先报交易侧, 其次背包侧）
    bool        accepted{false};  // false = 被拦截（displayOnly）
    // 定位用: 命中的容器枚举（ContainerEnumName: 7=LevelEntityContainer, 31/32=付费A/B,
    // 33=产物, 47/48/49=双付费变体）与动态 id
    int         container{0};
    int         containerId{-1};
    // 触发本次回调的配方网络 id（CraftRecipe 类动作携带）; 非 CraftRecipe 动作为 -1。
    // 纯协议层路径下 = netIdBase + offerIndex（默认 netIdBase = 3676）。
    // **追加在尾部**: 保持既有字段偏移不变（已编译的消费方不会错位）。
    int         recipeNetId{-1};
};

// 槽位引用（对齐参考实现 GMLIB ChestUI 的 ChangingSlot）
struct TradeSlotRef {
    int slot{-1};      // 容器内槽位; -1 = 无（例如关闭事件、或该动作没有目标槽）
    // 所属容器（ContainerEnumName）:
    //   7=实体容器（配方列表条目 / 打开容器）, 31/32=当前条目的付费A/B, 33=当前条目的产物,
    //   47/48/49=双付费变体, 12=玩家背包, 28/29=快捷栏 ...  0 = 无
    int container{0};
};

// 逐动作回调（对齐参考实现 ChestUI 的 ChestUICallback 契约）:
//   src      = 玩家取物的槽位（取/消耗动作用它）
//   dst      = 玩家放物的槽位（放/丢弃动作用它）
//   amount   = 数量
// 关闭哨兵: 界面被客户端关闭时回调会以 { slot = -1 } + amount = -1 调用一次,
// 表示"这次交易界面结束了"（与参考实现同一约定）, 随后菜单记录被清掉。
using TradeActionCallback = std::function<void(
    std::string const&  playerName,
    int64_t             menuId,
    TradeSlotRef const& src,
    TradeSlotRef const& dst,
    int                 amount
)>;

// 诊断用: 菜单打开期间收到的**每一个**动作（不论是否命中交易容器）。
// 用途: 排查"客户端到底发了什么" —— 例如某个条目被点击时只发了一个不带槽位的 craft 动作,
// 或压根什么包都没发, 靠 TradeActionCallback 是分辨不出来的。
struct TradeRawAction {
    // 包内动作变体下标 = BDS 的 ItemStackRequestAction 变体序（实测 26.40）:
    //   0=Take 1=Place 2=Swap 3=Drop 4=Destroy 5=Consume 6=Create 7=LabTableCombine
    //   8=BeaconPayment 9=MineBlock 10=CraftRecipe 11=CraftRecipeAuto 12=CraftCreative
    //   13=CraftRecipeOptional 14=CraftRepairAndDisenchant 15=CraftLoom 16=CraftNonImplemented
    //   17=CraftResults
    int          actionIndex{-1};
    TradeSlotRef src{};
    TradeSlotRef dst{};
    int          amount{1};
    bool         hasSrc{false};
    bool         hasDst{false};
    bool         tradeRelated{false}; // src/dst 是否命中交易容器
    // CraftRecipe 类动作携带的配方网络 id（"点了哪一条交易"的信号）; 其它动作 -1。
    int          recipeNetId{-1};
    // src/dst 侧的**动态容器 id**（FullContainerName.mDynamicId）: 交易界面是 1..100,
    // 虚拟容器是 101..199。用来定位"客户端到底把它当成哪个容器"（追加在尾部, ABI 安全）。
    int          srcContainerId{-1};
    int          dstContainerId{-1};
    // 这个动作来自哪条包通道: 147 = 独立的 ItemStackRequestPacket;
    // 144 = 搭在 PlayerAuthInputPacket(AuthInput) 内嵌请求里（菜单界面的交互常走这条）。
    int          sourcePacketId{147};
};

using TradeRawActionCallback =
    std::function<void(std::string const& playerName, int64_t menuId, TradeRawAction const& action)>;

class ITradeMenu {
public:
    virtual ~ITradeMenu() = default;

    // 打开交易菜单（同一玩家重复调用会先关掉旧菜单）; 返回 menuId, <0 = 失败（玩家不在线等）
    virtual int64_t open(std::string const& playerName, TradeMenuSpec const& spec) = 0;
    virtual bool    update(int64_t menuId, TradeMenuSpec const& spec) = 0;
    virtual bool    close(int64_t menuId) = 0;
    virtual void    closeAll() = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t menuId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const = 0;

    // 点击回传（多播, 多个插件可同时注册）; 返回 token（0 = 失败）
    virtual uint64_t addClickListener(std::function<void(TradeClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token) = 0;

    // 逐动作回传（含关闭哨兵; 语义见 TradeActionCallback）
    virtual uint64_t addActionListener(TradeActionCallback listener) = 0;
    virtual bool     removeActionListener(uint64_t token) = 0;

    // 诊断用原始动作回传（见 TradeRawAction）
    virtual uint64_t addRawActionListener(TradeRawActionCallback listener) = 0;
    virtual bool     removeRawActionListener(uint64_t token) = 0;
};

// ─────────────────────────────────────────────
// NPC 对话框（minecraft:npc 的对话界面; 1.21.0）
//
// 协议: NpcDialoguePacket（下发: 场景名 + 正文 + NPC 名 + 按钮 JSON）+ NpcRequestPacket（回传:
// 点的是第几个按钮 / 玩家关闭了对话）。字段与枚举取值取自 sculk 与 BDS 26.40 头文件。
//   · 按钮表完全由服务端逐玩家生成 —— "按权限/tag/计分板/任务状态决定按钮是否出现"
//     就是在调用本接口前自行算好 buttons 即可, 不需要任何客户端配合
//   · 多层级对话 = 点击回传里带回 sceneName + buttonIndex, 调用方据此发送下一层对话
//
// 载体实体: 与交易菜单同理, 对话框需要 NPC 实体承载名字/头像/按钮。载体是**纯协议实体**
// （合成 AddActor, 不进 BDS 实体系统）, 每玩家一个: 位置在世界下方 y=-66 —— 客户端看不到实体,
// 但界面里的头像照常渲染（改用隐形标志位反而会让头像一起消失）; 只发给该玩家;
// 点按钮 / 关闭 / 玩家离线时发 RemoveActor 删掉该客户端的实体。
//
// 关于按钮 JSON: 原版按钮结构无公开文档, 故 buttons 之外另给 rawActionJson —— 非空时
// 原样作为 mActionJSON 下发, 便于在游戏内实测字段格式（探针 MeowTradeTest 即用它迭代）。
// ─────────────────────────────────────────────

struct NpcDialogButton {
    std::string              label;    // 按钮文本
    std::vector<std::string> commands; // 展示在按钮上的命令（合成 NPC 不由 BDS 代跑, 命令回传给调用方）
    std::string              actionId; // 服务器侧标识, 点击回传里原样带回（对接对话树/任务用）
    // 客户端动作类型（与参考实现 ActionType 一致）:
    //   0 = 普通按钮 —— 点击后服务端收到 ExecuteAction(回传 buttonIndex)
    //   1 = 关闭      —— 点击后服务端收到 ExecuteClosingCommands(closed = true)
    //   2 = 打开
    int                      mode{0};
};

struct NpcDialogSpec {
    // 载体实体类型。必须是 NPC 家族（默认 minecraft:npc）—— 客户端的 NPC 对话界面与该
    // 实体类型绑定, 换成村民等其他原版实体是不行的（实测要求）。可改仅用于排查对比。
    std::string                  carrierIdentifier{"minecraft:npc"};
    std::string                  npcName{"NPC"};    // 对话框中显示的名字
    std::string                  sceneName{"main"}; // 场景名（多层级对话靠它路由）
    std::string                  dialogue;          // 正文
    // 诊断用: 非 0 时用它作为对话框的目标 NPC uniqueId（跳过自建载体）。
    // 用途: 验证"客户端是否会为真实 NPC 弹对话界面" —— 若真实 NPC 的 id 能弹、自建载体不能,
    // 说明卡点在实体识别, 而不在包内容。
    std::int64_t                 npcUniqueIdOverride{0};
    std::vector<NpcDialogButton> buttons;
    std::string                  rawActionJson;     // 非空 = 原样作为 mActionJSON（实测字段格式用）
};

struct NpcDialogClickEvent {
    std::string playerName;
    int64_t     dialogId{-1};
    std::string sceneName;
    int         buttonIndex{-1}; // 点了第几个按钮（-1 = 非按钮事件）
    std::string actionId;        // 对应 NpcDialogButton::actionId（按 buttonIndex 回填）
    bool        closed{false};   // true = 玩家关闭了对话
    // 被点击按钮上挂的命令（按 buttonIndex 回填）。合成 NPC 服务端侧没有 BDS 实体代跑命令,
    // 需要由调用方自行执行（例如 player.runCommand）。
    std::vector<std::string> commands;
};

// ─────────────────────────────────────────────
// 虚拟容器（列表）界面（协议层; 1.21.0 追加）
//
// 方案与参考实现 GMLIB 的 ChestUI 同路（逐条复刻）:
//   1. 客户端侧摆一个**箱子方块**（UpdateBlockPacket, 只发给该玩家, 服务端世界里没有它）
//   2. 客户端侧摆该方块的**方块实体 NBT**（BlockActorDataPacket）: id=Chest / CustomName=标题 /
//      x,y,z / Items=[...] —— 条目物品就走这份 NBT（不是逐格 InventorySlot）
//   3. 等 10 tick, 发 ContainerOpen（ContainerType=Container(0), 位置=那个方块, 目标实体=-1）
//   4. 玩家点击 → 客户端发 ItemStackRequest(147) → 槽位命中本容器 → 回调（附槽位号 = 条目下标）
//   5. 关闭 → ContainerClosePacket → 恢复真方块 → 回调 closed
//
// 关键性质: **服务端根本没有这个容器** —— 物品只是"摆在那里", 玩家拿走/移动都不会真的改变任何
// 东西（天然只读, 不需要 displayOnly 开关）。适合当任务列表、成就列表、商店预览这类
// "只展示 + 点击回调"的界面。
//
// 大小容器都在这里: rows=3 → 单箱子 27 格; rows=6 → **大箱子 54 格**
// （大箱子 = 相邻两个箱子方块 + 方块实体的 pairx/pairz/pairlead 配对键, 前 27 格进 lead 那半,
//  后 27 格进副半, 槽位号在各自 NBT 里是 0..26 —— 与 GMLIB 的 updateBlockActor 一致）。
//
// 载体位置: 玩家脚上方 5 格（GMLIB 同款; 超出世界高度则下移 4 格）, 并优先挑选空气位, 避免
// 覆盖真实方块/方块实体。关闭时会用真方块的网络 id 把那一格改回来。
//
// 容器 id 用 101..199 显示区间, 与 BDS 真实容器（含交易）的 1..100 不冲突。
// ─────────────────────────────────────────────

struct ContainerMenuItem {
    std::string              type;  // minecraft:xxx（空 = 该槽留空）
    int                      count{1};
    int                      damage{0};
    std::string              name;  // 自定义名（空 = 无）
    std::vector<std::string> lore;
};

struct ContainerMenuSpec {
    int                           rows{3}; // 容器行数（3 = 小箱子 27 格, 6 = 大箱子 54 格）
    std::vector<ContainerMenuItem> items;  // 按槽位顺序; 下标即槽位号（大箱子时 0..53）
    // false（默认）= GMLIB 方案: 客户端侧箱子方块 + 方块实体（实测客户端认这条）。
    // true = 旧路径: 只发一只合成的箱子矿车实体 + ContainerOpen(MinecartChest) —— 实测客户端
    // 不弹界面, 保留仅为对比排查。
    bool                          useMinecart{false};
    // 箱子界面标题（方块实体的 CustomName）。**追加在尾部**: 保持既有字段偏移不变,
    // 已编译的消费方（未重编）不会因此错位读到别的字段。
    std::string                   title{"虚拟容器"};
    // 摆好客户端侧方块+方块实体之后, 等多少 tick 再发 ContainerOpen。
    // 为什么必须等: 客户端要先把这个方块与它的方块实体应用上去, ContainerOpen 才绑得住这个位置
    // （背靠背发界面打不开）。参考实现 GMLIB 的 ChestUI 等的是 10 tick, 之后还要再等 4 tick 补格
    // （共 ~700ms）; 本库的条目本来就放在方块实体 NBT 里, 不需要那 4 tick（默认 10 tick ≈ 500ms）。
    // **实测(26.40, 本机客户端): 7 能正常开界面, 6 不行** —— 默认 7(~350ms; GMLIB 等效 14 tick
    // ≈700ms)。下限与客户端/机器有关, 换设备或负载高时可能要回调大, 所以留成可调。
    // **追加在尾部**: 保持既有字段偏移不变。
    int                           openDelayTicks{7};
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};        // 被点击的槽位（= items 的下标）
    bool        closed{false};   // true = 界面被关闭（此时 slot = -1）
};

class IContainerMenu {
public:
    virtual ~IContainerMenu() = default;

    virtual int64_t open(std::string const& playerName, ContainerMenuSpec const& spec) = 0;
    virtual bool    close(int64_t menuId) = 0;
    virtual void    closeAll() = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t menuId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const = 0;

    // 点击/关闭回传（多播）; 返回 token（0 = 失败）
    virtual uint64_t addClickListener(std::function<void(ContainerClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token) = 0;

    // ── 1.21.0 追加（冻结契约: 只在尾部追加）──
    // 就地换内容（翻页/进子菜单/返回）: 复用同一个载体方块, **不拆界面、不重发方块、不等待** ——
    // 只把新的方块实体 NBT（Items/标题）重发一次, 再把 ContainerOpen 重发一次。
    // 对比 open(): open 会先关旧菜单（发 ContainerClose + 恢复真方块）再重新摆方块 + 等
    // openDelayTicks, 所以"整体重开"每次都要付一次打开延迟; 翻页属于同一界面的内容变更, 用这个。
    // 返回 false = 该菜单已不在（调用方应改用 open）。
    virtual bool update(int64_t menuId, ContainerMenuSpec const& spec) = 0;

    // ── 按槽动态刷新（1.21.0 追加）──
    // 只改一个格子: 发一条 InventorySlot（与真实箱子同步内容用的同一种包）, 客户端就地换掉那一格
    // —— 不重发方块实体、不重发 ContainerOpen、不重开界面, 所以**没有延迟也没有闪烁**。
    // 适合"任务完成打勾/数量变化/价格变化"这类单格改动; 换整页用 update()。
    // item.type 为空 = 把该槽清空。返回 false = 该菜单已不在或槽位越界。
    virtual bool setItem(int64_t menuId, int slot, ContainerMenuItem const& item) = 0;
};

class INpcDialogue {
public:
    virtual ~INpcDialogue() = default;

    // 打开对话（同一玩家重复调用会先关掉上一层的界面）; 返回 dialogId, <0 = 失败
    virtual int64_t open(std::string const& playerName, NpcDialogSpec const& spec) = 0;
    virtual bool    close(int64_t dialogId) = 0;
    virtual void    closeAll() = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t dialogId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const = 0;

    // 点击/关闭回传（多播）; 返回 token（0 = 失败）
    virtual uint64_t addClickListener(std::function<void(NpcDialogClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token) = 0;

    // ── 1.21.0 追加（冻结契约: 只在尾部追加）──
    // 就地换内容（翻页 / 进子菜单 / 返回 / 命令后重推）: 复用同一个载体 NPC, 只重发一次
    // NpcDialoguePacket(Open) —— 不删载体、不重建、不关界面, 所以无闪烁、无延迟。
    // 在点击回调里调用它即表示"这次点击已由调用方接管": 库随后不会再拆掉这个对话
    // （否则客户端收起界面后库会按玩家把对话与载体删掉, 就地换的内容会一起没）。
    // 返回 false = 该对话已不在（调用方应改用 open）。
    //
    // ⚠ 实测限制（26.40 客户端）: 原版 NPC 对话界面在**点击任意按钮时客户端就会自行收起**,
    // 而本函数只重发 NpcDialoguePacket(Open) —— 客户端不会因此重新弹出界面。
    // 所以"点按钮后就地换页"对 NPC 对话**不可行**; 要让客户端重新显示, 必须走 open()
    // （open 会删旧载体 + 建新载体 + 发 Open, 客户端才会再弹一次界面）。
    // 本函数适合的场景: 玩家界面仍开着时的内容微调（例如对话里某项状态变化, 不经点击触发）。
    virtual bool update(int64_t dialogId, NpcDialogSpec const& spec) = 0;
};

// ─────────────────────────────────────────────
// 感知域(1.21.0): 客户端设备判断
//
// 数据源: PlayerAuthInputPacket(AuthInput) 的 InputMode 字段, 每个玩家每 tick 持续上报 ——
// 库在协议层挂钩捕获, 消费方随时查询"这个玩家用的是什么设备"。
// 取值来自 26.40 的 InputMode 枚举: 0=Undefined 1=Mouse(键鼠) 2=Touch 3=GamePad 4=MotionController
//
// 典型用途: 按设备路由 UI —— 触屏玩家给容器列表(实证触屏走不通纯协议层交易),
// 键鼠/手柄给交易界面; 输入方式由客户端决定, 服务器只读。
// ─────────────────────────────────────────────
enum class ClientInputDevice {
    Unknown = 0,       // 尚未收到该玩家的 AuthInput, 或客户端报 Undefined
    KeyboardMouse,     // 键盘 + 鼠标
    Touch,             // 触屏
    Gamepad,           // 手柄
    MotionController,  // 体感(已弃用, 客户端仍可能报)
};

class IPlayerSensing {
public:
    virtual ~IPlayerSensing() = default;

    // 查询玩家当前输入设备（玩家不在线/尚未上报 = Unknown; 换设备后下一 AuthInput 即更新）
    [[nodiscard]] virtual ClientInputDevice inputDeviceOf(std::string const& playerName) const = 0;
    // 便捷判断: 是否触屏
    [[nodiscard]] virtual bool isTouch(std::string const& playerName) const = 0;
};

// ─────────────────────────────────────────────
// 库入口单例
// ─────────────────────────────────────────────
class IHologramLib {
public:
    HOLOGLIB_API static IHologramLib& getInstance();

    virtual ~IHologramLib() = default;

    virtual IShapeDrawer&  shapes()     = 0;
    virtual IHologramText& holograms()  = 0;
    virtual IItemDetail&   itemDetails() = 0;

    // LSE 兼容层是否可用（LegacyRemoteCall 运行时检测成功）
    virtual bool isLseAvailable() = 0;

    // 库版本（BCD: 0x011B00 = 1.21.0, 与 HOLOGLIB_API_VERSION 同值）
    virtual uint32_t version() = 0;

    // ── 1.6.0 追加（冻结契约: 只在尾部追加）──
    virtual IItemDisplay& itemDisplays() = 0;

    // 查找距 (x,y,z) 最近的可悬浮显示（dim 匹配; maxDist<=0 视为无限制）
    // 返回 id, 无匹配返回 -1
    virtual int64_t findNearestItemDisplay(float x, float y, float z, int dim, double maxDist) = 0;

    // ── 1.10.0 追加（冻结契约: 只在尾部追加）──
    virtual ICustomEntity& customEntities() = 0;

    // ── 1.12.0 追加（冻结契约: 只在尾部追加）──
    // ghost 交互监听（C++ 推送; 每次点击回调一次, 建议主线程处理）
    virtual void setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener) = 0;
    virtual void clearGhostInteractListener() = 0;
    // LSE 轮询版: 取走并清空待处理交互队列（每条为可解析字符串, 格式见 API.md）
    virtual std::vector<std::string> pollGhostInteractions() = 0;

    // ── 1.14.0 追加（冻结契约: 只在尾部追加）──
    virtual IParticleShape& particleShapes() = 0;

    // ── 1.16.0 追加（冻结契约: 只在尾部追加）──
    virtual IPlayerNpc& playerNpcs() = 0;

    // ── 1.19.1 追加（冻结契约: 只在尾部追加）──
    // ghost 交互多播监听: 多个插件可同时注册, 互不覆盖（旧 set/clear 单槽接口保留兼容）。
    // 返回 token（0=失败）; removeGhostInteractListener(token) 移除; 事件在主线程网络处理路径上回调。
    virtual uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener) = 0;
    virtual bool removeGhostInteractListener(uint64_t token) = 0;

    // ── 1.21.0 追加（冻结契约: 只在尾部追加）──
    virtual ITradeMenu& tradeMenus() = 0;

    // ── 1.21.0 追加（冻结契约: 只在尾部追加）──
    virtual INpcDialogue& npcDialogs() = 0;

    // 虚拟容器（列表）界面（1.21.0 追加, 尾部追加保持 ABI 兼容）
    virtual IContainerMenu& containerMenus() = 0;

    // ── 感知域(1.21.0 追加): 客户端设备判断 ──
    virtual IPlayerSensing& playerSensing() = 0;
};

} // namespace hologramlib
