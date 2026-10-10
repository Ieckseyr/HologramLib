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
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// 库 API 版本（与 IHologramLib::version() 同值; 编码规律与完整对照见 VERSION-HISTORY.md）
//   中间字节 = 次版本号, 按十六进制递增: 1.15.0 -> 0x011500, 1.19.0 -> 0x011900,
//   1.20.0 -> 0x011A00, 1.21.0 -> 0x011B00, 1.22.0 -> 0x011C00, 1.23.0 -> 0x011D00（补丁位通常为 00）
// 消费方可用于编译期静态断言最低版本要求。
// 注意: 只有**正式发布新版本**才推高本宏; 在同一条尚未发布的线上继续加能力域时不改变它。
// 本值 0x011C00 随 26.40.4 发布: 交易菜单（1.21.0 引入）改为**纯展示**, 撤回了它在 26.40.3 里
// 短暂存在的点击回调（TradeClickEvent / TradeActionCallback / TradeRawAction 与对应的
// ITradeMenu 监听方法全部移除）—— 这是收缩而非新增, 所以抬到新的次版本, 消费方可以用
// >= 0x011C00 门住『交易菜单没有点击回调、且带 addOffer / setTier』这一形态。
// 本值 0x012000 随 26.40.8 发布（1.26.0）: 一次**追加 + 收缩 + 补全**（收缩明记, 见 VERSION-HISTORY.md）——
//   追加: IPlayerNpc 尾部补 setPositionLight / injectSkin / injectSkinAll / playAnimation / playAnimationTo /
//   setEntitySpawnCallback（轻量位置更新 + 玩家皮肤注入 + 按名动画）; view* LSE 导出补齐逐字段（6→16 函数）;
//   收缩: 移除 setLineGradient / setLineRainbow / setLineScroll / setVerticalAnimation / setLineSpacing /
//   setLineColor / setLineScale 与 tick（前六项在"整块单形状"模型下不可实现或空转, tick 的库内自驱按"基础轮子"原则移除;
//   C++ 侧这 8 个方法保留**废弃空槽**（非纯虚空实现 + [[deprecated]]）, 虚表槽位不变 —— 旧二进制不错位, 真正清槽留到 2.0.0）;
//   补全: 文本形状补齐 背景框颜色 / 穿墙开关(depthTest) / 三轴旋转(useRotation) / 整块缩放,
//   且 setRotation 对文本形状真正生效（此前缺 useRotation 标志）; 视图覆盖清理旧世界残骸（过期注释 + 就地改写死代码, 行为不变: 严格只拦不发）。
//   消费方用 >= 0x012000 门住这些新方法。
// 本值 0x012100 随 26.40.9 发布（1.27.0）: 视图覆盖 `EntityView::skinId`（非玩家实体同-id 换"我们的模型"）+
//   真实实体交互多播（addActorInteractListener / removeActorInteractListener, 类尾追加）;
//   配套 ABI 布局戳（消费方编译期自检, 库侧运行期拦截布局不一致的消费方）。消费方用 >= 0x012100 门住这些新方法。
#define HOLOGLIB_API_VERSION 0x012100

// ── ABI 布局戳（消费方自检用; 与上面的"功能版本"是两件事, 别混）──
// 语义: 只在**破坏虚表槽位**的改动时 +1 —— 在接口**中段**插入/删除/重排虚函数、改已发布方法的签名;
//      **尾部追加不动它**（追加不改变已有槽位, 用旧头文件编译的消费方照样兼容 —— 这正是"冻结契约"的价值）。
// 为什么需要: 消费方把虚函数的**声明顺序**在编译期固化成槽位号, 运行时才去 DLL 的虚表里取。DLL 布局与
//   消费方编译时的头文件不是同一版时, 调用会落到**别的方法**上。实测两次（2026-10-09 / 10-10 启动崩溃）:
//   addActorInteractListener / removeActorInteractListener 的第一版被插在 ghost 段之后（中段）→
//   playerNpcs() 及其后整体前移 2 格 → 旧二进制的 playerNpcs() 取到"返回 bool"的方法 → 拿到 0 当引用用 →
//   解引用 0xC0000005 崩服; 而栈全在消费方自己代码里, 看起来像"两个插件互相冲突"。
//   功能版本号挡不住这种情况: 错位的那版 DLL 版本号反而更高（>= kRequired 照样通过）。
// 实现方式: DLL 导出纯 C 符号（见下面 Symbol）—— 消费方用 GetProcAddress **动态**取, 老 DLL 没有它时
//   拿到 null 而不是链接/加载期失败, 也不动虚表（加虚函数槽本身在"批次不一致"时也会崩）。
// 用法（消费方 enable() 里做一次; 取不到或不一致 = 库不是同一批构建, 应**拒绝启用**并提示用户）:
//   auto h = ::GetModuleHandleA("HologramLib.dll");
//   auto fn = h ? ::GetProcAddress(h, HOLOGLIB_ABI_STAMP_SYMBOL) : nullptr;
//   if (!fn || reinterpret_cast<std::uint32_t(__cdecl*)()>(fn)() != HOLOGLIB_ABI_STAMP) { 拒绝启用 }
// 提示语要点: 把**同一批构建**的 HologramLib.dll 部署到 plugins/HologramLib/ 并重启;
//   若插件的槽位已错位, 必须重新**编译**（槽位号写死在机器码里）, 只重新链接不算。
//   例: MSkinventory::HoloLoad.cpp / MeowHolographicRenderer::ModEntry.cpp 各有一份实现。
inline constexpr std::uint32_t HOLOGLIB_ABI_STAMP = 1;
// 布局戳的导出符号名（纯 C; 见上）
inline constexpr char const* HOLOGLIB_ABI_STAMP_SYMBOL = "hologramlib_abiStamp";

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
    // 三轴欧拉角 [Pitch, Yaw, Roll]（度, 官方脚本 API 口径; 本库直通线格式, 不做换算）。
    // 文本形状: 自动启用 useRotation（不再面向相机）并打开双面渲染; clearRotation 恢复面向相机。
    virtual bool setRotation(int64_t id, float pitch, float yaw, float roll) = 0;
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

    // ── 1.26.0 追加（冻结契约: 只在尾部追加）──
    // 以下三项仅对文本形状（createText）有效, 其它类型返回 false。
    // 背景框（文本底板）颜色（RGBA 0~1）; clearBackgroundColor = 回客户端默认色。
    virtual bool setBackgroundColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool clearBackgroundColor(int64_t id)                                   = 0;
    // 穿墙可见性（depthTest）: true = 被方块/实体遮挡; false = 始终渲染（穿墙可见, 默认）。
    virtual bool setDepthTest(int64_t id, bool enabled)                             = 0;
};

