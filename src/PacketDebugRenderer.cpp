#include "DiagFlags.h"
// PacketDebugRenderer.cpp - 基于 BedrockProtocol v2168 的形状渲染器实现
//
// 直接持有 v2168 PrimitiveShapes，通过 PrimitiveShapesPacket + NetworkPeer::sendPacket 发送。
#include "PacketDebugRenderer.h"

#include "DiagLog.h"

#include "EventIdCompat.h"

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

PacketDebugRenderer& PacketDebugRenderer::getInstance() {
    static PacketDebugRenderer instance;
    return instance;
}

uint64_t PacketDebugRenderer::generateNetworkId() {
    return mNextNetworkId.fetch_sub(1, std::memory_order_relaxed);
}

// 创建形状 - 直接构建 v944 DebugShape

int64_t PacketDebugRenderer::createText(float x, float y, float z, const std::string& text) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Text;
    shape->proto = makeTextShape(generateNetworkId(), Vec3{x, y, z}, text);

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createLine(float x1, float y1, float z1, float x2, float y2, float z2) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Line;
    shape->proto = makeLineShape(generateNetworkId(), Vec3{x1, y1, z1}, Vec3{x2, y2, z2});

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createBox(float x1, float y1, float z1, float x2, float y2, float z2) {
    Vec3 center{(x1 + x2) / 2.0f, (y1 + y2) / 2.0f, (z1 + z2) / 2.0f};
    Vec3 bound{std::abs(x2 - x1), std::abs(y2 - y1), std::abs(z2 - z1)};

    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Box;
    shape->proto = makeBoxShape(generateNetworkId(), center, bound);

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createCircle(float x, float y, float z, float scale) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Circle;
    shape->proto = makeCircleShape(generateNetworkId(), Vec3{x, y, z}, scale);

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createSphere(float x, float y, float z, float scale) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Sphere;
    shape->proto = makeSphereShape(generateNetworkId(), Vec3{x, y, z}, scale);

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createArrow(float x1, float y1, float z1, float x2, float y2, float z2) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Arrow;
    shape->proto = makeArrowShape(generateNetworkId(), Vec3{x1, y1, z1}, Vec3{x2, y2, z2});

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

// 属性设置 - 直接修改 DebugShape

bool PacketDebugRenderer::setText(int64_t id, const std::string& text) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        auto* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (textData) {
            textData->mText = text;
        }
    });
}

bool PacketDebugRenderer::setLocation(int64_t id, float x, float y, float z) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mLocation = Vec3{x, y, z};
    });
}

bool PacketDebugRenderer::setColor(int64_t id, float r, float g, float b, float a) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mColor = ProtoColor::fromFloat(r, g, b, a).toPacked();
    });
}

bool PacketDebugRenderer::setScale(int64_t id, float scale) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mScale = scale;
    });
}

bool PacketDebugRenderer::setDuration(int64_t id, float seconds) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mTotalTimeLeft = seconds;
    });
}

bool PacketDebugRenderer::setDimension(int64_t id, int dimId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mDimensionId = dimId;
    });
}

bool PacketDebugRenderer::setRotation(int64_t id, float pitch, float yaw, float roll) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mRotation = Vec3{pitch, yaw, roll};
        // 文本形状: 旋转要生效必须同时开 useRotation（否则客户端永远让它面向相机）,
        // 并按官方脚本 API 的默认行为打开双面渲染（否则转到背面就看不见）
        if (auto* textData = std::get_if<DebugText>(&shape.proto.mShape)) {
            textData->mUseRotation      = true;
            textData->mShowBackface     = true;
            textData->mShowTextBackface = true;
        }
    });
}

bool PacketDebugRenderer::clearRotation(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mRotation = std::nullopt;
        if (auto* textData = std::get_if<DebugText>(&shape.proto.mShape)) {
            textData->mUseRotation      = false; // 回到面向相机（billboard）
            textData->mShowBackface     = false;
            textData->mShowTextBackface = false;
        }
    });
}

