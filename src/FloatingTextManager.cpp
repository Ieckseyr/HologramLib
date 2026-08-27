#include "FloatingTextManager.h"
#include "PacketDebugRenderer.h"

#include "lse/LseBridge.h"

#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace debugshape_export {
// Color4f 实现
Color4f Color4f::fromHSV(float h, float s, float v, float a) {
    // h: 0-360, s: 0-1, v: 0-1
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    return Color4f(r + m, g + m, b + m, a);
}
// FloatingTextManager 单例
FloatingTextManager& FloatingTextManager::getInstance() {
    static FloatingTextManager instance;
    return instance;
}

FloatingTextManager::FloatingTextManager() {
    registerBuiltinVariables();
}
// 创建与销毁
int64_t FloatingTextManager::create(float x, float y, float z) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto ft = std::make_unique<FloatingText>();
    ft->id = mNextId++;
    ft->x = x;
    ft->y = y;
    ft->z = z;

    int64_t id = ft->id;
    mFloatingTexts[id] = std::move(ft);
    return id;
}

bool FloatingTextManager::destroy(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mFloatingTexts.find(id);
    if (it == mFloatingTexts.end()) return false;

    destroyTextShape(*it->second);
    mFloatingTexts.erase(it);
    return true;
}

void FloatingTextManager::destroyAll() {
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& [id, ft] : mFloatingTexts) {
        destroyTextShape(*ft);
    }
    mFloatingTexts.clear();
}
// 行管理
// 文本更新后自动 rebuild + redraw (同 networkId 客户端原地覆盖, 无闪烁)
bool FloatingTextManager::addLine(int64_t id, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    LineConfig line;
    line.text = text;
    ft->lines.push_back(line);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineText(int64_t id, int lineIndex, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].text = text;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineScale(int64_t id, int lineIndex, float scale) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].scale = std::max(0.5f, std::min(scale, 3.0f));

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::removeLine(int64_t id, int lineIndex) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines.erase(ft->lines.begin() + lineIndex);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::clearLines(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->lines.clear();
    destroyTextShape(*ft);
    return true;
}

int FloatingTextManager::getLineCount(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    return ft ? (int)ft->lines.size() : 0;
}
// 颜色设置
// 单形状方案: 整块文本一个背景框一种底色, 行级颜色差异由 § 颜色代码承担
bool FloatingTextManager::setColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    for (auto& line : ft->lines) {
        line.colorMode = ColorMode::Solid;
        line.solidColor = Color4f(r, g, b, a);
    }

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineColor(int64_t id, int lineIndex, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].colorMode = ColorMode::Solid;
    ft->lines[lineIndex].solidColor = Color4f(r, g, b, a);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineGradient(int64_t id, int lineIndex,
                                          float r1, float g1, float b1,
                                          float r2, float g2, float b2) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].colorMode = ColorMode::Gradient;
    ft->lines[lineIndex].gradientStart = Color4f(r1, g1, b1, 1.0f);
    ft->lines[lineIndex].gradientEnd = Color4f(r2, g2, b2, 1.0f);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineRainbow(int64_t id, int lineIndex, float speed) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].colorMode = ColorMode::Rainbow;
    ft->lines[lineIndex].rainbowSpeed = speed;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}
// 动画设置
bool FloatingTextManager::setLineScroll(int64_t id, int lineIndex, int direction, float speed) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].scroll = static_cast<ScrollDirection>(direction);
    ft->lines[lineIndex].scrollSpeed = speed;

    // 确保滚动偏移数组大小正确
    if (ft->scrollOffsets.size() <= (size_t)lineIndex) {
        ft->scrollOffsets.resize(lineIndex + 1, 0.0f);
    }

    return true;
}

bool FloatingTextManager::setVerticalAnimation(int64_t id, int type, float speed, float range) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->vertAnim = static_cast<VerticalAnimation>(type);
    ft->vertAnimSpeed = speed;
    ft->vertAnimRange = range;
    return true;
}