// ─────────────────────────────────────────────
// 悬浮字 / 全息（多行、整块样式、跟随、动态变量; 1.26.0 收缩后口径）
// 实现"整块文本单一背景框"渲染（非逐字符分框）:
//   整块 = 一个文本形状 —— 颜色/缩放/背景框/穿墙/旋转都是**整块**属性;
//   行级样式差异由调用方在文本内嵌 § 颜色代码承担（不再提供行级颜色/缩放 API）。
// ABI: 1.26.0 收缩掉的 8 个方法保留为**废弃空槽**（非纯虚空实现, 虚表槽位与 1.25.0 一致）
//   —— 旧二进制按旧槽位调用命中空实现、不会错位; 细节见 VERSION-HISTORY.md「第二次收缩」。
// ─────────────────────────────────────────────
class IHologramText {
public:
    virtual ~IHologramText() = default;

    virtual int64_t create(float x, float y, float z)      = 0;
    virtual bool    destroy(int64_t id)                    = 0;
    virtual void    destroyAll()                           = 0;

    // 行管理（行索引 0 起; 行只负责文本）
    virtual bool addLine(int64_t id, std::string const& text)              = 0;
    virtual bool setLineText(int64_t id, int lineIndex, std::string const& text) = 0;
    // ── 以下 8 个是**废弃空槽**（1.26.0 收缩; 功能整体移除, 调用一律无动作/返回 false）──
    // 保留**虚表槽位**（非纯虚 + 内联空实现）只为不动既有 ABI: 1.26.0 之前编译且未重编译的
    // 消费方二进制照旧按旧槽位调用、命中空实现, 不会错位打到别的函数; 重编译得到 [[deprecated]] 警告。
    // 真正清槽留到下一个大版本（2.0.0）。
    [[deprecated("1.26.0 移除: 行级缩放从未生效; 整块缩放请用 setScale")]]
    virtual bool setLineScale(int64_t /*id*/, int /*lineIndex*/, float /*scale*/) { return false; }
    virtual bool removeLine(int64_t id, int lineIndex)                     = 0;
    virtual bool clearLines(int64_t id)                                    = 0;
    virtual int  getLineCount(int64_t id)                                  = 0;

    // 整块颜色（RGBA 0~1）
    virtual bool setColor(int64_t id, float r, float g, float b, float a)  = 0;
    [[deprecated("1.26.0 移除: 行级颜色无协议支撑; 行级颜色请用文本内嵌 § 颜色代码")]]
    virtual bool setLineColor(int64_t /*id*/, int /*lineIndex*/, float /*r*/, float /*g*/, float /*b*/, float /*a*/) {
        return false;
    }
    [[deprecated("1.26.0 移除: 行级渐变无协议支撑")]]
    virtual bool setLineGradient(
        int64_t /*id*/,
        int     /*lineIndex*/,
        float   /*r1*/,
        float   /*g1*/,
        float   /*b1*/,
        float   /*r2*/,
        float   /*g2*/,
        float   /*b2*/
    ) {
        return false;
    }
    [[deprecated("1.26.0 移除: 彩虹模式会把文本涂白")]]
    virtual bool setLineRainbow(int64_t /*id*/, int /*lineIndex*/, float /*speed*/) { return false; }
    [[deprecated("1.26.0 移除: 依赖已删除的库内自驱动画")]]
    virtual bool setLineScroll(int64_t /*id*/, int /*lineIndex*/, int /*direction*/, float /*speed*/) { return false; }
    [[deprecated("1.26.0 移除: 依赖已删除的库内自驱动画")]]
    virtual bool setVerticalAnimation(int64_t /*id*/, int /*type*/, float /*speed*/, float /*range*/) {
        return false;
    }
    [[deprecated("1.26.0 移除: 协议 2168 无行间距字段")]]
    virtual bool setLineSpacing(int64_t /*id*/, float /*spacing*/) { return false; }

    // 位置与跟随（跟随位置在 setFollowPlayer / draw / refresh 调用时就地解析 —— 无库内自驱 tick）
    virtual bool setLocation(int64_t id, float x, float y, float z)         = 0;
    virtual bool setFollowPlayer(int64_t id, std::string const& playerName, float offsetY) = 0;
    virtual bool clearFollowPlayer(int64_t id)                              = 0;

    // 显示
    virtual bool draw(int64_t id)                                           = 0;
    virtual bool drawToDimension(int64_t id, int dimId)                     = 0;
    virtual bool drawToPlayer(int64_t id, std::string const& playerName)    = 0;
    virtual bool remove(int64_t id)                                         = 0;
    virtual bool refresh(int64_t id)                                        = 0; // 重新解析变量/跟随坐标并原地重发

    [[deprecated("1.26.0 移除: 库内自驱已删; 跟随/变量在 draw / refresh 时就地解析")]]
    virtual void tick(float /*deltaTime*/) {} // 废弃空槽（同上, 只为保持虚表槽位）

    // ── 1.12.0 追加（冻结契约: 只在尾部追加）──
    // 迁移维度: 已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁）
    virtual bool setDimension(int64_t id, int dimId)                        = 0;

    // ── 1.26.0 追加 ──
    // 整块缩放（替代原行级 setLineScale —— 行级缩放从未生效）
    virtual bool setScale(int64_t id, float scale)                          = 0;
    // 背景框（文本底板）颜色（RGBA 0~1; 不设 = 客户端默认色）
    virtual bool setBackgroundColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool clearBackgroundColor(int64_t id)                           = 0;
    // 穿墙可见性（depthTest）: true = 被方块/实体遮挡; false = 始终渲染（穿墙可见, 默认）
    virtual bool setDepthTest(int64_t id, bool enabled)                     = 0;
    // 整块三轴旋转（度, [Pitch, Yaw, Roll]; 官方脚本 API 口径）:
    // 设置后整块不再面向相机（useRotation）; clearRotation 恢复面向相机。
    virtual bool setRotation(int64_t id, float pitch, float yaw, float roll) = 0;
    virtual bool clearRotation(int64_t id)                                   = 0;