// 文本专属: 背景框颜色 / 清除回默认 / 穿墙开关（其余形状没有这些字段）
bool PacketDebugRenderer::setBackgroundColor(int64_t id, float r, float g, float b, float a) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        auto* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (!textData)
            return false;
        textData->mBackgroundColor = ProtoColor::fromFloat(r, g, b, a).toPacked();
        return true;
    });
}

bool PacketDebugRenderer::clearBackgroundColor(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        auto* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (!textData)
            return false;
        textData->mBackgroundColor = std::nullopt; // 不设 = 客户端默认背景色
        return true;
    });
}

bool PacketDebugRenderer::setDepthTest(int64_t id, bool enabled) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        auto* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (!textData)
            return false;
        textData->mDepthTest = enabled; // true = 被方块/实体遮挡; false = 始终渲染（穿墙可见）
        return true;
    });
}

// 属性获取

std::string PacketDebugRenderer::getText(int64_t id) {
    std::string out;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        auto const* textData = std::get_if<DebugText>(&shape.proto.mShape);
        out                  = textData ? textData->mText : "";
    });
    return out;
}

std::vector<float> PacketDebugRenderer::getLocation(int64_t id) {
    std::vector<float> out;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        if (!shape.proto.mLocation.has_value())
            return;
        auto const& loc = *shape.proto.mLocation;
        out             = {loc.mX, loc.mY, loc.mZ};
    });
    return out;
}

std::vector<float> PacketDebugRenderer::getColor(int64_t id) {
    std::vector<float> out;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        if (!shape.proto.mColor.has_value())
            return;
        // 从 int32 打包颜色解包（ARGB: A 最高字节, R 次之, G 再次, B 最低 —— 与 ProtoColor::toPacked 对应）
        std::uint32_t packed = static_cast<std::uint32_t>(*shape.proto.mColor);
        float         r      = static_cast<std::uint8_t>((packed >> 16) & 0xFF) / 255.0f;
        float         g      = static_cast<std::uint8_t>((packed >> 8) & 0xFF) / 255.0f;
        float         b      = static_cast<std::uint8_t>(packed & 0xFF) / 255.0f;
        float         a      = static_cast<std::uint8_t>((packed >> 24) & 0xFF) / 255.0f;
        out                  = {r, g, b, a};
    });
    return out;
}

int PacketDebugRenderer::getShapeType(int64_t id) {
    int out = -1;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        out = static_cast<int>(shape.type);
    });
    return out;
}

std::vector<float> PacketDebugRenderer::getRotation(int64_t id) {
    std::vector<float> out;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        if (!shape.proto.mRotation.has_value())
            return;
        auto const& rot = *shape.proto.mRotation;
        out             = {rot.mX, rot.mY, rot.mZ};
    });
    return out;
}

// 显示控制 - 通过 NetworkPeer::sendPacket 发送

bool PacketDebugRenderer::draw(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.visible = true;
        shape.target  = ShapeData::Target::All;
        shape.targetPlayer.clear();
        return ProtocolPacketWriter::sendToAll({shape.proto});
    });
}

bool PacketDebugRenderer::drawToPlayer(int64_t id, const std::string& playerName) {
    return mShapes.withObject(id, [&](ShapeData& shape) -> bool {
        shape.visible      = true;
        shape.target       = ShapeData::Target::Player;
        shape.targetPlayer = playerName;
        auto level         = ll::service::getLevel();
        if (!level.has_value())
            return false;
        auto* player = level->getPlayer(playerName);
        if (!player)
            return false;
        return ProtocolPacketWriter::sendToPlayer(*player, {shape.proto});
    });
}

bool PacketDebugRenderer::drawToDimension(int64_t id, int dimId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.visible            = true;
        shape.proto.mDimensionId = dimId;
        shape.target             = ShapeData::Target::Dimension;
        shape.targetPlayer.clear();
        return ProtocolPacketWriter::sendToDimension(dimId, {shape.proto});
    });
}

