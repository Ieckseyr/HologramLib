#include "FloatingTextManager.h"
#include "PacketDebugRenderer.h"
#include "kernel/EventIds.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/Listener.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace debugshape_export {

namespace {

std::uint64_t steadyMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

ll::event::ListenerPtr sDynamicTickListener;
ll::event::ListenerPtr sJoinListener;
} // namespace

FloatingTextManager& FloatingTextManager::getInstance() {
    static FloatingTextManager instance;
    return instance;
}

FloatingTextManager::FloatingTextManager() {
    registerBuiltinVariables();
}

int64_t FloatingTextManager::create(float x, float y, float z) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto ft = std::make_unique<FloatingText>();
    ft->x   = x;
    ft->y   = y;
    ft->z   = z;

    int64_t const id = mFloatingTexts.insert(std::move(ft));
    if (auto* inserted = mFloatingTexts.peek(id))
        inserted->id = id;
    return id;
}

bool FloatingTextManager::destroy(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = mFloatingTexts.peek(id);
    if (ft == nullptr)
        return false;

    destroyTextShape(*ft);
    mFloatingTexts.erase(id);
    return true;
}

void FloatingTextManager::destroyAll() {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (auto const id : mFloatingTexts.snapshotAll()) {
        if (auto* ft = mFloatingTexts.peek(id))
            destroyTextShape(*ft);
    }
    mFloatingTexts.clear();
}

bool FloatingTextManager::addLine(int64_t id, const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->lines.push_back(FloatingTextLine{text, {}, 0, true});

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineText(int64_t id, int lineIndex, const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size())
        return false;

    ft->lines[lineIndex].text = text;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::removeLine(int64_t id, int lineIndex) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size())
        return false;

    ft->lines.erase(ft->lines.begin() + lineIndex);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::clearLines(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->lines.clear();
    destroyTextShape(*ft);
    return true;
}

int FloatingTextManager::getLineCount(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    return ft ? (int)ft->lines.size() : 0;
}

bool FloatingTextManager::setColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->color = Color4f(r, g, b, a);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setScale(int64_t id, float scale) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;
    if (!(scale > 0.0f))
        return false;

    ft->scale = scale;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setBackgroundColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->backgroundColor = Color4f(r, g, b, a);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::clearBackgroundColor(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->backgroundColor.reset();

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setDepthTest(int64_t id, bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->depthTest = enabled;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setRotation(int64_t id, float pitch, float yaw, float roll) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->useRotation = true;
    ft->rotPitch    = pitch;
    ft->rotYaw      = yaw;
    ft->rotRoll     = roll;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::clearRotation(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->useRotation = false;
    ft->rotPitch = ft->rotYaw = ft->rotRoll = 0.0f;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLinePool(int64_t id, int lineIndex, const std::vector<std::string>& content, int intervalMs) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size())
        return false;

    ft->lines[lineIndex].pool           = content;
    ft->lines[lineIndex].poolIntervalMs = intervalMs;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineParseVariables(int64_t id, int lineIndex, bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size())
        return false;

    ft->lines[lineIndex].parseVariables = enabled;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLocation(int64_t id, float x, float y, float z) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

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
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->dimId = dimId;

    if (ft->isDrawn && ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setDimension(ft->textShapeId, dimId);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setFollowPlayer(int64_t id, const std::string& playerName, float offsetY) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->followPlayer  = playerName;
    ft->followOffsetY = offsetY;

    resolveFollowPosition(*ft);
    return true;
}

bool FloatingTextManager::clearFollowPlayer(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    ft->followPlayer.clear();
    return true;
}

bool FloatingTextManager::draw(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    resolveFollowPosition(*ft);
    ft->drawTarget = FloatingText::DrawTarget::All;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0)
        return false;

    bool ok     = PacketDebugRenderer::getInstance().draw(ft->textShapeId);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::drawToDimension(int64_t id, int dimId) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    resolveFollowPosition(*ft);
    ft->dimId      = dimId;
    ft->drawTarget = FloatingText::DrawTarget::Dimension;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0)
        return false;

    bool ok     = PacketDebugRenderer::getInstance().drawToDimension(ft->textShapeId, dimId);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::drawToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    resolveFollowPosition(*ft);
    ft->drawTarget   = FloatingText::DrawTarget::Player;
    ft->targetPlayer = playerName;
    rebuildTextShape(*ft);
    if (ft->textShapeId < 0)
        return false;

    bool ok     = PacketDebugRenderer::getInstance().drawToPlayer(ft->textShapeId, playerName);
    ft->isDrawn = true;
    return ok;
}

bool FloatingTextManager::remove(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft)
        return false;

    if (ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().remove(ft->textShapeId);
    }

    ft->isDrawn    = false;
    ft->drawTarget = FloatingText::DrawTarget::None;
    return true;
}

