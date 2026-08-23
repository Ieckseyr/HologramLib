// PacketDebugRenderer.cpp - 基于 Protocol v944 的形状渲染器实现
//
// 直接持有 v944 DebugShape，通过 DebugDrawerPacket + NetworkPeer::sendPacket 发送。
#include "PacketDebugRenderer.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace debugshape_export {

namespace {
auto& pdLogger() {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *log;
}
} // namespace

PacketDebugRenderer& PacketDebugRenderer::getInstance() {
    static PacketDebugRenderer instance;
    return instance;
}

uint64_t PacketDebugRenderer::generateNetworkId() {
    return mNextNetworkId--;
}

// 创建形状 - 直接构建 v944 DebugShape

int64_t PacketDebugRenderer::createText(float x, float y, float z, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Text;
    shape->proto = makeTextShape(generateNetworkId(), Vec3{x, y, z}, text);
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

int64_t PacketDebugRenderer::createLine(float x1, float y1, float z1, float x2, float y2, float z2) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Line;
    shape->proto = makeLineShape(generateNetworkId(), Vec3{x1, y1, z1}, Vec3{x2, y2, z2});
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

int64_t PacketDebugRenderer::createBox(float x1, float y1, float z1, float x2, float y2, float z2) {
    std::lock_guard<std::mutex> lock(mMutex);
    Vec3 center{(x1 + x2) / 2.0f, (y1 + y2) / 2.0f, (z1 + z2) / 2.0f};
    Vec3 bound{std::abs(x2 - x1), std::abs(y2 - y1), std::abs(z2 - z1)};
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Box;
    shape->proto = makeBoxShape(generateNetworkId(), center, bound);
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

int64_t PacketDebugRenderer::createCircle(float x, float y, float z, float scale) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Circle;
    shape->proto = makeCircleShape(generateNetworkId(), Vec3{x, y, z}, scale);
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

int64_t PacketDebugRenderer::createSphere(float x, float y, float z, float scale) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Sphere;
    shape->proto = makeSphereShape(generateNetworkId(), Vec3{x, y, z}, scale);
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

int64_t PacketDebugRenderer::createArrow(float x1, float y1, float z1, float x2, float y2, float z2) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto shape = std::make_unique<ShapeData>();
    shape->id = mNextId++;
    shape->type = LSEShapeType::Arrow;
    shape->proto = makeArrowShape(generateNetworkId(), Vec3{x1, y1, z1}, Vec3{x2, y2, z2});
    int64_t id = shape->id;
    mShapes[id] = std::move(shape);
    return id;
}

// 属性设置 - 直接修改 DebugShape

bool PacketDebugRenderer::setText(int64_t id, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    auto* textData = std::get_if<DebugText>(&shape->proto.mShape);
    if (textData) {
        textData->mText = text;
    }
    return true;
}

bool PacketDebugRenderer::setLocation(int64_t id, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mLocation = Vec3{x, y, z};
    return true;
}

bool PacketDebugRenderer::setColor(int64_t id, float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mColor = ProtoColor::fromFloat(r, g, b, a).toPacked();
    return true;
}

bool PacketDebugRenderer::setScale(int64_t id, float scale) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mScale = scale;
    return true;
}

bool PacketDebugRenderer::setDuration(int64_t id, float seconds) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mTotalTimeLeft = seconds;
    return true;
}

bool PacketDebugRenderer::setDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mDimensionId = dimId;
    return true;
}

bool PacketDebugRenderer::setRotation(int64_t id, float pitch, float yaw, float roll) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mRotation = Vec3{pitch, yaw, roll};
    return true;
}

bool PacketDebugRenderer::clearRotation(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->proto.mRotation = std::nullopt;
    return true;
}

// 属性获取

std::string PacketDebugRenderer::getText(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return "";
    auto* textData = std::get_if<DebugText>(&shape->proto.mShape);
    return textData ? textData->mText : "";
}

std::vector<float> PacketDebugRenderer::getLocation(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape || !shape->proto.mLocation.has_value()) return {};
    auto& loc = *shape->proto.mLocation;
    return {loc.mX, loc.mY, loc.mZ};
}

std::vector<float> PacketDebugRenderer::getColor(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape || !shape->proto.mColor.has_value()) return {};
    // 从 int32 打包颜色解包（与 ProtoColor::toPacked 对应）
    std::uint32_t packed = static_cast<std::uint32_t>(*shape->proto.mColor);
    float r = static_cast<std::uint8_t>(packed & 0xFF) / 255.0f;
    float g = static_cast<std::uint8_t>((packed >> 8) & 0xFF) / 255.0f;
    float b = static_cast<std::uint8_t>((packed >> 16) & 0xFF) / 255.0f;
    float a = static_cast<std::uint8_t>((packed >> 24) & 0xFF) / 255.0f;
    return {r, g, b, a};
}

int PacketDebugRenderer::getShapeType(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    return shape ? static_cast<int>(shape->type) : -1;
}

std::vector<float> PacketDebugRenderer::getRotation(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape || !shape->proto.mRotation.has_value()) return {};
    auto& rot = *shape->proto.mRotation;
    return {rot.mX, rot.mY, rot.mZ};
}