bool PacketDebugRenderer::drawBatch(const std::vector<int64_t>& ids) {
    std::vector<ProtoShape> shapesList;
    shapesList.reserve(ids.size());
    for (int64_t id : ids) {
        mShapes.withObject(id, [&](ShapeData& shape) {
            shape.visible = true;
            shape.target  = ShapeData::Target::All;
            shape.targetPlayer.clear();
            shapesList.push_back(shape.proto);
        });
    }
    if (shapesList.empty()) return true;
    return ProtocolPacketWriter::sendToAll(shapesList);
}

bool PacketDebugRenderer::remove(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.visible    = false;
        auto removeShape = makeRemoveShape(shape.proto.mNetworkId);
        return ProtocolPacketWriter::sendToAll({removeShape});
    });
}

bool PacketDebugRenderer::removeToPlayer(int64_t id, const std::string& playerName) {
    return mShapes.withObject(id, [&](ShapeData& shape) -> bool {
        shape.visible = false;
        auto level    = ll::service::getLevel();
        if (!level.has_value())
            return false;
        auto* player = level->getPlayer(playerName);
        if (!player)
            return false;
        auto removeShape = makeRemoveShape(shape.proto.mNetworkId);
        return ProtocolPacketWriter::sendToPlayer(*player, {removeShape});
    });
}

bool PacketDebugRenderer::removeToDimension(int64_t id, int dimId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.visible    = false;
        auto removeShape = makeRemoveShape(shape.proto.mNetworkId);
        return ProtocolPacketWriter::sendToDimension(dimId, {removeShape});
    });
}

bool PacketDebugRenderer::update(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        if (shape.visible) {
            // 协议语义: 同 networkId 的完整形状 = 客户端原地替换 (原子更新)
            // 不能先发移除包再重建 —— 倒计时每秒刷新会导致整个文本框闪烁
            ProtocolPacketWriter::sendToAll({shape.proto});
        }
    });
}

bool PacketDebugRenderer::updateToPlayer(int64_t id, const std::string& playerName) {
    return mShapes.withObject(id, [&](ShapeData& shape) -> bool {
        if (!shape.visible)
            return true;
        auto level = ll::service::getLevel();
        if (!level.has_value())
            return false;
        auto* player = level->getPlayer(playerName);
        if (!player)
            return false;
        // 同 networkId 原地替换, 不先删后建 (防闪烁)
        return ProtocolPacketWriter::sendToPlayer(*player, {shape.proto});
    });
}

bool PacketDebugRenderer::updateToDimension(int64_t id, int dimId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        if (shape.visible) {
            // 同 networkId 原地替换, 不先删后建 (防闪烁)
            ProtocolPacketWriter::sendToDimension(dimId, {shape.proto});
        }
    });
}

// 查询

std::vector<int64_t> PacketDebugRenderer::findTextByLocation(float x, float y, float z, float radius) {
    std::vector<int64_t> result;
    float const          radiusSq = radius * radius;
    mShapes.forEachLocked([&](int64_t id, ShapeData const& shape) {
        if (shape.type != LSEShapeType::Text)
            return;
        if (!shape.proto.mLocation.has_value())
            return;
        auto const& loc = *shape.proto.mLocation;
        float       dx = loc.mX - x, dy = loc.mY - y, dz = loc.mZ - z;
        if (dx * dx + dy * dy + dz * dz <= radiusSq)
            result.push_back(id);
    });
    return result;
}

int64_t PacketDebugRenderer::findTextByLocationAndContent(float x, float y, float z, float radius, const std::string& text) {
    float const radiusSq = radius * radius;
    int64_t     found    = -1;
    mShapes.forEachLocked([&](int64_t id, ShapeData const& shape) {
        if (found >= 0)
            return; // 已命中, 不再扫描
        if (shape.type != LSEShapeType::Text)
            return;
        auto const* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (!textData || textData->mText != text)
            return;
        if (!shape.proto.mLocation.has_value())
            return;
        auto const& loc = *shape.proto.mLocation;
        float       dx = loc.mX - x, dy = loc.mY - y, dz = loc.mZ - z;
        if (dx * dx + dy * dy + dz * dz <= radiusSq)
            found = id;
    });
    return found;
}

