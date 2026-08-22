#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace debugshape_export {

// 颜色结构
struct LineColor {
    float r, g, b, a;
    
    LineColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    LineColor(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
    
    static LineColor lerp(const LineColor& a, const LineColor& b, float t) {
        return LineColor(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    static LineColor fromHSV(float h, float s, float v, float a = 1.0f);
};

// 渐变线条数据
struct GradientLine {
    int64_t id;
    float x1, y1, z1;  // 起点
    float x2, y2, z2;  // 终点
    LineColor startColor;
    LineColor endColor;
    int segments;      // 分段数
    bool isRainbow;    // 彩虹模式
    float rainbowSpeed;
    float animTime;
    
    std::vector<int64_t> segmentIds;  // 每段线条的 ShapeID
    bool isDrawn;
};

/**
 * GradientLineManager - 渐变线条管理器
 * 
 * 通过将线条拆分成多个短线段来模拟渐变效果
 */
class GradientLineManager {
public:
    static GradientLineManager& getInstance();
    
    GradientLineManager(const GradientLineManager&) = delete;
    GradientLineManager& operator=(const GradientLineManager&) = delete;
    
    // 创建渐变线条
    // segments: 分段数，越多越平滑，但性能开销越大 (建议 10-50)
    int64_t create(float x1, float y1, float z1, 
                   float x2, float y2, float z2,
                   int segments = 20);
    
    // 设置渐变颜色
    bool setGradient(int64_t id, 
                     float r1, float g1, float b1,
                     float r2, float g2, float b2);
    
    // 设置彩虹效果
    bool setRainbow(int64_t id, float speed = 1.0f);
    
    // 设置纯色
    bool setColor(int64_t id, float r, float g, float b, float a = 1.0f);
    
    // 更新端点位置
    bool setEndpoints(int64_t id,
                      float x1, float y1, float z1,
                      float x2, float y2, float z2);
    
    // 绘制
    bool draw(int64_t id);
    bool drawToDimension(int64_t id, int dimId);
    
    // 移除显示
    bool remove(int64_t id);
    
    // 销毁
    bool destroy(int64_t id);
    void destroyAll();
    
    // 动画更新
    void tick(float deltaTime);

private:
    GradientLineManager() = default;
    ~GradientLineManager() = default;
    
    GradientLine* getLine(int64_t id);
    void rebuildSegments(GradientLine& line);
    void destroySegments(GradientLine& line);
    void updateSegmentColors(GradientLine& line);
    
    std::unordered_map<int64_t, std::unique_ptr<GradientLine>> mLines;
    int64_t mNextId = 1;
    std::mutex mMutex;
};

} // namespace debugshape_export
