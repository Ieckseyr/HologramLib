#include "GradientLineManager.h"
#include "PacketDebugRenderer.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace debugshape_export {
// LineColor 实现
LineColor LineColor::fromHSV(float h, float s, float v, float a) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    if (h < 60)       { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    
    return LineColor(r + m, g + m, b + m, a);
}
// GradientLineManager 单例
GradientLineManager& GradientLineManager::getInstance() {
    static GradientLineManager instance;
    return instance;
}

GradientLine* GradientLineManager::getLine(int64_t id) {
    auto it = mLines.find(id);
    return (it != mLines.end()) ? it->second.get() : nullptr;
}
// 创建与销毁
int64_t GradientLineManager::create(float x1, float y1, float z1,
                                     float x2, float y2, float z2,
                                     int segments) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto line = std::make_unique<GradientLine>();
    line->id = mNextId++;
    line->x1 = x1; line->y1 = y1; line->z1 = z1;
    line->x2 = x2; line->y2 = y2; line->z2 = z2;
    line->segments = (segments < 2) ? 2 : ((segments > 100) ? 100 : segments);
    line->startColor = LineColor(1.0f, 0.0f, 0.0f);  // 默认红
    line->endColor = LineColor(0.0f, 0.0f, 1.0f);    // 默认蓝
    line->isRainbow = false;
    line->rainbowSpeed = 1.0f;
    line->animTime = 0.0f;
    line->isDrawn = false;
    
    int64_t id = line->id;
    mLines[id] = std::move(line);
    return id;
}

bool GradientLineManager::destroy(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mLines.find(id);
    if (it == mLines.end()) return false;
    
    destroySegments(*it->second);
    mLines.erase(it);
    return true;
}

void GradientLineManager::destroyAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    for (auto& [id, line] : mLines) {
        destroySegments(*line);
    }
    mLines.clear();
}
// 颜色设置
bool GradientLineManager::setGradient(int64_t id,
                                       float r1, float g1, float b1,
                                       float r2, float g2, float b2) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    line->startColor = LineColor(r1, g1, b1);
    line->endColor = LineColor(r2, g2, b2);
    line->isRainbow = false;
    
    if (line->isDrawn) {
        updateSegmentColors(*line);
    }
    return true;
}

bool GradientLineManager::setRainbow(int64_t id, float speed) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    line->isRainbow = true;
    line->rainbowSpeed = speed;
    
    if (line->isDrawn) {
        updateSegmentColors(*line);
    }
    return true;
}

bool GradientLineManager::setColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    line->startColor = LineColor(r, g, b, a);
    line->endColor = LineColor(r, g, b, a);
    line->isRainbow = false;
    
    if (line->isDrawn) {
        updateSegmentColors(*line);
    }
    return true;
}

bool GradientLineManager::setEndpoints(int64_t id,
                                        float x1, float y1, float z1,
                                        float x2, float y2, float z2) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    line->x1 = x1; line->y1 = y1; line->z1 = z1;
    line->x2 = x2; line->y2 = y2; line->z2 = z2;
    
    if (line->isDrawn) {
        // 需要重建所有线段
        rebuildSegments(*line);
        // 重新绘制
        auto& shapeMgr = PacketDebugRenderer::getInstance();
        for (int64_t segId : line->segmentIds) {
            shapeMgr.draw(segId);
        }
    }
    return true;
}
// 显示控制
bool GradientLineManager::draw(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    rebuildSegments(*line);
    
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (int64_t segId : line->segmentIds) {
        shapeMgr.draw(segId);
    }
    
    line->isDrawn = true;
    return true;
}

bool GradientLineManager::drawToDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    rebuildSegments(*line);
    
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (int64_t segId : line->segmentIds) {
        shapeMgr.drawToDimension(segId, dimId);
    }
    
    line->isDrawn = true;
    return true;
}

bool GradientLineManager::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto* line = getLine(id);
    if (!line) return false;
    
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (int64_t segId : line->segmentIds) {
        shapeMgr.remove(segId);
    }
    
    line->isDrawn = false;
    return true;
}
// 动画更新
// 注意：由于 DebugShape API 的限制，频繁调用 draw() 会导致闪烁
// tick 只更新内部状态，不会自动重绘
void GradientLineManager::tick(float deltaTime) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    for (auto& [id, line] : mLines) {
        if (!line->isDrawn) continue;
        
        // 只有彩虹模式需要更新时间
        if (line->isRainbow) {
            line->animTime += deltaTime;
            // 只更新颜色值，不调用 draw()
            updateSegmentColors(*line);
        }
    }
}
// 内部方法
void GradientLineManager::destroySegments(GradientLine& line) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (int64_t segId : line.segmentIds) {
        shapeMgr.destroy(segId);
    }
    line.segmentIds.clear();
}

void GradientLineManager::rebuildSegments(GradientLine& line) {
    // 先销毁旧的
    destroySegments(line);
    
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    line.segmentIds.reserve(line.segments);
    
    // 计算方向向量
    float dx = line.x2 - line.x1;
    float dy = line.y2 - line.y1;
    float dz = line.z2 - line.z1;
    
    // 创建 N 段线条
    for (int i = 0; i < line.segments; ++i) {
        float t1 = (float)i / line.segments;
        float t2 = (float)(i + 1) / line.segments;
        
        // 计算这段线条的起点和终点
        float sx = line.x1 + dx * t1;
        float sy = line.y1 + dy * t1;
        float sz = line.z1 + dz * t1;
        float ex = line.x1 + dx * t2;
        float ey = line.y1 + dy * t2;
        float ez = line.z1 + dz * t2;
        
        // 创建线段
        int64_t segId = shapeMgr.createLine(sx, sy, sz, ex, ey, ez);
        if (segId < 0) continue;
        
        // 设置持续时间为1小时，避免闪烁
        shapeMgr.setDuration(segId, 3600.0f);
        
        // 计算颜色
        float tMid = (t1 + t2) / 2.0f;  // 使用中点颜色
        LineColor color;
        
        if (line.isRainbow) {
            float hue = std::fmod(tMid * 360.0f + line.animTime * line.rainbowSpeed * 60.0f, 360.0f);
            color = LineColor::fromHSV(hue, 1.0f, 1.0f);
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
        float t = ((float)i + 0.5f) / line.segments;  // 中点
        LineColor color;
        
        if (line.isRainbow) {
            float hue = std::fmod(t * 360.0f + line.animTime * line.rainbowSpeed * 60.0f, 360.0f);
            color = LineColor::fromHSV(hue, 1.0f, 1.0f);
        } else {
            color = LineColor::lerp(line.startColor, line.endColor, t);
        }
        
        shapeMgr.setColor(line.segmentIds[i], color.r, color.g, color.b, color.a);
        // 不调用 draw()，避免闪烁
    }
}

} // namespace debugshape_export
