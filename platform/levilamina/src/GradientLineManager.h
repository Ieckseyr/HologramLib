#pragma once

#include "core/Registry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace debugshape_export {

struct LineColor {
    float r, g, b, a;

    LineColor()
        : r(1.0f)
        , g(1.0f)
        , b(1.0f)
        , a(1.0f) {}
    LineColor(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_) {}

    static LineColor lerp(const LineColor& a, const LineColor& b, float t) {
        return LineColor(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t);
    }

    static LineColor fromHSV(float h, float s, float v, float a = 1.0f);
};

struct GradientLine {
    int64_t   id;
    float     x1, y1, z1;
    float     x2, y2, z2;
    LineColor startColor;
    LineColor endColor;
    int       segments;
    bool      isRainbow;
    float     rainbowSpeed;
    float     animTime;

    std::vector<int64_t> segmentIds;
    bool                 isDrawn;
};

class GradientLineManager {
public:
    static GradientLineManager& getInstance();

    GradientLineManager(const GradientLineManager&)            = delete;
    GradientLineManager& operator=(const GradientLineManager&) = delete;

    int64_t create(float x1, float y1, float z1,
                   float x2, float y2, float z2,
                   int segments = 20);

    bool setGradient(int64_t id,
                     float r1, float g1, float b1,
                     float r2, float g2, float b2);

    bool setRainbow(int64_t id, float speed = 1.0f);

    bool setColor(int64_t id, float r, float g, float b, float a = 1.0f);

    bool setEndpoints(int64_t id,
                      float x1, float y1, float z1,
                      float x2, float y2, float z2);

    bool draw(int64_t id);
    bool drawToDimension(int64_t id, int dimId);

    bool remove(int64_t id);

    bool destroy(int64_t id);
    void destroyAll();

    void tick(float deltaTime);

private:
    GradientLineManager()  = default;
    ~GradientLineManager() = default;

    void rebuildSegments(GradientLine& line);
    void destroySegments(GradientLine& line);
    void updateSegmentColors(GradientLine& line);

    core::Registry<GradientLine> mLines;
};

} // namespace debugshape_export
