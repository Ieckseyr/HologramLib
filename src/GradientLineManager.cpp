#include "GradientLineManager.h"
#include "PacketDebugRenderer.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace debugshape_export {

LineColor LineColor::fromHSV(float h, float s, float v, float a) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h < 60) {
        r = c;
        g = x;
        b = 0;
    } else if (h < 120) {
        r = x;
        g = c;
        b = 0;
    } else if (h < 180) {
        r = 0;
        g = c;
        b = x;
    } else if (h < 240) {
        r = 0;
        g = x;
        b = c;
    } else if (h < 300) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }

    return LineColor(r + m, g + m, b + m, a);
}

GradientLineManager& GradientLineManager::getInstance() {
    static GradientLineManager instance;
    return instance;
}

int64_t GradientLineManager::create(float x1, float y1, float z1,
                                    float x2, float y2, float z2,
                                    int segments) {
    auto line          = std::make_unique<GradientLine>();
    line->x1           = x1;
    line->y1           = y1;
    line->z1           = z1;
    line->x2           = x2;
    line->y2           = y2;
    line->z2           = z2;
    line->segments     = (segments < 2) ? 2 : ((segments > 100) ? 100 : segments);
    line->startColor   = LineColor(1.0f, 0.0f, 0.0f);
    line->endColor     = LineColor(0.0f, 0.0f, 1.0f);
    line->isRainbow    = false;
    line->rainbowSpeed = 1.0f;
    line->animTime     = 0.0f;
    line->isDrawn      = false;

    int64_t const id = mLines.insert(std::move(line));
    mLines.withObject(id, [&](GradientLine& l) {
        l.id = id;
    });
    return id;
}

bool GradientLineManager::destroy(int64_t id) {

    if (!mLines.withObject(id, [&](GradientLine& line) {
            destroySegments(line);
        }))
        return false;
    mLines.erase(id);
    return true;
}

void GradientLineManager::destroyAll() {

    for (auto const id : mLines.snapshotAll()) {
        destroy(id);
    }
}

bool GradientLineManager::setGradient(int64_t id,
                                      float r1, float g1, float b1,
                                      float r2, float g2, float b2) {
    return mLines.withObject(id, [&](GradientLine& line) {
        line.startColor = LineColor(r1, g1, b1);
        line.endColor   = LineColor(r2, g2, b2);
        line.isRainbow  = false;

        if (line.isDrawn) {
            updateSegmentColors(line);
        }
    });
}

bool GradientLineManager::setRainbow(int64_t id, float speed) {
    return mLines.withObject(id, [&](GradientLine& line) {
        line.isRainbow    = true;
        line.rainbowSpeed = speed;

        if (line.isDrawn) {
            updateSegmentColors(line);
        }
    });
}

bool GradientLineManager::setColor(int64_t id, float r, float g, float b, float a) {
    return mLines.withObject(id, [&](GradientLine& line) {
        line.startColor = LineColor(r, g, b, a);
        line.endColor   = LineColor(r, g, b, a);
        line.isRainbow  = false;

        if (line.isDrawn) {
            updateSegmentColors(line);
        }
    });
}

bool GradientLineManager::setEndpoints(int64_t id,
                                       float x1, float y1, float z1,
                                       float x2, float y2, float z2) {
    return mLines.withObject(id, [&](GradientLine& line) {
        line.x1 = x1;
        line.y1 = y1;
        line.z1 = z1;
        line.x2 = x2;
        line.y2 = y2;
        line.z2 = z2;

        if (line.isDrawn) {

            rebuildSegments(line);

            auto& shapeMgr = PacketDebugRenderer::getInstance();
            for (int64_t segId : line.segmentIds) {
                shapeMgr.draw(segId);
            }
        }
    });
}

bool GradientLineManager::draw(int64_t id) {
    return mLines.withObject(id, [&](GradientLine& line) {
        rebuildSegments(line);

        auto& shapeMgr = PacketDebugRenderer::getInstance();
        for (int64_t segId : line.segmentIds) {
            shapeMgr.draw(segId);
        }

        line.isDrawn = true;
    });
}

bool GradientLineManager::drawToDimension(int64_t id, int dimId) {
    return mLines.withObject(id, [&](GradientLine& line) {
        rebuildSegments(line);

        auto& shapeMgr = PacketDebugRenderer::getInstance();
        for (int64_t segId : line.segmentIds) {
            shapeMgr.drawToDimension(segId, dimId);
        }

        line.isDrawn = true;
    });
}

bool GradientLineManager::remove(int64_t id) {
    return mLines.withObject(id, [&](GradientLine& line) {
        auto& shapeMgr = PacketDebugRenderer::getInstance();
        for (int64_t segId : line.segmentIds) {
            shapeMgr.remove(segId);
        }

        line.isDrawn = false;
    });
}

void GradientLineManager::tick(float deltaTime) {
    mLines.forEachLocked([&](int64_t, GradientLine& line) {
        if (!line.isDrawn)
            return;

        if (line.isRainbow) {
            line.animTime += deltaTime;

            updateSegmentColors(line);
        }
    });
}

void GradientLineManager::destroySegments(GradientLine& line) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (int64_t segId : line.segmentIds) {
        shapeMgr.destroy(segId);
    }
    line.segmentIds.clear();
}

void GradientLineManager::rebuildSegments(GradientLine& line) {

    destroySegments(line);

    auto& shapeMgr = PacketDebugRenderer::getInstance();
    line.segmentIds.reserve(line.segments);

    float dx = line.x2 - line.x1;
    float dy = line.y2 - line.y1;
    float dz = line.z2 - line.z1;

    for (int i = 0; i < line.segments; ++i) {
        float t1 = (float)i / line.segments;
        float t2 = (float)(i + 1) / line.segments;

        float sx = line.x1 + dx * t1;
        float sy = line.y1 + dy * t1;
        float sz = line.z1 + dz * t1;
        float ex = line.x1 + dx * t2;
        float ey = line.y1 + dy * t2;
        float ez = line.z1 + dz * t2;

        int64_t segId = shapeMgr.createLine(sx, sy, sz, ex, ey, ez);
        if (segId < 0)
            continue;

        float     tMid = (t1 + t2) / 2.0f;
        LineColor color;

        if (line.isRainbow) {
            float hue = std::fmod(tMid * 360.0f + line.animTime * line.rainbowSpeed * 60.0f, 360.0f);
            color     = LineColor::fromHSV(hue, 1.0f, 1.0f);
        } else {
            color = LineColor::lerp(line.startColor, line.endColor, tMid);
        }

        shapeMgr.setColor(segId, color.r, color.g, color.b, color.a);
        line.segmentIds.push_back(segId);
    }
}

void GradientLineManager::updateSegmentColors(GradientLine& line) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();

    for (size_t i = 0; i < line.segmentIds.size(); ++i) {
        float     t = ((float)i + 0.5f) / line.segments;
        LineColor color;

        if (line.isRainbow) {
            float hue = std::fmod(t * 360.0f + line.animTime * line.rainbowSpeed * 60.0f, 360.0f);
            color     = LineColor::fromHSV(hue, 1.0f, 1.0f);
        } else {
            color = LineColor::lerp(line.startColor, line.endColor, t);
        }

        shapeMgr.setColor(line.segmentIds[i], color.r, color.g, color.b, color.a);
    }
}

} // namespace debugshape_export