    // ── 1.26.0 追加: 动态行（**重构自 Phantom**, LGPL-3.0 —— 见 src/FloatingTextManager.h 文件头标注）──
    // setLinePool: 该行的候选内容池 —— 每 intervalMs 毫秒轮播一项（时间取模 = 无状态, 多个浮字
    //   同池同相）; 池 <=1 项或 intervalMs<=0 = 不轮播; content 为空 = 清除池（回到 setLineText 文本）。
    //   轮播由库内 0.5s 节流刷新承担（**内容变了才重发**）—— 区别于本版移除的自驱"偏移"
    //   （那时每拍都在动、但驱出来的东西没有消费方）; 这里每次刷新都是客户端可见的真变化。
    virtual bool setLinePool(int64_t id, int lineIndex, std::vector<std::string> const& content, int intervalMs) = 0;
    // setLineParseVariables: 该行是否解析变量（内置 {player}/{online}/{time}/{tps}/{dimension}/{x}/{y}/{z}
    //   + 经 MeowPAPI 的外部占位符）。默认 true; 关掉 = { } 按字面量原样显示。
    virtual bool setLineParseVariables(int64_t id, int lineIndex, bool enabled) = 0;
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

// 实体属性（按名同步, ChangeMobProperty）: 目前用于 minecraft:sulfur_cube 的外观档位
// minecraft:sulfur_cube_archetype; 其它实体的 client_sync 属性同样适用。
struct EntityMobProperty {
    std::string name;  // 完整属性名, 如 "minecraft:sulfur_cube_archetype"
    std::string value; // 字符串取值, 如 "sticky"
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
    // ── 1.23.0 追加（冻结契约: 结构尾部追加）──
    // 实体属性（ChangeMobProperty, 按名同步的 client_sync 属性; 目前用于硫磺立方体的外观档位）
    std::vector<EntityMobProperty> mobProperties;
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

    // ── 1.23.0 追加: 实体属性同步（ChangeMobProperty）──
    // 按属性名下发一个字符串取值（enum 型 client_sync 属性, 如硫磺立方体的外观档位）。
    // 实体尚未送到客户端时会记下来, spawn 之后自动补发; respawn 后也会重放。
    virtual bool setMobProperty(int64_t id, std::string const& name, std::string const& value) = 0;
    virtual bool clearMobProperties(int64_t id) = 0;

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
// ─────────────────────────────────────────────
// 真实实体交互事件（1.27.0）: 玩家右键 / 攻击**服务端真实实体**时回调
//   · 来源与 ghost 同一处（协议 944+ 的 ItemUseOnActor 事务 + InteractPacket 兜底）:
//     ghost 路由管"库内协议实体", 这里管"真实实体"（原版生物 / 自定义生物 / 玩家）。
//   · 为什么由库来做: 消费方自己挂钩子 = 往 BDS 热路径上叠 detour 层, 叠到第 4 层会毁掉
//     最内层 trampoline（实测崩溃）。库已经挂在收包点上, 顺手派发即可 —— **零新增层**。
//   · 事件在主线程（收包处理路径）上回调: 回调里只做"入队 / 轻量判断", 重活留给自己的 tick。
// ─────────────────────────────────────────────
struct ActorInteractEvent {
    std::string   playerName;  // 交互发起者（realName）
    std::string   targetType;  // 目标实体类型（如 minecraft:cow; 服务端查不到时为空）
    int           action{0};   // 1 = 右键交互, 2 = 左键攻击（与 GhostInteractEvent 对齐）
    std::uint64_t runtimeId{0};
    std::int64_t  uniqueId{0}; // 目标 uniqueId（0 = 服务端查不到这只实体）
    bool          hasPos{false};
    float         x{0}, y{0}, z{0};
};

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
    std::string pngPath{};     // PNG 文件路径（方形 2 的幂: 64/128/256/512/1024）
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

// 注意：早前"客户端不渲染皮肤、外观回退默认模型"的问题已在 26.40.2 修复
// （PlayerList 皮肤条目的 Id / FullId 补全 + 2168 帧格式校验）。本域全部接口可用且数据链路完整：
// 皮肤注册/在线采集/目录导入、getSkinBlob/registerSkinFromBlob 导出恢复、
// 创建/移动/朝向/缩放/视距/显隐/交互均正常渲染。
class IPlayerNpc {
public:
    virtual ~IPlayerNpc() = default;

    // ── 皮肤注册表（全局; NPC 引用 skinId, 解码一次多处复用）──
    // PNG 注册（方形 2 的幂 ≥ 64; 失败返回 false 并给出错误; 重复注册覆盖）
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

    // ── 1.26.0 追加（冻结契约: 只在尾部追加）──
    // 轻量位置更新: 只发 MoveActorAbsolute（不重建实体 / 不重发皮肤, 无闪烁）,
    // 与 setRotationLight 走同一条轻脏通道, 同一 tick 内自动合并为一条包。
    // 用途: 每 tick 跟随会移动的东西（如"给生物换肤"的载体跟随真身）。
    // dim 与当前不同时返回 false（跨维度请用 setPosition 走重建式刷新）; id 不存在返回 false。
    virtual bool setPositionLight(int64_t id, float x, float y, float z, int dim) = 0;

    // ── 1.26.0 追加: 玩家皮肤注入（把注册表里的皮肤发给玩家看, 含"自己看自己"）──
    // 机制: 用 **target 自己的 UUID / uniqueId** 发一条 PlayerList(Add) 就地更新皮肤条目
    //   （与原版"皮肤更新"、Geyser 对 session 玩家自身的处理一致）。
    //   · viewerName 空串 = 所有在线玩家（含 target 本人）; 非空 = 只发给该玩家;
    //   · targetName 必须是**在线玩家**的 realName（离线/查无此人返回 false）;
    //   · skinId 必须已注册（未注册返回 false）;
    //   · 皮肤条目按注册表原样下发（含 OverridesPlayerAppearance=true 与 trust 三态 ——
    //     这是"覆盖玩家客户端上已装备皮肤"的前提, 不然客户端会拒绝、继续用他自己的皮肤）;
    //   · **不自动重发**: 客户端偶发丢帧时由消费方自己再调一次（Geyser 的做法是 ~100ms 后重发）。
    virtual bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId) = 0;
    // 便捷: 注入给所有在线玩家（含本人）; 等价 injectSkin("", targetName, skinId)
    virtual bool injectSkinAll(std::string const& targetName, std::string const& skinId) = 0;