bool FloatingTextManager::setLineSpacing(int64_t id, float spacing) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    // 单形状多行文本的行距由客户端渲染器决定, 此字段仅保留 API 兼容
    ft->lineSpacing = spacing;
    return true;
}
// 位置与跟随
bool FloatingTextManager::setLocation(int64_t id, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->x = x;
    ft->y = y;
    ft->z = z;

    if (ft->isDrawn && ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setLocation(ft->textShapeId, x, y, z);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->dimId = dimId;

    // 已绘制: 同步底层形状维度后按原绘制目标原地重发（同 networkId 覆盖, 无闪烁）
    if (ft->isDrawn && ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setDimension(ft->textShapeId, dimId);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setFollowPlayer(int64_t id, const std::string& playerName, float offsetY) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->followPlayer = playerName;
    ft->followOffsetY = offsetY;
    return true;
}

bool FloatingTextManager::clearFollowPlayer(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->followPlayer.clear();
    return true;
}
// 显示控制
bool FloatingTextManager::draw(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->drawTarget = FloatingText::DrawTarget::All;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0) return false;

    bool ok = PacketDebugRenderer::getInstance().draw(ft->textShapeId);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::drawToDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->dimId = dimId;
    ft->drawTarget = FloatingText::DrawTarget::Dimension;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0) return false;

    bool ok = PacketDebugRenderer::getInstance().drawToDimension(ft->textShapeId, dimId);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::drawToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->drawTarget = FloatingText::DrawTarget::Player;
    ft->targetPlayer = playerName;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0) return false;

    bool ok = PacketDebugRenderer::getInstance().drawToPlayer(ft->textShapeId, playerName);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    if (ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().remove(ft->textShapeId); // 发移除包, 保留 shape 内存
    }

    ft->isDrawn = false;
    ft->drawTarget = FloatingText::DrawTarget::None;
    return true;
}

bool FloatingTextManager::refresh(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || !ft->isDrawn) return false;

    // 跟随模式: tick() 已把最新位置写入 ft->x/y/z,
    // 重发前先同步到底层形状, 否则形状永远停在创建时的位置
    if (!ft->followPlayer.empty() && ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setLocation(ft->textShapeId, ft->x, ft->y, ft->z);
    }

    // 复用 shape (networkId 不变), 客户端原地覆盖, 无闪烁
    rebuildTextShape(*ft);
    return redrawTextShape(*ft);
}
// 动画更新 (tick)
// 注意：由于 DebugShape API 的限制，频繁调用 draw() 会导致闪烁
// 因此 tick 只用于更新内部状态，不会自动重绘
// 如果需要看到动画效果，请手动调用 refresh()
void FloatingTextManager::tick(float deltaTime) {
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& [id, ft] : mFloatingTexts) {
        if (!ft->isDrawn) continue;

        ft->animTime += deltaTime;

        // 更新滚动偏移值（内部状态）
        for (size_t i = 0; i < ft->lines.size(); ++i) {
            if (ft->lines[i].scroll != ScrollDirection::None) {
                if (ft->scrollOffsets.size() <= i) {
                    ft->scrollOffsets.resize(i + 1, 0.0f);
                }
                float dir = (ft->lines[i].scroll == ScrollDirection::Left) ? -1.0f : 1.0f;
                ft->scrollOffsets[i] += dir * ft->lines[i].scrollSpeed * deltaTime;
            }
        }

        // 更新跟随玩家位置（内部状态）
        if (!ft->followPlayer.empty()) {
            auto level = ll::service::getLevel();
            if (level.has_value()) {
                auto* player = level->getPlayer(ft->followPlayer);
                if (player) {
                    auto pos = player->getPosition();
                    ft->x = pos.x;
                    ft->y = pos.y + ft->followOffsetY;
                    ft->z = pos.z;
                }
            }
        }

        // 注意：不在这里调用重绘
        // 因为那会导致闪烁。用户需要手动调用 refresh() 来更新显示
    }
}
// 动态变量
void FloatingTextManager::registerVariable(const std::string& name, VariableProvider provider) {
    mVariables[name] = provider;
}

void FloatingTextManager::registerBuiltinVariables() {
    // {time} - 当前时间
    registerVariable("time", [](const std::string&) -> std::string {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    });

    // {online} - 在线人数
    registerVariable("online", [](const std::string&) -> std::string {
        auto level = ll::service::getLevel();
        if (level.has_value()) {
            return std::to_string(level->getActivePlayerCount());
        }
        return "0";
    });

    // {player} - 玩家名 (需要上下文)
    registerVariable("player", [](const std::string& playerName) -> std::string {
        return playerName.empty() ? "Unknown" : playerName;
    });

    // {tps} - TPS (简化实现)
    registerVariable("tps", [](const std::string&) -> std::string {
        return "20.0"; // TODO: 实际TPS计算
    });
}