bool FloatingTextManager::refresh(int64_t id) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || !ft->isDrawn)
        return false;

    resolveFollowPosition(*ft);
    if (ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setLocation(ft->textShapeId, ft->x, ft->y, ft->z);
    }

    rebuildTextShape(*ft);
    return redrawTextShape(*ft);
}

void FloatingTextManager::sampleTps() {
    auto const now  = steadyMs();
    auto const last = mLastTickMs.exchange(now, std::memory_order_relaxed);
    if (last == 0 || now <= last)
        return;
    auto const deltaMs = now - last;
    if (deltaMs > 1000)
        return;
    double inst = 1000.0 / static_cast<double>(deltaMs);
    if (inst > 20.0)
        inst = 20.0;
    double const prev = mTps.load(std::memory_order_relaxed);
    mTps.store(prev * 0.9 + inst * 0.1, std::memory_order_relaxed);
}

void FloatingTextManager::tickDynamic() {
    sampleTps();
    if (++mDynamicTickCounter % 10 != 0)
        return;

    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto const                            ts = steadyMs();

    for (auto const id : mFloatingTexts.snapshotAll()) {
        auto* ftPtr = mFloatingTexts.peek(id);
        if (ftPtr == nullptr)
            continue;
        auto& ft = *ftPtr;
        if (!ft.isDrawn || !hasDynamicLines(ft))
            continue;

        if (textHasVariables(ft) && ft.drawTarget != FloatingText::DrawTarget::Player) {

            auto level = ::ll::service::getLevel();
            if (!level.has_value())
                continue;
            bool        changed  = false;
            std::size_t eligible = 0;
            level->forEachPlayer([&](Player& p) -> bool {
                std::string const name = p.getRealName();
                if (ft.drawTarget == FloatingText::DrawTarget::Dimension && static_cast<int>(p.getDimensionId()) != ft.dimId) {
                    return true;
                }
                ++eligible;
                auto const it = ft.viewerShapes.find(name);
                if (it == ft.viewerShapes.end()) {
                    changed = true;
                    return false;
                }
                auto const last = mLastSentText.find(it->second);
                if (last == mLastSentText.end() || last->second != composeText(ft, name, ts)) {
                    changed = true;
                    return false;
                }
                return true;
            });
            if (!changed && eligible != ft.viewerShapes.size())
                changed = true;
            if (changed)
                rebuildTextShapesPerViewer(ft);
            continue;
        }

        std::string const context =
            (ft.drawTarget == FloatingText::DrawTarget::Player) ? ft.targetPlayer : ft.followPlayer;
        std::string const combined = composeText(ft, context, ts);
        auto const        it       = mLastSentText.find(ft.textShapeId);
        if (it != mLastSentText.end() && it->second == combined)
            continue;

        rebuildTextShape(ft);
        redrawTextShape(ft);
    }
}

void FloatingTextManager::onPlayerJoin(Player& player) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto const                            name = player.getRealName();
    auto const                            dim  = static_cast<int>(player.getDimensionId());

    for (auto const id : mFloatingTexts.snapshotAll()) {
        auto* ft = mFloatingTexts.peek(id);
        if (ft == nullptr || !ft->isDrawn)
            continue;
        if (ft->drawTarget == FloatingText::DrawTarget::Player && ft->targetPlayer != name)
            continue;
        if (ft->drawTarget == FloatingText::DrawTarget::Dimension && ft->dimId != dim)
            continue;
        // 静态文本也要在人进服时补一遍：整表按当前在线重建，新进来的自然被包含
        rebuildTextShapesPerViewer(*ft);
    }
}

void FloatingTextManager::initDynamicDriver() {
    if (!sJoinListener) {
        sJoinListener = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                    [](ll::event::PlayerJoinEvent& ev) {
                                                                                        FloatingTextManager::getInstance().onPlayerJoin(ev.self());
                                                                                    });
    }
    if (sDynamicTickListener)
        return;
    sDynamicTickListener = debugshape_export::core::listen<ll::event::ServerLevelTickEvent>(core::kServerLevelTickId,
                                                                                            [](ll::event::ServerLevelTickEvent const&) {
                                                                                                FloatingTextManager::getInstance().tickDynamic();
                                                                                            });
}

void FloatingTextManager::shutdownDynamicDriver() {
    if (sJoinListener) {
        ll::event::EventBus::getInstance().removeListener(sJoinListener);
        sJoinListener = nullptr;
    }
    if (!sDynamicTickListener)
        return;
    ll::event::EventBus::getInstance().removeListener(sDynamicTickListener);
    sDynamicTickListener = nullptr;
}

void FloatingTextManager::registerVariable(const std::string& name, VariableProvider provider) {
    mVariables[name] = provider;
}