    // ── 1.26.0 追加: 播放动画（AnimateEntityPacket）──
    // 与 ICustomEntity::playAnimation 同一套机制: 库对"已见过该 NPC 的玩家"发一条
    // AnimateEntityPacket（animation = 资源包里的动画标识符, 如 animation.ms.xxx.idle）,
    // controller 名库内按 NPC id 自动唯一化（不需要资源包里预先存在同名控制器）。
    //   · stopExpression 空串 = 常驻循环; "query.any_animation" = 立刻停;
    //   · durationTicks > 0 时到期自动补发一条停止包（stopExpression = query.any_animation）;
    //   · 动画名必须是**客户端能解析的**（原版动画名, 或随资源包下发的自定义动画）——
    //     皮肤几何里内嵌的 animations 段不参与解析（见 README 的说明）。
    //   · 无观察者 / NPC 不存在时返回 false; 新观察者的补发由消费方经 EntitySpawnCallback 自行处理。
    virtual bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks
    ) = 0;
    // 只发给指定玩家（补发用; 玩家未见过该 NPC 返回 false）
    virtual bool playAnimationTo(
        int64_t id,
        std::string const& playerName,
        std::string const& animation,
        std::string const& stopExpression,
        int durationTicks
    ) = 0;
    // 同一套 EntitySpawnCallback 语义: NPC 对某玩家出生完成后回调（补发动画/状态用）
    using EntitySpawnCallback = std::function<void(int64_t id, std::string const& playerName)>;
    virtual void setEntitySpawnCallback(EntitySpawnCallback callback) = 0;
};

// ─────────────────────────────────────────────
// 村民交易菜单（协议层; 1.21.0 追加, 1.22.0 起为纯展示）
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
// 三条路径（`usePacketOffers` × `settleLocally`）:
//   · 纯展示（默认: usePacketOffers=true, settleLocally=false）: 只发 UpdateTrade, 服务端没有
//     交易表 —— 付费放不进交易槽、点了不成交（天然只读）。
//   · 真结算（1.24.0 追加: usePacketOffers=true, settleLocally=true）: 界面仍是我们自建的,
//     但库把客户端的**付费放置 / 取回 / 成交**请求接住并自己结算 —— 真的从玩家背包扣付费、
//     真的把产物发进背包; 结果走 `TradeSettlementEvent` 回传（LSE 侧 `tradePollSettlements`）。
//     **已知限制（实测, 未打通）**: 客户端要求服务端在物品应答里回带交易槽的**槽位更正**
//     （按客户端分配的物品网络 id）, 而该段在本协议版本与协议库的线格式不一致（协议库多写一个
//     字符串、时长按 varint; 客户端要定长 short）—— 手写会直接把客户端打崩。因此放料虽然被服务端
//     受理（扣款/暂存/退还都是真的）, 客户端界面仍可能把这次放料撤回（表现为"放进槽里又弹回"、
//     成交点不动）。**要稳定成交请走 `usePacketOffers=false`（真实交易表, BDS 结算）或容器 UI。**
//   · 真实交易表（usePacketOffers=false）: 给载体装真实交易表 + BDS `openTrading`, 成交由 BDS 完成。
//
// 不开结算时本域不读任何点击; 开了结算只读"落在交易槽上的物品动作"与带自建配方 id 的
// CraftRecipe 动作, 其余一律放行。要"点条目就有回调"的列表界面仍推荐虚拟容器（IContainerMenu）。
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
    // 非 0 = 直接用该实体（真实村民的 uniqueId）作为交易对象, 且**不发自建 offers**:
    // 改为调用 BDS 自己的 Player::openTrading, 于是客户端看到的交易表就是服务端持有的那一份。
    // 0 = 用自建载体 + 自建 offers（默认路径）。
    // true（默认）= 纯协议层: 只发我们自己构造的 UpdateTrade。界面完全由这个包打开, 服务端不放
    // 交易表 —— 天然只读（与参考实现 GMLIB ChestUI 同路）: 玩家往付费槽放东西的请求会被 BDS
    // 拒掉、物品弹回, 界面也就停在"不可成交"。纯展示正合适。
    // false = 给载体装真实交易表 + 走 BDS 自己的 openTrading —— 客户端与服务端持有同一份交易表,
    // 玩家是**真的在交易**（物品真的消耗）。要"展示 + 真成交"就用它。
    bool                        usePacketOffers{true};
    std::int64_t                carrierUniqueIdOverride{0};
    std::vector<TradeMenuOffer> offers;
    // 载体实体类型(1.21.0 追加): "minecraft:villager_v2"(默认) 或
    // "minecraft:wandering_trader"(流浪商人 —— 界面外观与生物头图随类型变化)。
    // 两条路径都适用; 真实交易表路径下流浪商人同样持有 EconomyTradeableComponent, 装表方式相同。
    // **追加在尾部**: 保持既有字段偏移不变。
    std::string                 carrierIdentifier{"minecraft:villager_v2"};
    // **1.24.0 追加**: 纯协议层路径下的"真结算"（usePacketOffers 必须为 true 才有意义）。
    //   false（默认）= 纯展示: 付费放不进交易槽、点了不成交;
    //   true = 库接住客户端的付费放置/取回/成交请求并**自己结算** —— 真的从玩家背包扣付费、
    //          真的把产物写进背包（不依赖服务端交易表）。结果走 TradeSettlementEvent 回传。
    //   **已知限制**: 客户端要求服务端回带交易槽的槽位更正, 而该段的线格式在本协议版本与协议库
    //   不一致（手写会把客户端打崩）, 所以客户端可能把放料撤回 —— 稳定成交请用
    //   `usePacketOffers = false`（真实交易表, BDS 结算）或容器 UI。详见域注释。
    //   开着的菜单也可用 ITradeMenu::setSettleLocally 随时切换。
    bool                        settleLocally{false};
};

// 真结算的回传（成功/失败各一条; 失败时 reason 给原因）
struct TradeSettlementEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         offerIndex{-1}; // 成交的是第几条（-1 = 定位不到）
    bool        ok{false};      // true = 已真结算（付费已扣、产物已发）
    std::string reason;         // ok=false: "not-owner" / "payment-missing" / "inventory-full" ...
};