std::string FloatingTextManager::processVariables(const std::string& text, const std::string& playerContext) {
    if (text.empty()) return text;

    // 1. 经 LseBridge 翻译 %name% 和 {name} 占位符
    //    （运行时可选: lrca + MeowPAPI/MeowSidebar 在场时生效）
    //    未注册的占位符会被原样保留，留给下一步内置变量兜底
    std::string result = playerContext.empty()
        ? hologramlib::lse::translateString(text)
        : hologramlib::lse::translateStringWithPlayer(text, playerContext);

    // 2. 兜底：内置变量替换（{time}/{online}/{player}/{tps}）
    //    处理 MeowPAPI 不可用或未注册这些占位符的情况
    for (const auto& [name, provider] : mVariables) {
        std::string placeholder = "{" + name + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            std::string value = provider(playerContext);
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}
// 内部辅助方法
FloatingText* FloatingTextManager::getFloatingText(int64_t id) {
    auto it = mFloatingTexts.find(id);
    return (it != mFloatingTexts.end()) ? it->second.get() : nullptr;
}

// 销毁文本形状 (发送移除包 + 删除内存)
void FloatingTextManager::destroyTextShape(FloatingText& ft) {
    if (ft.textShapeId < 0) return;
    // PacketDebugRenderer::destroy 内部会先发移除包再删内存
    PacketDebugRenderer::getInstance().destroy(ft.textShapeId);
    ft.textShapeId = -1;
}

// 重建单一多行文本形状
//
// 所有行用 '\n' 合并为一个 Text shape —— 客户端原生渲染多行文本,
// 整块文字共用同一个背景框 (修复"一个字一个框")。
//
// 复用已有 shape (networkId 不变) 时, 客户端收到同 id 形状直接覆盖,
// 实现"原子替换" —— 倒计时等高频文本更新无闪烁。
void FloatingTextManager::rebuildTextShape(FloatingText& ft) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();

    if (ft.lines.empty()) {
        destroyTextShape(ft);
        return;
    }

    // 合并所有行 (\n), 逐行解析动态变量
    std::string combined;
    combined.reserve(ft.lines.size() * 16);
    for (size_t i = 0; i < ft.lines.size(); ++i) {
        if (i > 0) combined += '\n';
        combined += processVariables(ft.lines[i].text, ft.followPlayer);
    }

    // 首行样式作为整块基础样式 (行级颜色差异由文本内 § 颜色代码承担)
    const LineConfig& first = ft.lines.front();
    Color4f color{1.0f, 1.0f, 1.0f, 1.0f};
    if (first.colorMode == ColorMode::Solid) {
        color = first.solidColor;
    } else if (first.colorMode == ColorMode::Gradient) {
        color = first.gradientStart;
    }

    if (ft.textShapeId >= 0 && shapeMgr.exists(ft.textShapeId)) {
        // 原地更新 (保留 networkId → 客户端覆盖式刷新)
        shapeMgr.setText(ft.textShapeId, combined);
        shapeMgr.setScale(ft.textShapeId, first.scale);
        shapeMgr.setColor(ft.textShapeId, color.r, color.g, color.b, color.a);
    } else {
        // 首次创建
        ft.textShapeId = shapeMgr.createText(ft.x, ft.y, ft.z, combined);
        if (ft.textShapeId < 0) return;
        shapeMgr.setScale(ft.textShapeId, first.scale);
        shapeMgr.setColor(ft.textShapeId, color.r, color.g, color.b, color.a);
    }
    // 注: 不再设置有限 duration —— mTotalTimeLeft 会让客户端倒计时后自动删除形状,
    // 永久悬浮字必须保持 nullopt(由 destroy/remove 显式控制生命周期)
}

// 按绘制目标重发形状
bool FloatingTextManager::redrawTextShape(FloatingText& ft) {
    if (ft.textShapeId < 0) return false;
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    switch (ft.drawTarget) {
    case FloatingText::DrawTarget::All:
        return shapeMgr.draw(ft.textShapeId);
    case FloatingText::DrawTarget::Dimension:
        return shapeMgr.drawToDimension(ft.textShapeId, ft.dimId);
    case FloatingText::DrawTarget::Player:
        return shapeMgr.drawToPlayer(ft.textShapeId, ft.targetPlayer);
    default:
        return false;
    }
}

} // namespace debugshape_export
