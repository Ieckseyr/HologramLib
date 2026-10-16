#include "FloatingTextManager.h"
#include "PacketDebugRenderer.h"

#include "lse/LseBridge.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/Listener.h>
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
// 单调毫秒时钟（内容池轮播"时间取模"用; 多个浮字同一时间源 → 同池同相）
// 动态行部分（轮播/组装/节流驱动）重构自 Phantom (LGPL-3.0) —— 见 FloatingTextManager.h 文件头标注
std::uint64_t steadyMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
// 动态行驱动监听（单例管理器只有一份; 幂等挂/摘）
ll::event::ListenerPtr sDynamicTickListener;
} // namespace

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
// 行管理（行只负责文本; 行级样式差异请在文本内嵌 § 颜色代码）
// 文本更新后自动 rebuild + redraw (同 networkId 客户端原地覆盖, 无闪烁)
bool FloatingTextManager::addLine(int64_t id, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->lines.push_back(FloatingTextLine{text, {}, 0, true});

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
// 整块样式（单形状方案: 整块文本一种颜色 / 一份缩放 / 一个背景框）
bool FloatingTextManager::setColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->color = Color4f(r, g, b, a);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setScale(int64_t id, float scale) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;
    if (!(scale > 0.0f)) return false; // 非正缩放无意义（客户端会画出退化的文本）

    ft->scale = scale;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setBackgroundColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->backgroundColor = Color4f(r, g, b, a);

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::clearBackgroundColor(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->backgroundColor.reset(); // 回客户端默认背景色

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setDepthTest(int64_t id, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->depthTest = enabled;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setRotation(int64_t id, float pitch, float yaw, float roll) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->useRotation = true; // 设置即固定朝向（不再跟随相机）
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
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft) return false;

    ft->useRotation = false; // 恢复面向相机（billboard）
    ft->rotPitch = ft->rotYaw = ft->rotRoll = 0.0f;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}
// ── 动态行（内容池轮播 + 行级变量开关; 重构自 Phantom, LGPL-3.0 —— 见文件头标注）──

bool FloatingTextManager::setLinePool(int64_t id, int lineIndex, const std::vector<std::string>& content, int intervalMs) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].pool           = content;
    ft->lines[lineIndex].poolIntervalMs = intervalMs;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
    return true;
}

bool FloatingTextManager::setLineParseVariables(int64_t id, int lineIndex, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto* ft = getFloatingText(id);
    if (!ft || lineIndex < 0 || lineIndex >= (int)ft->lines.size()) return false;

    ft->lines[lineIndex].parseVariables = enabled;

    if (ft->isDrawn) {
        rebuildTextShape(*ft);
        redrawTextShape(*ft);
    }
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
    // 立刻对齐一次位置（无库内自驱 tick; 之后由调用方按需 refresh 推进）
    resolveFollowPosition(*ft);
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

    resolveFollowPosition(*ft); // 跟随: 绘制时就地取一次目标位置
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

    resolveFollowPosition(*ft);
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

    resolveFollowPosition(*ft);
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

    // 跟随模式: 就地解析目标玩家位置（无库内自驱 tick, 每次调用取最新值）
    resolveFollowPosition(*ft);
    if (ft->textShapeId >= 0) {
        PacketDebugRenderer::getInstance().setLocation(ft->textShapeId, ft->x, ft->y, ft->z);
    }

    // 复用 shape (networkId 不变), 客户端原地覆盖, 无闪烁
    rebuildTextShape(*ft);
    return redrawTextShape(*ft);
}