void FloatingTextManager::registerBuiltinVariables() {

    registerVariable("time", [](const std::string&) -> std::string {
        auto    now  = std::chrono::system_clock::now();
        auto    time = std::chrono::system_clock::to_time_t(now);
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

    registerVariable("online", [](const std::string&) -> std::string {
        auto level = ll::service::getLevel();
        if (level.has_value()) {
            return std::to_string(level->getActivePlayerCount());
        }
        return "0";
    });

    registerVariable("player", [](const std::string& playerName) -> std::string {
        return playerName.empty() ? "Unknown" : playerName;
    });

    registerVariable("tps", [this](const std::string&) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << mTps.load(std::memory_order_relaxed);
        return oss.str();
    });

    registerVariable("dimension", [](const std::string& playerName) -> std::string {
        if (playerName.empty())
            return "?";
        auto level = ll::service::getLevel();
        if (!level.has_value())
            return "?";
        auto* p = level->getPlayer(playerName);
        return p ? std::to_string(static_cast<int>(p->getDimensionId())) : "?";
    });
    for (auto const& axis : {"x", "y", "z"}) {
        std::string const name(axis);
        registerVariable(name, [name](const std::string& playerName) -> std::string {
            if (playerName.empty())
                return "?";
            auto level = ll::service::getLevel();
            if (!level.has_value())
                return "?";
            auto* p = level->getPlayer(playerName);
            if (!p)
                return "?";
            auto const   pos = p->getPosition();
            double const v   = name == "x" ? pos.x : name == "y" ? pos.y
                                                                 : pos.z;
            return std::to_string(static_cast<int>(std::floor(v)));
        });
    }
}

bool FloatingTextManager::textHasVariables(FloatingText const& ft) {
    auto const hasBrace = [](std::string const& s) {
        return s.find('{') != std::string::npos;
    };
    for (auto const& line : ft.lines) {
        if (!line.parseVariables)
            continue;
        if (hasBrace(line.text))
            return true;
        for (auto const& item : line.pool) {
            if (hasBrace(item))
                return true;
        }
    }
    return false;
}

bool FloatingTextManager::hasDynamicLines(FloatingText const& ft) {
    for (auto const& line : ft.lines) {
        if (line.pool.size() > 1 && line.poolIntervalMs > 0)
            return true;
        if (line.parseVariables && line.text.find('{') != std::string::npos)
            return true;
    }
    return false;
}

// 【重构来源标注 · LGPL-3.0】动态行轮播（内容池按间隔时间取模 + 行级变量开关）重构自 Phantom
// （https://github.com/GroupMountain/Phantom，LGPL-3.0，版权归 Phantom 贡献者）。详见 FloatingTextManager.h 文件头。
std::string FloatingTextManager::composeLine(FloatingTextLine const& line, const std::string& playerContext, std::uint64_t nowMs) {
    std::string base;
    if (line.pool.empty()) {
        base = line.text;
    } else if (line.pool.size() > 1 && line.poolIntervalMs > 0) {
        auto const idx = static_cast<std::size_t>(
            (nowMs / static_cast<std::uint64_t>(line.poolIntervalMs)) % line.pool.size());
        base = line.pool[idx];
    } else {
        base = line.pool.front();
    }
    return line.parseVariables ? processVariables(base, playerContext) : base;
}

std::string FloatingTextManager::composeText(FloatingText const& ft, const std::string& playerContext, std::uint64_t nowMs) {
    std::string combined;
    combined.reserve(ft.lines.size() * 16);
    for (std::size_t i = 0; i < ft.lines.size(); ++i) {
        if (i > 0)
            combined += '\n';
        combined += composeLine(ft.lines[i], playerContext, nowMs);
    }
    return combined;
}

void FloatingTextManager::resolveFollowPosition(FloatingText& ft) {
    if (ft.followPlayer.empty())
        return;
    auto level = ll::service::getLevel();
    if (!level.has_value())
        return;
    auto* player = level->getPlayer(ft.followPlayer);
    if (!player)
        return;
    auto pos = player->getPosition();
    ft.x     = pos.x;
    ft.y     = pos.y + ft.followOffsetY;
    ft.z     = pos.z;
}

void FloatingTextManager::applyBlockStyle(int64_t shapeId, FloatingText const& ft) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    shapeMgr.setScale(shapeId, ft.scale);
    shapeMgr.setColor(shapeId, ft.color.r, ft.color.g, ft.color.b, ft.color.a);
    if (ft.backgroundColor) {
        shapeMgr.setBackgroundColor(
            shapeId,
            ft.backgroundColor->r,
            ft.backgroundColor->g,
            ft.backgroundColor->b,
            ft.backgroundColor->a);
    } else {
        shapeMgr.clearBackgroundColor(shapeId);
    }
    shapeMgr.setDepthTest(shapeId, ft.depthTest);
    if (ft.useRotation) {
        shapeMgr.setRotation(shapeId, ft.rotPitch, ft.rotYaw, ft.rotRoll);
    } else {
        shapeMgr.clearRotation(shapeId);
    }
}

