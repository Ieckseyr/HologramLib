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
#include <string>
#include <vector>

// 库 API 版本（与 IHologramLib::version() 同值, BCD: 0x010701 = 1.7.1）
// 消费方可用于编译期静态断言最低版本要求
#define HOLOGLIB_API_VERSION 0x011000

#ifdef HOLOGLIB_EXPORTS
#define HOLOGLIB_API __declspec(dllexport)
#else
#define HOLOGLIB_API __declspec(dllimport)
#endif

namespace hologramlib {

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
};

// ─────────────────────────────────────────────
// 自定义实体协议层生成（任意类型实体; 1.10.0 追加）
// AddActorPacket 直发客户端生成纯视觉实体, 不占服务端实体系统;
// 适合 NPC 壳 / 装饰生物 / 盔甲架布景等（不可交互, 无碰撞）
// ─────────────────────────────────────────────
struct CustomEntityConfig {
    std::string identifier{"minecraft:armor_stand"}; // 实体类型标识符（短名自动补 minecraft:）
    float       x{0}, y{64}, z{0};                   // 世界坐标
    int         dimension{0};                        // 维度 ID
    float       yaw{0}, pitch{0};                    // 朝向（度; 头/身旋转同值）
    std::string nametag{};                           // 头顶名字（支持 § 颜色码; 空 = 无）
    bool        nametagAlwaysShow{false};            // 名字常显（默认仅准星对准时显示）
    float       scale{1.0f};                         // 实体缩放（客户端有效域 0.0625~10, 自动钳制）
    int         variant{0};                          // 变种（皮肤/亚种, 依实体定义）
    int         markVariant{0};                      // 二级变种
    int         colorIndex{0};                       // 颜色索引（羊/项圈等染色实体）
    std::int64_t flags{0};                           // 原始 flags 位掩码（0x01=着火 0x20=隐身 等）
    bool        invisible{false};                    // 隐身便捷开关（与 flags 独立, 发包时合成 0x20）
    double      viewDistance{64.0};                  // 可见距离（方块; <=0 无限制）
    bool        enabled{true};
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

    virtual int64_t findNearest(float x, float y, float z, int dim, double maxDist) const = 0; // 无匹配 -1
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

    // 库版本（BCD: 0x010701 = 1.7.1）
    virtual uint32_t version() = 0;

    // ── 1.6.0 追加（冻结契约: 只在尾部追加）──
    virtual IItemDisplay& itemDisplays() = 0;

    // 查找距 (x,y,z) 最近的可悬浮显示（dim 匹配; maxDist<=0 视为无限制）
    // 返回 id, 无匹配返回 -1
    virtual int64_t findNearestItemDisplay(float x, float y, float z, int dim, double maxDist) = 0;

    // ── 1.10.0 追加（冻结契约: 只在尾部追加）──
    virtual ICustomEntity& customEntities() = 0;
};

} // namespace hologramlib