// ── 动态行驱动（重构自 Phantom 的 tick 刷新法; LGPL-3.0, 见文件头标注）──
// ServerLevelTickEvent 每 tick 调; 内部 10 tick(0.5s) 节流; 只碰"含动态行"的浮字;
// 组装当前文本与上次下发比对, **变了才重发**（不做无消费的空转）。
void FloatingTextManager::tickDynamic() {
    if (++mDynamicTickCounter % 10 != 0) return; // 0.5s 一拍（与 Phantom 的默认 10 tick 一致）

    std::lock_guard<std::mutex> lock(mMutex);
    auto const                  ts = steadyMs();

    for (auto& [id, ftPtr] : mFloatingTexts) {
        auto& ft = *ftPtr;
        if (!ft.isDrawn || !hasDynamicLines(ft)) continue;

        if (textHasVariables(ft) && ft.drawTarget != FloatingText::DrawTarget::Player) {
            // 逐观看者模式: 重建会顺带对齐观看者集合（有人新进/换维度）并按人重发;
            // 只在"文本或观看者集合发生变化"时才做（逐人比对上次下发）
            auto level = ::ll::service::getLevel();
            if (!level.has_value()) continue;
            bool        changed  = false;
            std::size_t eligible = 0;
            level->forEachPlayer([&](Player& p) -> bool {
                std::string const name = p.getRealName();
                if (ft.drawTarget == FloatingText::DrawTarget::Dimension
                    && static_cast<int>(p.getDimensionId()) != ft.dimId) {
                    return true; // 不在目标维度: 不参与
                }
                ++eligible;
                auto const it = ft.viewerShapes.find(name);
                if (it == ft.viewerShapes.end()) {
                    changed = true; // 新观看者(刚进服/刚进维度): 需要补形状
                    return false;
                }
                auto const last = mLastSentText.find(it->second);
                if (last == mLastSentText.end() || last->second != composeText(ft, name, ts)) {
                    changed = true; // 文本变了
                    return false;
                }
                return true;
            });
            if (!changed && eligible != ft.viewerShapes.size()) changed = true; // 有观看者离开
            if (changed) rebuildTextShapesPerViewer(ft);
            continue;
        }

        // 共享形状: 组装整块文本, 与上次下发比对 —— 变了才重建+重发
        std::string const context =
            (ft.drawTarget == FloatingText::DrawTarget::Player) ? ft.targetPlayer : ft.followPlayer;
        std::string const combined = composeText(ft, context, ts);
        auto const        it       = mLastSentText.find(ft.textShapeId);
        if (it != mLastSentText.end() && it->second == combined) continue;

        rebuildTextShape(ft);
        redrawTextShape(ft);
    }
}

// 挂/摘 ServerLevelTickEvent 监听（幂等; 由 ModEntry 的 enable/disable 调用）
void FloatingTextManager::initDynamicDriver() {
    if (sDynamicTickListener) return;
    sDynamicTickListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ServerLevelTickEvent>(
        [](ll::event::ServerLevelTickEvent const&) { FloatingTextManager::getInstance().tickDynamic(); }
    );
}

void FloatingTextManager::shutdownDynamicDriver() {
    if (!sDynamicTickListener) return;
    ll::event::EventBus::getInstance().removeListener(sDynamicTickListener);
    sDynamicTickListener = nullptr;
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

    // {dimension} / {x} / {y} / {z} - 观看者（或跟随目标）的维度与脚位坐标（重构自 Phantom 的变量集, LGPL-3.0）
    registerVariable("dimension", [](const std::string& playerName) -> std::string {
        if (playerName.empty()) return "?";
        auto level = ll::service::getLevel();
        if (!level.has_value()) return "?";
        auto* p = level->getPlayer(playerName);
        return p ? std::to_string(static_cast<int>(p->getDimensionId())) : "?";
    });
    for (auto const& axis : {"x", "y", "z"}) {
        std::string const name(axis);
        registerVariable(name, [name](const std::string& playerName) -> std::string {
            if (playerName.empty()) return "?";
            auto level = ll::service::getLevel();
            if (!level.has_value()) return "?";
            auto* p = level->getPlayer(playerName);
            if (!p) return "?";
            auto const  pos = p->getPosition();
            double const v = name == "x" ? pos.x : name == "y" ? pos.y : pos.z;
            return std::to_string(static_cast<int>(std::floor(v)));
        });
    }
}