// 结算回传的格式化（LSE 轮询条目, 与 containerPollClicks 同款: 一行可切分）
//   "player=Steve menuId=3 offer=1 ok=1 reason="

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

    // 追加一条交易并就地重发交易表（不必重开界面）; 返回 false = 菜单已不在
    // （LSE 侧 tradeAddOffer 用的就是它）
    virtual bool addOffer(int64_t menuId, TradeMenuOffer const& offer) = 0;
    // 改显示栏值 / 经验条（同样就地重发）
    virtual bool setTier(int64_t menuId, int tier, int experience) = 0;

    // ── 1.24.0 追加: 纯协议层真结算 ──
    // 结算开关（打开前用 TradeMenuSpec::settleLocally 定初值; 这里可随时切）
    virtual bool setSettleLocally(int64_t menuId, bool on) = 0;
    [[nodiscard]] virtual bool isSettleLocally(int64_t menuId) const = 0;
    // 结算回传: C++ 监听器
    virtual uint64_t addSettlementListener(std::function<void(TradeSettlementEvent const&)> listener) = 0;
    virtual bool     removeSettlementListener(uint64_t token) = 0;
    // LSE 轮询: 取走并清空（条目 = 上面 formatSettlement 的一行格式）
    virtual std::vector<std::string> pollSettlements() = 0;
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

    // ── 26.40.8 追加（尾部）: 聊天框内显示的头像/模型的自定义 ──
    // avatarSkinVariant: NPC **内置皮肤变体**（0..59, 即 NpcData.skin_list 里的 variant 值）——
    //   写进载体 ActorData 的 `SkinId(104)` 项, 对话界面里的头像/模型就用这张内置皮肤;
    //   -1（默认）= **不加这一项**（与旧行为逐字节一致; 离线对拍不回归）。
    // avatarViewSpec: 在这只载体上**叠加一层 IViewOverride**（spec 语法同 viewEntity,
    //   如 "type=minecraft:zombie"/"skin=<皮肤id>"/"name=§6酒保"/"always=1"）—— 载体生成时按视图
    //   语义应用到载体自身: `type` 换载体类型、（测试矩阵: 僵尸 / 鸡）
    //   `skin` 用 **playernpc 注册表里的皮肤**（MHR/MeowSkin 注册/采集的都在这张表）→ 载体改为
    //   **玩家模型**（PlayerList(Add)+AddPlayer, 同名同 id; Tab 条目 1s 后摘掉）、`name` 换界面标题/
    //   交互文字。**边界与 carrierIdentifier 相同**: `type` 换非 NPC 家族是否还能弹出对话界面
    //   需要实机验证（界面与该实体类型绑定, 见 carrierIdentifier 注释）; 皮肤类对非玩家实体无效。
    //   要"整只模型完全自定义"也可走既有 npcUniqueIdOverride 路线:
    //   用视图域/自定义实体生成展示实体, 把对话挂在它的 id 上 —— 界面按该实体的客户端样子渲染。
    int         avatarSkinVariant{-1};
    std::string avatarViewSpec;
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
// 东西（天然只读）。适合当任务列表、成就列表、商店预览这类"只展示 + 点击回调"的界面。
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
    // **实测(26.40 客户端): 7 能正常开界面, 6 不行** —— 默认 7(~350ms; GMLIB 等效 14 tick
    // ≈700ms)。下限与客户端/机器有关, 换设备或负载高时可能要回调大, 所以留成可调。
    // **追加在尾部**: 保持既有字段偏移不变。
    int                           openDelayTicks{7};
    // **可交互模式**（1.23.0 追加）: true = 客户端在本容器**内部**拖动/交换物品时, 库自己接住这条
    // 请求并回成功, 同时把改动记进条目表 —— 物品真的留在新格子（不会再被 BDS 打回）。
    // 只接"整条请求都落在本容器"的动作（容器内移动/交换/拆分）; 涉及玩家背包的动作照旧放行给 BDS
    // （服务端没有这个容器 → 客户端回滚, 与 false 时一致 —— 那是服务端背包真实性的固有边界）。
    // **追加在尾部**: 保持既有字段偏移不变。
    bool                          interactive{false};
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

    // ── 可交互模式开关（1.23.0 追加）──
    // 打开后客户端在容器内部的拖动/交换会被库接住并生效（见 ContainerMenuSpec::interactive）;
    // 关掉就回到"只回传点击、物品放不住"的默认行为。返回 false = 菜单已不在。
    virtual bool setInteractive(int64_t menuId, bool on) = 0;
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
    //  实测限制（26.40 客户端）: 原版 NPC 对话界面在**点击任意按钮时客户端就会自行收起**,
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
// 背包虚容器（协议层伪造玩家背包内容; 1.23.0）
//
// 机制: 直接改写**客户端**看到的玩家背包 —— 一条 InventoryContentPacket
// （ContainerId = Inventory(0)、FullContainerName = InventoryContainer(29)、Slots = 0..35 号槽位）
// 把整份内容换成调用方给的那一份; 单格改动走 InventorySlotPacket。服务端背包一个字都不动,
// 物品是"看起来有"。参考实现 GMLIB 的 ChestUI 填玩家物品栏用的就是同一条路（它逐格写
// InventorySlot, 容器 id 同样是 Inventory(0) + InventoryContainer(29)）。
//
// 功能项与虚拟容器（IContainerMenu）一致: 玩家点自己背包里的伪造物品 → 回调报槽位号,
// 物品不会真的被拿走 —— 客户端按伪造内容发出请求, 服务端真实槽位对不上 → BDS 判失败 →
// 客户端把预测撤回（与虚拟容器同一套表现, 不需要库代答）; 库随后把那一格重发一次,
// 让伪造内容不会被这次回滚冲掉。
//
// 与交易菜单 / 虚拟容器**共存**: 打开交易界面或箱子界面时 BDS 会重发玩家背包内容
// （伪造内容被覆盖）, 所以本域默认按 refreshIntervalTicks 周期重发（默认 20 tick = 1s）,
// 也可随时调 refresh() —— 界面上看到的背包区域因此始终是伪造内容, 点击照常回调。
//
// 边界（实测前先写清楚, 免得误用）:
//   · 伪造只影响客户端显示。玩家"真正能用/能吃"的仍是服务端真实物品。
//   · 若某格真实物品与伪造物品恰好一致, 那一次操作会被服务端当真执行 —— 想让某格纯展示,
//     别把它伪造成与真实物品相同的东西。
//   · 该格真实物品被别的原因改变（捡东西/用物品/别的插件）时, 周期重发会把伪造内容重新盖回去。
//
// 槽位编号与原生背包容器一致: 0..8 = 快捷栏, 9..35 = 主背包（共 36 格）。
// 护甲(6)/副手(34)不在本域范围内 —— 它们各有独立的容器枚举, 需要的话后续再扩。
// ─────────────────────────────────────────────

inline constexpr int kFakeInventorySlots = 36;

struct FakeInventorySpec {
    std::vector<ContainerMenuItem> items; // 下标即槽位（0..35）; type 空 = 该格留空
    // 周期重发间隔（tick）。0 = 不周期重发（只在下发、单格更新、显式 refresh 时发）。
    // 为什么要周期重发: 任何一次真实背包变动都会让 BDS 重发受影响的槽位, 打开箱子/交易界面时
    // 更是整包重发, 伪造内容会被覆盖; 周期重发把它盖回去。**追加在尾部**: 保持字段偏移不变。
    int refreshIntervalTicks{20};
};