std::vector<int64_t> PacketDebugRenderer::getAllShapeIds() {
    return mShapes.snapshotAll();
}

bool PacketDebugRenderer::exists(int64_t id) {
    return mShapes.exists(id);
}

// 生命周期

bool PacketDebugRenderer::destroy(int64_t id) {
    std::uint64_t networkId = 0;
    if (!mShapes.withObject(id, [&](ShapeData const& shape) {
            networkId = shape.proto.mNetworkId;
        })) {
        return false;
    }
    // 先发移除包再删内存 (否则客户端残留显示, 悬浮字不消失)
    // 未知 networkId 的移除包对未收到过该形状的客户端无副作用
    ProtocolPacketWriter::sendToAll({makeRemoveShape(networkId)});
    mShapes.erase(id);
    return true;
}

bool PacketDebugRenderer::destroyBatch(const std::vector<int64_t>& ids) {
    std::vector<ProtoShape> removes;
    removes.reserve(ids.size());
    for (int64_t id : ids) {
        std::uint64_t networkId = 0;
        if (!mShapes.withObject(id, [&](ShapeData const& shape) {
                networkId = shape.proto.mNetworkId;
            })) {
            continue;
        }
        removes.push_back(makeRemoveShape(networkId));
        mShapes.erase(id);
    }
    if (removes.empty()) return true;
    // 先发移除包再清内存 (与 destroy 一致; 未知 networkId 的移除包对客户端无副作用)
    return ProtocolPacketWriter::sendToAll(removes);
}

void PacketDebugRenderer::destroyAll() {
    if (mShapes.empty()) return;
    std::vector<ProtoShape> removes;
    removes.reserve(mShapes.size());
    mShapes.forEachLocked([&](int64_t, ShapeData const& shape) {
        removes.push_back(makeRemoveShape(shape.proto.mNetworkId));
    });
    mShapes.clear();
    // 客户端与内存同批清理 (此前只清内存 → 客户端残留显示)
    ProtocolPacketWriter::sendToAll(removes);
}

uint64_t PacketDebugRenderer::getNetworkId(int64_t id) {
    std::uint64_t networkId = 0;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        networkId = shape.proto.mNetworkId;
    });
    return networkId;
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
    if (mJoinListener) return; // 已初始化

    mJoinListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
        [this](ll::event::PlayerJoinEvent& ev) {
            if (hlibDomainDisabled("HLIB_NO_SHAPE")) return;
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
    if (mShapes.empty())
        return;

    auto level = ll::service::getLevel();
    if (!level.has_value()) return;
    level->forEachPlayer([this](::Player& p) -> bool {
        resendVisibleToPlayer(p, false); // 静默: 周期重发不打日志
        return true;
    });
}

void PacketDebugRenderer::resendVisibleToPlayer(::Player& player, bool log) {
    std::vector<ProtoShape> toSend;
    auto const              playerName = player.mName.get();
    auto const              playerDim  = ((int)player.getDimensionId());
    toSend.reserve(mShapes.size());
    mShapes.forEachLocked([&](int64_t, ShapeData const& shape) {
        if (!shape.visible)
            return;
        if (shape.target == ShapeData::Target::Player) {
            // 私有形状只发给目标玩家
            if (shape.targetPlayer != playerName)
                return;
        } else if (shape.proto.mDimensionId.has_value()) {
            // 维度限定形状: 服务端按玩家当前维度过滤(与 drawToDimension 语义一致)
            if (*shape.proto.mDimensionId != playerDim)
                return;
        }
        toSend.push_back(shape.proto);
    });
    if (!toSend.empty()) {
        ProtocolPacketWriter::sendToPlayer(player, toSend);
        if (log) {
            HLIB_LOG_INFO("[DebugDrawer] 进服重发: 玩家 {} 补发 {} 个形状", playerName, toSend.size());
        }
    }
}

} // namespace debugshape_export