// 显示控制 - 通过 NetworkPeer::sendPacket 发送

bool PacketDebugRenderer::draw(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = true;
    shape->target = ShapeData::Target::All;
    shape->targetPlayer.clear();
    return ProtocolPacketWriter::sendToAll({shape->proto});
}

bool PacketDebugRenderer::drawToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = true;
    shape->target = ShapeData::Target::Player;
    shape->targetPlayer = playerName;
    auto level = ll::service::getLevel();
    if (!level.has_value()) return false;
    auto* player = level->getPlayer(playerName);
    if (!player) return false;
    return ProtocolPacketWriter::sendToPlayer(*player, {shape->proto});
}

bool PacketDebugRenderer::drawToDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = true;
    shape->proto.mDimensionId = dimId;
    shape->target = ShapeData::Target::Dimension;
    shape->targetPlayer.clear();
    return ProtocolPacketWriter::sendToDimension(dimId, {shape->proto});
}

bool PacketDebugRenderer::drawBatch(const std::vector<int64_t>& ids) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<ProtoShape> shapesList;
    for (int64_t id : ids) {
        auto* shape = getShape(id);
        if (shape) {
            shape->visible = true;
            shape->target = ShapeData::Target::All;
            shape->targetPlayer.clear();
            shapesList.push_back(shape->proto);
        }
    }
    if (shapesList.empty()) return true;
    return ProtocolPacketWriter::sendToAll(shapesList);
}

bool PacketDebugRenderer::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = false;
    auto removeShape = makeRemoveShape(shape->proto.mNetworkId);
    return ProtocolPacketWriter::sendToAll({removeShape});
}

bool PacketDebugRenderer::removeToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = false;
    auto level = ll::service::getLevel();
    if (!level.has_value()) return false;
    auto* player = level->getPlayer(playerName);
    if (!player) return false;
    auto removeShape = makeRemoveShape(shape->proto.mNetworkId);
    return ProtocolPacketWriter::sendToPlayer(*player, {removeShape});
}

bool PacketDebugRenderer::removeToDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = false;
    auto removeShape = makeRemoveShape(shape->proto.mNetworkId);
    return ProtocolPacketWriter::sendToDimension(dimId, {removeShape});
}

bool PacketDebugRenderer::update(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    if (shape->visible) {
        // 协议语义: 同 networkId 的完整形状 = 客户端原地替换 (原子更新)
        // 不能先发移除包再重建 —— 倒计时每秒刷新会导致整个文本框闪烁
        ProtocolPacketWriter::sendToAll({shape->proto});
    }
    return true;
}

bool PacketDebugRenderer::updateToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    if (shape->visible) {
        auto level = ll::service::getLevel();
        if (!level.has_value()) return false;
        auto* player = level->getPlayer(playerName);
        if (!player) return false;
        // 同 networkId 原地替换, 不先删后建 (防闪烁)
        ProtocolPacketWriter::sendToPlayer(*player, {shape->proto});
    }
    return true;
}

bool PacketDebugRenderer::updateToDimension(int64_t id, int dimId) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    if (shape->visible) {
        // 同 networkId 原地替换, 不先删后建 (防闪烁)
        ProtocolPacketWriter::sendToDimension(dimId, {shape->proto});
    }
    return true;
}

// 查询

std::vector<int64_t> PacketDebugRenderer::findTextByLocation(float x, float y, float z, float radius) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<int64_t> result;
    float radiusSq = radius * radius;
    for (const auto& [id, shape] : mShapes) {
        if (shape->type != LSEShapeType::Text) continue;
        if (!shape->proto.mLocation.has_value()) continue;
        auto& loc = *shape->proto.mLocation;
        float dx = loc.mX - x, dy = loc.mY - y, dz = loc.mZ - z;
        if (dx * dx + dy * dy + dz * dz <= radiusSq) result.push_back(id);
    }
    return result;
}

int64_t PacketDebugRenderer::findTextByLocationAndContent(float x, float y, float z, float radius, const std::string& text) {
    std::lock_guard<std::mutex> lock(mMutex);
    float radiusSq = radius * radius;
    for (const auto& [id, shape] : mShapes) {
        if (shape->type != LSEShapeType::Text) continue;
        auto* textData = std::get_if<DebugText>(&shape->proto.mShape);
        if (!textData || textData->mText != text) continue;
        if (!shape->proto.mLocation.has_value()) continue;
        auto& loc = *shape->proto.mLocation;
        float dx = loc.mX - x, dy = loc.mY - y, dz = loc.mZ - z;
        if (dx * dx + dy * dy + dz * dz <= radiusSq) return id;
    }
    return -1;
}

std::vector<int64_t> PacketDebugRenderer::getAllShapeIds() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mShapes.size());
    for (const auto& [id, shape] : mShapes) ids.push_back(id);
    return ids;
}

bool PacketDebugRenderer::exists(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    return mShapes.find(id) != mShapes.end();
}

// 生命周期