struct FakeInventoryClickEvent {
    std::string playerName;
    int         slot{-1}; // 被点击的伪造槽位（0..8 快捷栏, 9..35 主背包）
};

class IFakeInventory {
public:
    virtual ~IFakeInventory() = default;

    // 下发整份伪造背包（覆盖该玩家此前的伪造内容）; 玩家不在线返回 false
    virtual bool apply(std::string const& playerName, FakeInventorySpec const& spec) = 0;
    // 单格改动（一条 InventorySlot, 无延迟）; 该玩家尚未 apply 过则返回 false
    virtual bool setSlot(std::string const& playerName, int slot, ContainerMenuItem const& item) = 0;
    // 立即重发当前伪造内容（打开界面后、或发现被覆盖时用）
    virtual bool refresh(std::string const& playerName) = 0;
    // 撤销伪造: 停掉周期重发, 并把**真实**背包内容重发一遍（恢复真相）
    virtual bool clear(std::string const& playerName) = 0;
    virtual void clearAll() = 0;

    [[nodiscard]] virtual bool                     isActive(std::string const& playerName) const = 0;
    [[nodiscard]] virtual std::vector<std::string> getActivePlayers() const = 0;

    // 点击回传（多播, 多个插件可同时注册）; 返回 token（0 = 失败）
    virtual uint64_t addClickListener(std::function<void(FakeInventoryClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token) = 0;
};

// ─────────────────────────────────────────────
// 硫磺立方体展示（1.23.0 追加）
//
// 第二种"把东西摆到世界上"的做法: 生成一只 `minecraft:sulfur_cube`（硫磺立方体）, 把要展示的方块/
// 物品**装进它的主手** —— 行为包里立方体就是靠 `slot.weapon.mainhand` 拿着"吞下去"的方块, 客户端按
// 装备渲染, 看起来就是方块被吞在它身上; 外观档位用 `minecraft:sulfur_cube_archetype` 属性同步
// （ChangeMobProperty, 13 档: none/regular/bouncy/slow_bouncy/slow_flat/fast_flat/light/fast_sliding/
// slow_sliding/sticky/high_resistance/explosive/hot）。
//
// **吞方块是本域的核心 API**: `SulfurDisplaySpec::block`（或 `setBlock`）就是"它吞下去的东西" ——
// 任何物品都能放, 建议放方块类物品（观感即"方块被吞在它身上"）。
// **隐身是一个参数, 而且默认就是开**: `SulfurDisplaySpec::invisible`（或 `setInvisible`）。
// **实测（26.40 客户端）: 立方体隐身时, 主手里"吞下去的方块"照常渲染** —— 于是默认 true =
// 只看到被吞的那个方块（这就是本域作为"第二种展示方式"的默认观感）; 想看立方体本体就设 false。
// （对比: NPC 头像那次隐身会把头像一起抹掉, 所以那里的载体不能隐身 —— 两个实体不一样, 别套用。）
//
// **"吞生物"没有做**: 协议层实体没有 AI, 真正吞并/消化做不到; 试过让另一个实体骑在立方体上做近似
// （骑乘位置/碰撞都调不出"被吞进去"的观感）, 已按实测结论整体移除 —— 本域只做方块与隐身。
// ─────────────────────────────────────────────

struct SulfurDisplaySpec {
    float       x{0.0f};
    float       y{64.0f};
    float       z{0.0f};
    int         dim{0};
    // **吞下去的方块**（走主手装备; 这是本域的核心参数）。type 空 = 空手
    ContainerMenuItem block;
    // 外观档位（minecraft:sulfur_cube_archetype）; 空串 = 不下发, 保持客户端默认
    std::string archetype{"regular"};
    int         variant{2};       // 1=小 / 2=中（中 = 含方块那一档）
    bool        invisible{true};  // 立方体隐身（**默认开**: 实测隐身时"吞下去的方块"照常渲染 → 只留内容）
    float       scale{1.0f};
    double      viewDistance{0.0};            // <=0 = 不限
    std::vector<std::string> visiblePlayers;  // 空 = 全员可见
};

class ISulfurDisplay {
public:
    virtual ~ISulfurDisplay() = default;

    // 生成一只硫磺立方体展示; 返回展示 id（<0 = 失败）
    virtual int64_t create(SulfurDisplaySpec const& spec) = 0;
    // 换"吞下去"的方块/物品（改主手装备, 会重建实体: 有一次 respawn）
    virtual bool    setBlock(int64_t id, ContainerMenuItem const& item) = 0;
    // 换外观档位（ChangeMobProperty; 不需要重建实体）
    virtual bool    setArchetype(int64_t id, std::string const& archetype) = 0;
    virtual bool    setInvisible(int64_t id, bool on) = 0;
    virtual bool    setScale(int64_t id, float scale) = 0;
    virtual bool    destroy(int64_t id) = 0;
    virtual void    destroyAll() = 0;