bool FloatingTextManager::textHasVariables(FloatingText const& ft) {
    auto const hasBrace = [](std::string const& s) { return s.find('{') != std::string::npos; };
    for (auto const& line : ft.lines) {
        if (!line.parseVariables) continue; // 关了解析 = { } 是字面量, 不触发逐观看者
        if (hasBrace(line.text)) return true;
        for (auto const& item : line.pool) {
            if (hasBrace(item)) return true;
        }
    }
    return false;
}

// 文本是否含"动态行"（有轮播池 / 或开启解析且含 { ）—— 库内驱动只处理这些
bool FloatingTextManager::hasDynamicLines(FloatingText const& ft) {
    for (auto const& line : ft.lines) {
        if (line.pool.size() > 1 && line.poolIntervalMs > 0) return true;
        if (line.parseVariables && line.text.find('{') != std::string::npos) return true;
    }
    return false;
}

// 组装一行: 内容池（时间取模, 无状态; 多浮字同池同相）→ 变量解析
std::string FloatingTextManager::composeLine(FloatingTextLine const& line, const std::string& playerContext, std::uint64_t nowMs) {
    std::string base;
    if (line.pool.empty()) {
        base = line.text;
    } else if (line.pool.size() > 1 && line.poolIntervalMs > 0) {
        auto const idx = static_cast<std::size_t>(
            (nowMs / static_cast<std::uint64_t>(line.poolIntervalMs)) % line.pool.size()
        );
        base = line.pool[idx];
    } else {
        base = line.pool.front();
    }
    return line.parseVariables ? processVariables(base, playerContext) : base;
}

// 组装整块共享文本（\n 合并所有行）
std::string FloatingTextManager::composeText(FloatingText const& ft, const std::string& playerContext, std::uint64_t nowMs) {
    std::string combined;
    combined.reserve(ft.lines.size() * 16);
    for (std::size_t i = 0; i < ft.lines.size(); ++i) {
        if (i > 0) combined += '\n';
        combined += composeLine(ft.lines[i], playerContext, nowMs);
    }
    return combined;
}

// 跟随位置就地解析（无自驱 tick; setFollowPlayer / draw* / refresh 时调用）
void FloatingTextManager::resolveFollowPosition(FloatingText& ft) {
    if (ft.followPlayer.empty()) return;
    auto level = ll::service::getLevel();
    if (!level.has_value()) return;
    auto* player = level->getPlayer(ft.followPlayer);
    if (!player) return;
    auto pos = player->getPosition();
    ft.x = pos.x;
    ft.y = pos.y + ft.followOffsetY;
    ft.z = pos.z;
}

// 整块样式落盘: 缩放 / 颜色 / 背景框 / 穿墙 / 旋转（每次重建都完整重新套用）
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
            ft.backgroundColor->a
        );
    } else {
        shapeMgr.clearBackgroundColor(shapeId); // 未设 = 客户端默认背景色
    }
    shapeMgr.setDepthTest(shapeId, ft.depthTest);
    if (ft.useRotation) {
        shapeMgr.setRotation(shapeId, ft.rotPitch, ft.rotYaw, ft.rotRoll);
    } else {
        shapeMgr.clearRotation(shapeId); // 恢复面向相机
    }
}

void FloatingTextManager::destroyViewerShapes(FloatingText& ft) {
    if (ft.viewerShapes.empty()) return;
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    for (auto const& [name, sid] : ft.viewerShapes) {
        if (sid >= 0 && shapeMgr.exists(sid)) shapeMgr.remove(sid);
        mLastSentText.erase(sid);
    }
    ft.viewerShapes.clear();
}

