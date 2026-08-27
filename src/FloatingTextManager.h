#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace debugshape_export {

// 颜色结构
struct Color4f {
    float r, g, b, a;
    
    Color4f() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color4f(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
    
    // 线性插值
    static Color4f lerp(const Color4f& a, const Color4f& b, float t) {
        return Color4f(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    // HSV 转 RGB (用于彩虹效果)
    static Color4f fromHSV(float h, float s, float v, float a = 1.0f);
};

// 滚动方向
enum class ScrollDirection {
    None = 0,   // 不滚动
    Left = 1,   // 向左滚动
    Right = 2   // 向右滚动
};

// 垂直动画类型
enum class VerticalAnimation {
    None = 0,       // 静止
    Bounce = 1,     // 上下弹跳
    Scroll = 2      // 循环滚动
};

// 颜色模式
enum class ColorMode {
    Solid = 0,      // 纯色
    Gradient = 1,   // 渐变
    Rainbow = 2     // 彩虹
};

// 单行配置
struct LineConfig {
    std::string text;
    float scale = 1.0f;
    ColorMode colorMode = ColorMode::Solid;
    Color4f solidColor{1.0f, 1.0f, 1.0f, 1.0f};
    Color4f gradientStart{1.0f, 0.0f, 0.0f, 1.0f};
    Color4f gradientEnd{0.0f, 0.0f, 1.0f, 1.0f};
    float rainbowSpeed = 1.0f;      // 彩虹变化速度
    ScrollDirection scroll = ScrollDirection::None;
    float scrollSpeed = 1.0f;       // 滚动速度 (字符/秒)
};

// 悬浮字实体
struct FloatingText {
    int64_t id;
    float x, y, z;
    int dimId = 0;

    std::vector<LineConfig> lines;
    float lineSpacing = 0.3f;       // 行间距 (多行合并单形状后由客户端渲染行距, 此字段保留兼容)

    VerticalAnimation vertAnim = VerticalAnimation::None;
    float vertAnimSpeed = 1.0f;     // 垂直动画速度
    float vertAnimRange = 0.5f;     // 垂直动画范围

    std::string followPlayer;       // 跟随的玩家名 (空=不跟随)
    float followOffsetY = 2.0f;     // 跟随时的Y偏移

    // 绘制目标 (重发/刷新时按此路由)
    enum class DrawTarget { None, All, Dimension, Player };
    DrawTarget drawTarget = DrawTarget::None;
    std::string targetPlayer;       // DrawTarget::Player 时的目标玩家名

    // 内部状态: 单一多行文本形状 (\n 合并, 所有文字共用同一个背景框)
    int64_t textShapeId = -1;
    float animTime = 0.0f;          // 动画时间累计
    std::vector<float> scrollOffsets;  // 每行的滚动偏移
    bool isDrawn = false;
};


/**
 * FloatingTextManager - 高级悬浮字管理器
 * 
 * 功能:
 * - 多行文本支持，每行独立配置
 * - 颜色渐变/彩虹效果
 * - 滚动动画 (左/右)
 * - 垂直动画 (弹跳/滚动)
 * - 跟随玩家
 * - 动态变量替换
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
    
    // 行管理
    
    // 添加一行文本
    bool addLine(int64_t id, const std::string& text);
    
    // 设置指定行的文本
    bool setLineText(int64_t id, int lineIndex, const std::string& text);
    
    // 设置指定行的缩放
    bool setLineScale(int64_t id, int lineIndex, float scale);
    
    // 移除指定行
    bool removeLine(int64_t id, int lineIndex);
    
    // 清空所有行
    bool clearLines(int64_t id);
    
    // 获取行数
    int getLineCount(int64_t id);
    
    // 颜色设置
    
    // 设置纯色 (整个悬浮字)
    bool setColor(int64_t id, float r, float g, float b, float a = 1.0f);
    
    // 设置指定行纯色
    bool setLineColor(int64_t id, int lineIndex, float r, float g, float b, float a = 1.0f);
    
    // 设置指定行渐变色
    bool setLineGradient(int64_t id, int lineIndex,
                         float r1, float g1, float b1,
                         float r2, float g2, float b2);
    
    // 设置指定行彩虹效果
    bool setLineRainbow(int64_t id, int lineIndex, float speed = 1.0f);
    
    // 动画设置
    
    // 设置指定行滚动
    // direction: 0=无, 1=左, 2=右
    bool setLineScroll(int64_t id, int lineIndex, int direction, float speed = 1.0f);
    
    // 设置垂直动画
    // type: 0=无, 1=弹跳, 2=滚动
    bool setVerticalAnimation(int64_t id, int type, float speed = 1.0f, float range = 0.5f);
    
    // 设置行间距
    bool setLineSpacing(int64_t id, float spacing);
    
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
    
    // 刷新显示 (重新解析变量并原地重发合并文本形状)
    bool refresh(int64_t id);
    
    // 动画更新
    
    // 更新所有动画 (应在tick中调用)
    void tick(float deltaTime);
    
    // 动态变量
    
    // 注册变量提供器
    using VariableProvider = std::function<std::string(const std::string& playerName)>;
    void registerVariable(const std::string& name, VariableProvider provider);
    
    // 内置变量: {time}, {online}, {tps}, {player}
    void registerBuiltinVariables();

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
    std::string processVariables(const std::string& text, const std::string& playerContext);

    std::unordered_map<int64_t, std::unique_ptr<FloatingText>> mFloatingTexts;
    std::unordered_map<std::string, VariableProvider> mVariables;
    int64_t mNextId = 1;
    std::mutex mMutex;
};

} // namespace debugshape_export