    [[nodiscard]] virtual bool                 exists(int64_t id) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const = 0;
    // 诊断: 背后的自定义实体 id（-1 = 不存在）
    [[nodiscard]] virtual int64_t entityIdOf(int64_t id) const = 0;
};

// ─────────────────────────────────────────────
// 客户端视图覆盖（协议层拦截; 1.25.0 新增）
//
// 让服务端在**不改动真实世界**的前提下，改变**某个玩家客户端上看到的东西**：
//   · 实体（含玩家）: 换成另一种生物来显示 / 换名字牌 / 对这名玩家隐藏
//   · 方块: 把某坐标的方块显示成另一种方块
//
// 机制（两条纪律, 各司其职）:
//   ① **只拦不发**: 出站钩子挂在 BDS 的按收件人发包汇合点（NetworkSystem::send / sendToMultiple）,
//      它**只决定"这一包发给这名玩家吗"**（放行 / 丢弃）—— **从不修改引擎包对象里的字段**。
//      需要"改"的地方一律: 丢掉原包 + 由库**自己手写协议包**补发（sculk 构造 → 回读校验 →
//      原始字节发送, 见 src/view/ViewPackets.h）。
//   ② **心跳**: 服务器每 tick 自查 —— 实体重新进入视野 / 玩家换区块时把覆盖重新推一遍。
//      出生包拦漏时靠它恢复; 方块覆盖被区块重发冲掉也靠它。没用到时只是一次原子读。
//
// 数据来源**只有协议包**: 位置/朝向/输入位来自入站 PlayerAuthInput; 出生包类型/位置/名字来自
// 出生包载荷; 动作来自 Animate / ActorEvent —— **不读服务端实体状态**（唯一例外是"认人":
// 网络标识 → 玩家、AddPlayer 里反查 uniqueId）。
//
// 关键性质:
//   · **逐玩家**: 覆盖只对指定玩家生效（playerName 传空串 = 所有玩家），其他玩家看到原样
//   · **自己不看自己**: 覆盖目标是玩家时, **该玩家本人的客户端不参与覆盖** —— 不发/不吃关于
//     他自己实体的包（RemoveActor / 同 runtimeId 的 AddActor / 位移打回本地玩家会让客户端
//     状态错乱甚至卡死, 2026-10-09 实证）。**例外: 换肤类（`asPlayer`）含本人** ——
//     PlayerList(Add) 发给本人是"自己看自己被换肤"的正路（同 IPlayerNpc::injectSkin 的约定）
//   · **只改客户端视图**: 服务端世界 / 存档 / 碰撞 / 其他插件看到的都是真实内容
//   · **可阻断**: hidden = true 时该实体的出生包与后续更新包不再发给这名玩家
//   · 实体按 uniqueId 认（稳定 id）; 库自动记录它与运行时 id（runtimeId）的对应关系
//   · **不认识 / 解析不出的包一律原样放行**（fail-safe: 宁可没效果，也不给客户端送可疑字节）
//   · 覆盖只存在内存里（重启服务器即失效）; 按玩家名的覆盖在该玩家重进后仍然有效 ——
//     实体重新出生时自动套用, 方块随区块送达自动补发
//
// 第一版的边界（写清楚免得误用）:
//   · **换实体类型**: 非玩家生物直接换; **玩家实体走"替换"**（见下）; 换玩家皮肤用 asPlayer。
//   · **非玩家实体换"我们的皮肤" = 同 id 替换**（1.27.0: `EntityView::skinId`）: 库吃掉它的出生包,
//     用**同一个 runtimeId/uniqueId** 发 PlayerList(Add)+AddPlayer（玩家模型 + 已注册皮肤, 可带
//     `scale`）—— 客户端看到我们的模型, 服务端那边还是那只生物, 攻击/瞄准/掉落全落在它身上。
//     这是"给生物换自定义模型"的正路（PlayerNpc 载体那套要额外隐形真身 + 逐 tick 跟随, 这一条不用）。
//   · **玩家变生物 = 替换（不是代理）**: 吃掉他的出生包, 用**同一个 runtimeId/uniqueId** 发一只
//     该类型的实体; 他的 MovePlayer 对这名观看者也吃掉, 位置/朝向由库按他发来的输入包用
//     MoveActorAbsolute 推。**因为 id 没变, 服务端那边仍然是那个真玩家 —— 打到它身上的攻击由
//     服务端按真身结算**。撤销时把替代实体移走（真身要等重进/换维度才重新出现）。
//   · **缩放 / 发光 / 隐身做不成逐玩家**: 26.40 的元数据表里没有 scale，也没有 glowing /
//     invisible 旗标（基岩版的隐身是效果, 走 MobEffectPacket）。这三项没有进本版 API。
//   · **方块覆盖**在区块重发后会丢，库会自动补发（该玩家收到覆盖范围内的区块时）。
//   · 撤销时离得太远（>64 格）或已经消失的实体不重建 —— 客户端上本来也没有它，等它下次出生即可。
//   · `identifier` 换的是**原版**类型（客户端注册表里有的）; 要换成**自定义模型/贴图**用 `skinId`
//     （皮肤走玩家皮肤通道, 客户端不需要资源包 —— 同 IPlayerNpc 的皮肤）。
//   · 覆盖的是"包里本来要发给该玩家的内容": 若某实体/方块本来就不发给该玩家（例如视野外），
//     覆盖不会凭空让客户端看见它。
// ─────────────────────────────────────────────

// 实体的客户端视图覆盖（空字段 / false = 该项不改）
struct EntityView {
    // 换成该实体类型（如 "minecraft:cow"）; 空 = 不改类型。
    //   · 非玩家实体: 库移除客户端那只 + 用同一个 runtimeId/uniqueId 重发一只该类型的;
    //   · **玩家实体**: 玩家模型渲染不了生物 → 库走**替换** —— 吃掉他的出生包, 用**同一个
    //     runtimeId/uniqueId** 发一只该类型的实体, 位置/朝向按其输入包用 MoveActorAbsolute 推。
    //     **因为 id 没变, 服务端那边仍是那个真玩家 —— 打到它身上的攻击由服务端按真身结算。**
    //     撤销时把替代实体移走（该观看者要等重进/换维度才再看到真身）。
    std::string identifier;
    // 换成**另一名在线玩家**的样子（皮肤取自那名玩家）; 空 = 不改。只对玩家实体有效。
    //   机制与原版"皮肤更新"一致: 用该实体自己的 UUID 再发一条 PlayerList(Add) 就地更新,
    //   且只发给这名观看者（撤销 = 用他自己的皮肤再发一次, 立刻恢复）。
    std::string asPlayer;
    // 换成**已注册皮肤**（含自定义模型 / 自定义贴图; 注册表见 IPlayerNpc::registerSkin / captureSkin）。
    //   只对**非玩家**实体有效（玩家实体换皮走 asPlayer / IPlayerNpc::injectSkin）。机制与"换类型"同源
    //   —— **拦下的包 + 我们自己的包替换**: 库吃掉这只实体的出生包, 用**同一个 runtimeId / uniqueId**
    //   发 PlayerList(Add)+AddPlayer（玩家模型 + 该皮肤）。
    //   于是客户端看到的是我们的模型, 而 **id 没变** —— 服务端仍认它是那只生物:
    //   点它 / 打它 / 瞄准它都还是它本人（伤害、掉落、其他插件看到的全是真身）。
    //   移动: 引擎发给生物的 MoveActor* 对这些观看者被吃掉, 位置/朝向由库每 tick 用
    //   MoveActorAbsolute 推（与 PlayerNpc 的 setPositionLight 同一条通道）。
    //   撤销 = clearEntity（重发一次真实生物的出生包, 立刻恢复原样）。
    std::string skinId;
    // 皮肤模型的缩放（配合皮肤的设计高度; 0 = 不缩放）。客户端碰撞箱（元数据 53/54）等比 ——
    // 想让"手感"与原生物一致就传 生物高度/1.8。
    float       scale{0.0f};
    // 皮肤模型的垂直微调（方块; 正数 = 抬高）。**只在 skinId 生效时用**:
    // 实体位置锚在"脚位", 而模型的几何原点不一定在脚底（导入来源五花八门）——
    // 想整体抬/沉一点就在这儿给, 出生包与逐 tick 的位移推送会一起带上（不会一高一低）。
    // ⚠ 本字段追加在结构体**末尾**: 构造 EntityView 的消费方需要**重新编译**
    //   （旧布局的实例会让库读到界外; 库的 ABI 描述符自检就是为了拦这个）。
    float       yOffset{0.0f};
    // true = 对这名玩家隐藏这只实体（出生包与后续更新包都不再发; 已经在客户端上的会立刻移除）
    bool        hidden{false};
    // 名字牌: hasNametag = true 才动名字牌（用它区分"清空名字"与"不改"）
    bool        hasNametag{false};
    std::string nametag;
    bool        nametagAlwaysShow{false};
};

// 方块的客户端视图覆盖（type 空 = 不改）
struct BlockView {
    std::string type;     // 换成该方块（如 "minecraft:diamond_block", 取默认状态）
};

class IViewOverride {
public:
    virtual ~IViewOverride() = default;