void FloatingTextManager::destroyViewerShapes(FloatingText& ft) {
    if (ft.viewerShapes.empty())
        return;
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (auto const& [name, sid] : ft.viewerShapes) {
        if (sid >= 0 && shapeMgr.exists(sid))
            shapeMgr.remove(sid);
        mLastSentText.erase(sid);
    }
    ft.viewerShapes.clear();
}

void FloatingTextManager::rebuildTextShapesPerViewer(FloatingText& ft) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    auto  level    = ll::service::getLevel();
    if (!level)
        return;

    if (ft.textShapeId >= 0 && shapeMgr.exists(ft.textShapeId)) {
        shapeMgr.remove(ft.textShapeId);
    }
    ft.textShapeId = -1;

    std::unordered_set<std::string> online;
    level->forEachPlayer([&](Player& p) -> bool {
        std::string const name = p.getRealName();
        if (ft.drawTarget == FloatingText::DrawTarget::Dimension && static_cast<int>(p.getDimensionId()) != ft.dimId) {
            auto it = ft.viewerShapes.find(name);
            if (it != ft.viewerShapes.end()) {
                if (shapeMgr.exists(it->second))
                    shapeMgr.remove(it->second);
                ft.viewerShapes.erase(it);
            }
            return true;
        }
        online.insert(name);
        return true;
    });
    for (auto it = ft.viewerShapes.begin(); it != ft.viewerShapes.end();) {
        if (!online.contains(it->first)) {
            if (shapeMgr.exists(it->second))
                shapeMgr.remove(it->second);
            it = ft.viewerShapes.erase(it);
        } else {
            ++it;
        }
    }

    auto const ts = steadyMs();
    for (auto const& name : online) {
        std::string combined;
        combined.reserve(ft.lines.size() * 16);
        for (size_t i = 0; i < ft.lines.size(); ++i) {
            if (i > 0)
                combined += '\n';
            combined += composeLine(ft.lines[i], name, ts);
        }

        int64_t& sid = ft.viewerShapes[name];
        if (sid >= 0 && shapeMgr.exists(sid)) {
            shapeMgr.setText(sid, combined);
            shapeMgr.setLocation(sid, ft.x, ft.y, ft.z);
            applyBlockStyle(sid, ft);

            shapeMgr.drawToPlayer(sid, name);
            mLastSentText[sid] = combined;
        } else {
            sid = shapeMgr.createText(ft.x, ft.y, ft.z, combined);
            if (sid < 0) {
                ft.viewerShapes.erase(name);
                continue;
            }
            applyBlockStyle(sid, ft);
            shapeMgr.drawToPlayer(sid, name);
            mLastSentText[sid] = combined;
        }
    }
    ft.isDrawn = true;
}

std::string FloatingTextManager::processVariables(const std::string& text, const std::string& playerContext) {
    if (text.empty())
        return text;

    std::string result = text;

    for (const auto& [name, provider] : mVariables) {
        std::string placeholder = "{" + name + "}";
        size_t      pos         = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            std::string value = provider(playerContext);
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}

FloatingText* FloatingTextManager::getFloatingText(int64_t id) {
    return mFloatingTexts.peek(id);
}

void FloatingTextManager::destroyTextShape(FloatingText& ft) {
    destroyViewerShapes(ft);
    if (ft.textShapeId < 0)
        return;
    mLastSentText.erase(ft.textShapeId);

    PacketDebugRenderer::getInstance().destroy(ft.textShapeId);
    ft.textShapeId = -1;
}

void FloatingTextManager::rebuildTextShape(FloatingText& ft) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();

    if (ft.lines.empty()) {
        destroyTextShape(ft);
        return;
    }

    if (textHasVariables(ft) && ft.drawTarget != FloatingText::DrawTarget::Player) {
        rebuildTextShapesPerViewer(ft);
        return;
    }

    destroyViewerShapes(ft);

    std::string const context =
        (ft.drawTarget == FloatingText::DrawTarget::Player) ? ft.targetPlayer : ft.followPlayer;
    std::string const combined = composeText(ft, context, steadyMs());

    if (ft.textShapeId >= 0 && shapeMgr.exists(ft.textShapeId)) {

        shapeMgr.setText(ft.textShapeId, combined);
    } else {

        ft.textShapeId = shapeMgr.createText(ft.x, ft.y, ft.z, combined);
        if (ft.textShapeId < 0)
            return;
    }

    applyBlockStyle(ft.textShapeId, ft);
    mLastSentText[ft.textShapeId] = combined;
}

bool FloatingTextManager::redrawTextShape(FloatingText& ft) {
    if (ft.textShapeId < 0)
        return false;
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