void FloatingTextManager::rebuildTextShapesPerViewer(FloatingText& ft) {
    auto& shapeMgr = PacketDebugRenderer::getInstance();
    auto  level    = ll::service::getLevel();
    if (!level) return;

    // 共享形状退场(切到逐观看者)
    if (ft.textShapeId >= 0 && shapeMgr.exists(ft.textShapeId)) {
        shapeMgr.remove(ft.textShapeId);
    }
    ft.textShapeId = -1;

    // 在线玩家集合(维度目标按维度过滤; 离线/离开维度观看者的形状就地销毁)
    std::unordered_set<std::string> online;
    level->forEachPlayer([&](Player& p) -> bool {
        std::string const name = p.getRealName();
        if (ft.drawTarget == FloatingText::DrawTarget::Dimension
            && static_cast<int>(p.getDimensionId()) != ft.dimId) {
            auto it = ft.viewerShapes.find(name);
            if (it != ft.viewerShapes.end()) {
                if (shapeMgr.exists(it->second)) shapeMgr.remove(it->second);
                ft.viewerShapes.erase(it);
            }
            return true;
        }
        online.insert(name);
        return true;
    });
    for (auto it = ft.viewerShapes.begin(); it != ft.viewerShapes.end();) {
        if (!online.contains(it->first)) {
            if (shapeMgr.exists(it->second)) shapeMgr.remove(it->second);
            it = ft.viewerShapes.erase(it);
        } else {
            ++it;
        }
    }

    // 每个观看者: 原地更新或新建(按该玩家解析文本), 并套用整块样式
    auto const ts = steadyMs();
    for (auto const& name : online) {
        std::string combined;
        combined.reserve(ft.lines.size() * 16);
        for (size_t i = 0; i < ft.lines.size(); ++i) {
            if (i > 0) combined += '\n';
            combined += composeLine(ft.lines[i], name, ts);
        }

        int64_t& sid = ft.viewerShapes[name];
        if (sid >= 0 && shapeMgr.exists(sid)) {
            shapeMgr.setText(sid, combined);
            shapeMgr.setLocation(sid, ft.x, ft.y, ft.z);
            applyBlockStyle(sid, ft);
            // 改完必须**重发**（同 networkId 原地覆盖）: 此前这里只改内存不发送,
            // 逐观看者文本的更新要等 15s 兜底重发才到客户端 —— "动态文本不刷新"的根因
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
    destroyViewerShapes(ft); // 逐观看者形状一并销毁(若有)
    if (ft.textShapeId < 0) return;
    mLastSentText.erase(ft.textShapeId);
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

    // 逐玩家变量: 目标不是单玩家时, 每个观看者一份按自己解析的形状
    // (共享单形状只能解析一次, 全员看到同一份 —— {player} 之类会失真)
    if (textHasVariables(ft) && ft.drawTarget != FloatingText::DrawTarget::Player) {
        rebuildTextShapesPerViewer(ft);
        return;
    }
    // 曾处于逐观看者模式(变量被清掉/目标改为单玩家): 清掉观看者形状切回共享
    destroyViewerShapes(ft);

    // 合并所有行 (\n), 逐行组装（内容池 → 变量解析）
    std::string const context =
        (ft.drawTarget == FloatingText::DrawTarget::Player) ? ft.targetPlayer : ft.followPlayer;
    std::string const combined = composeText(ft, context, steadyMs());

    if (ft.textShapeId >= 0 && shapeMgr.exists(ft.textShapeId)) {
        // 原地更新 (保留 networkId → 客户端覆盖式刷新)
        shapeMgr.setText(ft.textShapeId, combined);
    } else {
        // 首次创建
        ft.textShapeId = shapeMgr.createText(ft.x, ft.y, ft.z, combined);
        if (ft.textShapeId < 0) return;
    }
    // 整块样式（颜色/缩放/背景框/穿墙/旋转）完整重新套用
    applyBlockStyle(ft.textShapeId, ft);
    mLastSentText[ft.textShapeId] = combined; // 驱动比对基线
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