    // 让 playerName（空串 = 所有玩家）在客户端把 entityUniqueId 这只实体看成 view 描述的样子。
    // 实体还没出生（客户端还没见到它）也能先设 —— 库在它出生时套用。
    // 返回 false = 玩家不在线。
    virtual bool overrideEntity(std::string const& playerName, std::int64_t entityUniqueId, EntityView const& view) = 0;
    // 让 playerName（空串 = 所有玩家）在客户端把 (x,y,z) 看成 view.type 那种方块。
    // 返回 false = 玩家不在线 / 类型名无效（不在注册表里）。
    virtual bool overrideBlock(std::string const& playerName, int x, int y, int z, BlockView const& view) = 0;

    // 撤销单个实体的覆盖
    virtual bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId) = 0;
    // 撤销单个方块的覆盖（立刻把真实方块推给该玩家）
    virtual bool clearBlock(std::string const& playerName, int x, int y, int z) = 0;
    // 撤销某玩家的全部覆盖（下线 / 切场景时调用; 空串 = 全部玩家）
    virtual void clearAll(std::string const& playerName) = 0;

    // 诊断: 某玩家当前的覆盖条数摘要, 形如 "entities=2 blocks=5"
    [[nodiscard]] virtual std::string describeFor(std::string const& playerName) const = 0;

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

    // 库版本（BCD: 0x012100 = 1.27.0, 与 HOLOGLIB_API_VERSION 同值）
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

    // ── 1.21.0 追加, 1.22.0 起为纯展示（冻结契约: 只在尾部追加）──
    virtual ITradeMenu& tradeMenus() = 0;

    // ── 1.21.0 追加（冻结契约: 只在尾部追加）──
    virtual INpcDialogue& npcDialogs() = 0;

    // 虚拟容器（列表）界面（1.21.0 追加, 尾部追加保持 ABI 兼容）
    virtual IContainerMenu& containerMenus() = 0;

    // ── 感知域(1.21.0 追加): 客户端设备判断 ──
    virtual IPlayerSensing& playerSensing() = 0;

    // ── 背包虚容器（1.23.0 追加, 尾部追加保持 ABI 兼容）──
    virtual IFakeInventory& fakeInventories() = 0;

    // ── 硫磺立方体展示（1.23.0 追加, 尾部追加保持 ABI 兼容）──
    virtual ISulfurDisplay& sulfurDisplays() = 0;

    // ── 客户端视图覆盖（1.25.0 追加, 尾部追加保持 ABI 兼容）──
    // 拦截 BDS 发给玩家的原始包（只决定放行 / 丢弃）, 改变**该玩家客户端看到的内容**（实体类型/方块/元数据）。
    virtual IViewOverride& viewOverrides() = 0;

    // ── 1.27.0 追加（冻结契约: **只在类尾部追加**）──
    // 真实实体交互多播监听（玩家右键/攻击真实实体时回调; 复用库已有的收包钩子, 不新增 detour 层）。
    // 返回 token（0 = 失败）; removeActorInteractListener(token) 移除; 事件在主线程回调。
    //
    // ⚠ 位置说明（写给自己与后来者）: 这两个函数**必须留在类的最尾部**。
    //   第一版把它们插在了 ghost 段之后（中段）—— 结果后面所有槽位整体前移,
    //   用旧头文件编译的消费方（MeowHolographicRenderer 等）在 `playerNpcs()` 上取到了错的槽,
    //   直接 0xC0000005（实机复现）。冻结契约不是礼貌, 是 ABI。
    virtual uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener) = 0;
    virtual bool     removeActorInteractListener(uint64_t token) = 0;
};

} // namespace hologramlib

// ─────────────────────────────────────────────
// 消费方自检 + 库侧"调用前检查消费方"
// ─────────────────────────────────────────────
// 双方各自**导出**自己的戳, 互相读对方的那份:
//   · 消费方 enable 时读 DLL 导出的 hologramlib_abiStamp（函数）→ 不一致就**拒绝启用**;
//   · 库在唯一入口 getInstance 处读**调用方模块**导出的 hologramlib_consumerAbiStamp（数据）
//     → 不一致就记一笔, 可用 hologramlib_abiStatus 查。库侧**不在这里杀进程**: 该消费方可能
//     压根没用到错位的槽; 真正的拒绝由消费方那一侧做（它能往日志里写重新编译/同批部署）。
// 关于"自动": 试过用 MSVC 成员函数指针编码在编译期自动算槽位号, 实测拿到的是 thunk 地址而非
//   槽位号, 不可靠, 已放弃。真正自动的做法是**构建期用脚本解析本头文件生成槽位表**（下一步）;
//   在那之前这个戳靠人工维护 —— 规则: **任何动到虚表形状的改动（中段插入/删除/重排）都必须 +1**,
//   仅**尾部追加**不用动它（追加不改变已有槽位）。
// 覆盖不到: 没带这个导出的老构建（如 2026-10-10 崩溃里那版 MSkinventory）→ 库侧记为"未声明",
//   消费方侧读不到库的戳时按"不同批构建"处理（拒绝启用）。
inline constexpr char const* HOLOGLIB_CONSUMER_STAMP_SYMBOL = "hologramlib_consumerAbiStamp";
extern "C" inline __declspec(dllexport) std::uint32_t hologramlib_consumerAbiStamp = HOLOGLIB_ABI_STAMP;
