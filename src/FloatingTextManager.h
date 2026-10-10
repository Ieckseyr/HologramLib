#pragma once

// ─────────────────────────────────────────────────────────────
// 【重构来源标注 · LGPL-3.0】
//   本文件的「动态行」实现（FloatingTextLine 的内容池 / 轮播间隔 / 行级变量开关、
//   时间取模轮播、0.5s 节流刷新与逐玩家重发、内容去重）**重构自 Phantom**：
//     · 项目: github.com/GroupMountain/Phantom  (LeviLamina 基岩版悬浮字插件)
//     · 许可: GNU Lesser General Public License v3.0 (LGPL-3.0)
//   依 LGPL-3.0 的要求在此标注来源。该重构部分及其演绎作品按 LGPL-3.0 分发，
//   版权归 Phantom 贡献者（GroupMountain）所有；本库其余部分为 MIT（见 LICENSE）。
// ─────────────────────────────────────────────────────────────

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace debugshape_export {

// 颜色结构（各分量 0~1）
struct Color4f {
    float r, g, b, a;

    Color4f() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color4f(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

// 一行 = 文本 + 动态字段（**重构自 Phantom**, LGPL-3.0: 内容池轮播 / 行级变量开关）
// （来源与许可见本文件顶部标注）
struct FloatingTextLine {
    std::string              text;                  // 静态文本（无池 / 池为空时显示）
    std::vector<std::string> pool;                  // 内容池（>=2 项且 intervalMs>0 时轮播）
    int                      poolIntervalMs{0};     // 轮播间隔（毫秒; 时间取模 = 无状态）
    bool                     parseVariables{true};  // false = 原样显示（含 { } 字面量）
};

// 悬浮字实体（"整块文本单一背景框"模型: 所有行合并为**一个**文本形状,
// 样式（颜色/缩放/背景框/穿墙/旋转）都是**整块**属性 —— 行只负责文本本身）
struct FloatingText {
    int64_t id;
    float x, y, z;
    int dimId = 0;

    std::vector<FloatingTextLine> lines;

    // ── 整块样式 ──
    Color4f color{1.0f, 1.0f, 1.0f, 1.0f};  // 文字颜色（mColor）
    float   scale = 1.0f;                    // 整块缩放（mScale）
    std::optional<Color4f> backgroundColor;  // 背景框颜色（mBackgroundColor; 空 = 客户端默认色）
    bool    depthTest = false;               // 穿墙可见性（mDepthTest: false = 始终渲染/穿墙可见, true = 被方块遮挡）
    bool    useRotation = false;             // true = 整块不再面向相机, 用下面三轴（mUseRotation + mRotation）
    float   rotPitch = 0.0f, rotYaw = 0.0f, rotRoll = 0.0f;

    std::string followPlayer;       // 跟随的玩家名 (空=不跟随; 位置在 setFollowPlayer/draw/refresh 时就地解析)
    float followOffsetY = 2.0f;     // 跟随时的Y偏移

    // 绘制目标 (重发/刷新时按此路由)
    enum class DrawTarget { None, All, Dimension, Player };
    DrawTarget drawTarget = DrawTarget::None;
    std::string targetPlayer;       // DrawTarget::Player 时的目标玩家名

    // 内部状态: 单一多行文本形状 (\n 合并, 所有文字共用同一个背景框)
    int64_t textShapeId = -1;
    bool isDrawn = false;

    // ── 逐玩家变量: 文本含 {var} 且目标不是单玩家时, 按观看者各建一个形状 ──
    // (观看者名 → 形状 id; 每个观看者收到按自己名字解析的文本)
    std::unordered_map<std::string, int64_t> viewerShapes;

};


/**
 * FloatingTextManager - 高级悬浮字管理器
 *
 * 功能:
 * - 多行文本（\n 合并为单一文本形状）
 * - 整块样式: 颜色 / 缩放 / 背景框颜色 / 穿墙开关 / 三轴旋转
 * - 跟随玩家（位置在调用时就地解析 —— 不做库内自驱）
 * - 动态变量替换（含逐观看者解析）
 */
class FloatingTextManager {
public:
    static FloatingTextManager& getInstance();
    
    // 禁止拷贝
    FloatingTextManager(const FloatingTextManager&) = delete;
    FloatingTextManager& operator=(const FloatingTextManager&) = delete;
    
    // 创建与销毁
    
    // 创建悬浮字 (返回ID)
    int64_t create(float x, float y, float z);
    
    // 销毁悬浮字
    bool destroy(int64_t id);
    void destroyAll();
    
    // 行管理（行只负责文本; 行级样式差异请在文本内嵌 § 颜色代码）
    
    // 添加一行文本
    bool addLine(int64_t id, const std::string& text);
    
    // 设置指定行的文本
    bool setLineText(int64_t id, int lineIndex, const std::string& text);
    
    // 移除指定行
    bool removeLine(int64_t id, int lineIndex);
    
    // 清空所有行
    bool clearLines(int64_t id);
    
    // 获取行数
    int getLineCount(int64_t id);
    
    // 整块样式（1.26.0: 行级颜色/缩放已移除, 样式一律整块）
    
    // 设置整块纯色
    bool setColor(int64_t id, float r, float g, float b, float a = 1.0f);
    
    // 设置整块缩放（替代原 setLineScale —— 行级缩放从未生效）
    bool setScale(int64_t id, float scale);
    
    // 设置背景框（文本底板）颜色; 不设 = 客户端默认色
    bool setBackgroundColor(int64_t id, float r, float g, float b, float a = 1.0f);
    bool clearBackgroundColor(int64_t id);
    
    // 穿墙可见性（depthTest）: true = 被方块/实体遮挡; false = 始终渲染（穿墙可见, 默认）
    bool setDepthTest(int64_t id, bool enabled);
    
    // 整块三轴旋转（度, [Pitch, Yaw, Roll]）: 设置后整块不再面向相机; clearRotation 恢复面向相机
    bool setRotation(int64_t id, float pitch, float yaw, float roll);
    bool clearRotation(int64_t id);

    // ── 动态行（1.26.0 追加; **重构自 Phantom**, LGPL-3.0 —— 见文件头标注）──
    // 内容池: 每 intervalMs 毫秒轮播一项（时间取模 = 无状态, 多个浮字同池同相）;
    //   池 <=1 项或 intervalMs <=0 = 不轮播（显示第 0 项）; content 为空 = 清除池（回到 setLineText 文本）。
    //   轮播由库内 0.5s 节流刷新承担（**内容变了才重发**）—— 区别于 26.40.8 移除的自驱"偏移"
    //   （那时每拍都在动、但驱出来的东西没有消费方）; 这里每次刷新都是客户端可见的真变化。
    bool setLinePool(int64_t id, int lineIndex, const std::vector<std::string>& content, int intervalMs);
    // 该行是否解析变量（内置 {player}/{online}/{time}/{tps}/{dimension}/{x}/{y}/{z} + MeowPAPI 外部占位符）
    bool setLineParseVariables(int64_t id, int lineIndex, bool enabled);
    
    // 位置与跟随
    
    // 设置位置
    bool setLocation(int64_t id, float x, float y, float z);

    // 迁移维度（1.12.0）: 已绘制时同步底层形状维度并按原绘制目标原地重发
    bool setDimension(int64_t id, int dimId);
    
    // 设置跟随玩家
    bool setFollowPlayer(int64_t id, const std::string& playerName, float offsetY = 2.0f);
    
    // 取消跟随
    bool clearFollowPlayer(int64_t id);
    
    // 显示控制
    
    // 绘制到世界
    bool draw(int64_t id);
    bool drawToDimension(int64_t id, int dimId);
    bool drawToPlayer(int64_t id, const std::string& playerName);
    
    // 移除显示
    bool remove(int64_t id);
    
    // 刷新显示 (重新解析变量/跟随坐标并原地重发合并文本形状)
    bool refresh(int64_t id);
    
    // 动态变量
    
    // 注册变量提供器
    using VariableProvider = std::function<std::string(const std::string& playerName)>;
    void registerVariable(const std::string& name, VariableProvider provider);
    
    // 内置变量: {time}, {online}, {tps}, {player}, {dimension}, {x}, {y}, {z}
    void registerBuiltinVariables();

    // 动态行驱动: 挂/摘 ServerLevelTickEvent 监听（幂等; 由 ModEntry 的 enable/disable 调用）
    void initDynamicDriver();
    void shutdownDynamicDriver();

private:
    FloatingTextManager();
    ~FloatingTextManager() = default;

    // 内部方法
    FloatingText* getFloatingText(int64_t id);
    // 重建单一多行文本形状 (\n 合并所有行)
    // 复用已有 shape (保留 networkId) 以实现客户端原地覆盖, 避免闪烁
    void rebuildTextShape(FloatingText& ft);
    // 销毁文本形状 (发送移除包 + 删除内存)
    void destroyTextShape(FloatingText& ft);
    // 按绘制目标重发形状 (同 networkId 覆盖, 无闪烁)
    bool redrawTextShape(FloatingText& ft);
    // 文本是否含变量占位符 {…}
    [[nodiscard]] static bool textHasVariables(FloatingText const& ft);
    // 逐观看者形状重建(含变量 + 目标不是单玩家时, 代替共享形状)
    void rebuildTextShapesPerViewer(FloatingText& ft);
    // 销毁全部观看者形状(切回共享路径 / 销毁条目时)
    void destroyViewerShapes(FloatingText& ft);
    // 跟随位置就地解析（无库内自驱 tick; 在 setFollowPlayer / draw* / refresh 时调用）
    void resolveFollowPosition(FloatingText& ft);
    // 把整块样式（缩放/颜色/背景框/穿墙/旋转）写到指定形状上
    void applyBlockStyle(int64_t shapeId, FloatingText const& ft);
    std::string processVariables(const std::string& text, const std::string& playerContext);

    // ── 动态行驱动（重构自 Phantom 的 tick 刷新法; LGPL-3.0）──
    // ServerLevelTickEvent 每 tick 调; 内部 10 tick(0.5s) 节流; 只碰"含动态行"的浮字;
    // 组装当前文本与上次下发比对, **变了才重发**（不做无消费的空转）。
    void tickDynamic();
    // 文本是否含"动态行"（有轮播池 / 或开启解析且含 { ）
    [[nodiscard]] static bool hasDynamicLines(FloatingText const& ft);
    // 组装一行的最终文本（池取时间取模项 → 变量解析）
    std::string composeLine(FloatingTextLine const& line, const std::string& playerContext, std::uint64_t nowMs);
    // 组装整块共享文本（\n 合并所有行）
    std::string composeText(FloatingText const& ft, const std::string& playerContext, std::uint64_t nowMs);

    std::unordered_map<int64_t, std::unique_ptr<FloatingText>> mFloatingTexts;
    std::unordered_map<std::string, VariableProvider> mVariables;
    // 上次下发的文本（形状 id → 文本）—— 驱动比对用; 形状销毁时同步清理
    std::unordered_map<int64_t, std::string> mLastSentText;
    std::uint64_t mDynamicTickCounter = 0; // 节流用（0.5s = 10 tick）
    int64_t mNextId = 1;
    std::mutex mMutex;
};

} // namespace debugshape_export