bool PacketDebugRenderer::destroy(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    // 先发移除包再删内存 (否则客户端残留显示, 悬浮字不消失)
    // 未知 networkId 的移除包对未收到过该形状的客户端无副作用
    auto removeShape = makeRemoveShape(it->second->proto.mNetworkId);
    ProtocolPacketWriter::sendToAll({removeShape});
    mShapes.erase(it);
    return true;
}

bool PacketDebugRenderer::destroyBatch(const std::vector<int64_t>& ids) {
    std::lock_guard<std::mutex> lock(mMutex);
    for (int64_t id : ids) mShapes.erase(id);
    return true;
}

void PacketDebugRenderer::destroyAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    mShapes.clear();
}

void PacketDebugRenderer::tick(float /*deltaTime*/) {
}

uint64_t PacketDebugRenderer::getNetworkId(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    return shape ? shape->proto.mNetworkId : 0;
}

// 进服重发：玩家加入后将所有可见形状按绘制目标过滤后补发。
// DebugDrawer 形状只存在于客户端内存, 玩家进服时客户端一片空白——
// 服务端必须在玩家就绪后重发全部可见形状, 否则悬浮字/形状全部不可见。
//
// 三重保障:
//   1. PlayerJoinEvent 后 1s + 5s 延迟重发（客户端加载世界期间立即发包会丢失）
//   2. 周期兜底: 每 15s 对全部在线玩家重发（覆盖加载超 5s / 事件异常等一切情况）
//   3. 同 networkId 重复发送 = 客户端原地覆盖, 幂等无副作用

void PacketDebugRenderer::init() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mJoinListener) return; // 已初始化

    mJoinListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
        [this](ll::event::PlayerJoinEvent& ev) {
            // PlayerJoinEvent 时客户端仍在加载世界, 立即发送的包会丢失;
            // 1s + 5s 双重发兜底（大世界/慢机器加载超 1s 时 5s 补救）
            auto playerKey = ev.self().getNetworkIdentifier().getIPAndPort();
            auto resend = [this, playerKey]() {
                auto level = ll::service::getLevel();
                if (!level.has_value()) return;

                ::Player* target = nullptr;
                level->forEachPlayer([&](::Player& p) -> bool {
                    if (p.getNetworkIdentifier().getIPAndPort() == playerKey) {
                        target = &p;
                        return true;
                    }
                    return false;
                });
                if (target) resendVisibleToPlayer(*target);
            };
            auto& executor = ll::thread::ServerThreadExecutor::getDefault();
            executor.executeAfter(resend, std::chrono::milliseconds(1000));
            executor.executeAfter(resend, std::chrono::milliseconds(5000));
        }
    );

    // 周期兜底重发: 无论客户端加载多慢/事件是否异常, 最迟 15s 必定补齐
    mTickListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ServerLevelTickEvent>(
        [this](ll::event::ServerLevelTickEvent const&) {
            tickResend(0.05f); // 20 tps 固定步长
        }
    );
}

void PacketDebugRenderer::shutdown() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mJoinListener) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener = nullptr;
    }
    if (mTickListener) {
        ll::event::EventBus::getInstance().removeListener(mTickListener);
        mTickListener = nullptr;
    }
}

// 周期兜底重发（tick 驱动, 每 15s 一次全量静默补发）
void PacketDebugRenderer::tickResend(float deltaTime) {
    mResendTimer += deltaTime;
    if (mResendTimer < 15.0f) return;
    mResendTimer = 0.0f;

    // 无可见形状时跳过（不持锁快查）
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mShapes.empty()) return;
    }

    auto level = ll::service::getLevel();
    if (!level.has_value()) return;
    level->forEachPlayer([this](::Player& p) -> bool {
        resendVisibleToPlayer(p, false); // 静默: 周期重发不打日志
        return true;
    });
}

void PacketDebugRenderer::resendVisibleToPlayer(::Player& player, bool log) {
    std::vector<ProtoShape> toSend;
    auto const playerName = player.mName.get();
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto const playerDim = player.getDimensionId().id;
        toSend.reserve(mShapes.size());
        for (auto const& [id, shape] : mShapes) {
            if (!shape->visible) continue;
            if (shape->target == ShapeData::Target::Player) {
                // 私有形状只发给目标玩家
                if (shape->targetPlayer != playerName) continue;
            } else if (shape->proto.mDimensionId.has_value()) {
                // 维度限定形状: 服务端按玩家当前维度过滤(与 drawToDimension 语义一致)
                if (*shape->proto.mDimensionId != playerDim) continue;
            }
            toSend.push_back(shape->proto);
        }
    }
    if (!toSend.empty()) {
        ProtocolPacketWriter::sendToPlayer(player, toSend);
        if (log) {
            pdLogger().info("[DebugDrawer] 进服重发: 玩家 {} 补发 {} 个形状", playerName, toSend.size());
        }
    }
}

// 内部方法

ShapeData* PacketDebugRenderer::getShape(int64_t id) {
    auto it = mShapes.find(id);
    return it != mShapes.end() ? it->second.get() : nullptr;
}

const ShapeData* PacketDebugRenderer::getShape(int64_t id) const {
    auto it = mShapes.find(id);
    return it != mShapes.end() ? it->second.get() : nullptr;
}

} // namespace debugshape_export
