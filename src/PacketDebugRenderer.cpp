#include "DiagFlags.h"
#include "core/EventIds.h"

#include "PacketDebugRenderer.h"

#include "DiagLog.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

PacketDebugRenderer& PacketDebugRenderer::getInstance() {
    static PacketDebugRenderer instance;
    return instance;
}

uint64_t PacketDebugRenderer::generateNetworkId() {
    return mNextNetworkId.fetch_sub(1, std::memory_order_relaxed);
}

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

int64_t PacketDebugRenderer::createCylinder(
    float x,
    float y,
    float z,
    float radiusX,
    float radiusZ,
    float height,
    int   segments) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Cylinder;
    shape->proto = makeCylinderShape(
        generateNetworkId(),
        Vec3{x, y, z},
        radiusX,
        radiusZ,
        height,
        static_cast<std::uint8_t>(std::clamp(segments, 3, 255)));

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createPyramid(float x, float y, float z, float width, float height, float depth) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Pyramid;
    shape->proto = makePyramidShape(generateNetworkId(), Vec3{x, y, z}, width, height, depth);

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createEllipsoid(
    float x,
    float y,
    float z,
    float radiusX,
    float radiusY,
    float radiusZ,
    int   segmentsPerAxis) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Ellipsoid;
    shape->proto = makeEllipsoidShape(
        generateNetworkId(),
        Vec3{x, y, z},
        Vec3{radiusX, radiusY, radiusZ},
        static_cast<std::uint8_t>(std::clamp(segmentsPerAxis, 3, 255)));

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

int64_t PacketDebugRenderer::createCone(float x, float y, float z, float radiusX, float radiusZ, float height, int segments) {
    auto shape   = std::make_unique<ShapeData>();
    shape->type  = LSEShapeType::Cone;
    shape->proto = makeConeShape(
        generateNetworkId(),
        Vec3{x, y, z},
        radiusX,
        radiusZ,
        height,
        static_cast<std::uint8_t>(std::clamp(segments, 3, 255)));

    int64_t const id = mShapes.insert(std::move(shape));
    mShapes.withObject(id, [&](ShapeData& s) {
        s.id = id;
    });
    return id;
}

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
            textData->mUseRotation      = false;
            textData->mShowBackface     = false;
            textData->mShowTextBackface = false;
        }
    });
}

bool PacketDebugRenderer::setMaxRenderDistance(int64_t id, float distance) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        if (distance > 0.0f) {
            shape.proto.mMaxRenderDistance = distance;
        } else {
            shape.proto.mMaxRenderDistance = std::nullopt;
        }
    });
}

bool PacketDebugRenderer::setAttachedEntity(int64_t id, int64_t entityUniqueId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mAttachedToEntityId = entityUniqueId;
    });
}

bool PacketDebugRenderer::clearAttachedEntity(int64_t id) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        shape.proto.mAttachedToEntityId = std::nullopt;
    });
}

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
        textData->mBackgroundColor = std::nullopt;
        return true;
    });
}

bool PacketDebugRenderer::setDepthTest(int64_t id, bool enabled) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        auto* textData = std::get_if<DebugText>(&shape.proto.mShape);
        if (!textData)
            return false;
        textData->mDepthTest = enabled;
        return true;
    });
}

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
    if (shapesList.empty())
        return true;
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

        return ProtocolPacketWriter::sendToPlayer(*player, {shape.proto});
    });
}

bool PacketDebugRenderer::updateToDimension(int64_t id, int dimId) {
    return mShapes.withObject(id, [&](ShapeData& shape) {
        if (shape.visible) {

            ProtocolPacketWriter::sendToDimension(dimId, {shape.proto});
        }
    });
}

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
            return;
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

bool PacketDebugRenderer::destroy(int64_t id) {
    std::uint64_t networkId = 0;
    if (!mShapes.withObject(id, [&](ShapeData const& shape) {
            networkId = shape.proto.mNetworkId;
        })) {
        return false;
    }

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
    if (removes.empty())
        return true;

    return ProtocolPacketWriter::sendToAll(removes);
}

void PacketDebugRenderer::destroyAll() {
    if (mShapes.empty())
        return;
    std::vector<ProtoShape> removes;
    removes.reserve(mShapes.size());
    mShapes.forEachLocked([&](int64_t, ShapeData const& shape) {
        removes.push_back(makeRemoveShape(shape.proto.mNetworkId));
    });
    mShapes.clear();

    ProtocolPacketWriter::sendToAll(removes);
}

uint64_t PacketDebugRenderer::getNetworkId(int64_t id) {
    std::uint64_t networkId = 0;
    mShapes.withObject(id, [&](ShapeData const& shape) {
        networkId = shape.proto.mNetworkId;
    });
    return networkId;
}

void PacketDebugRenderer::init() {
    if (mJoinListener)
        return;

    mJoinListener = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                [this](ll::event::PlayerJoinEvent& ev) {
                                                                                    if (hlibDomainDisabled("HLIB_NO_SHAPE"))
                                                                                        return;

                                                                                    auto playerKey = ev.self().getNetworkIdentifier().getIPAndPort();
                                                                                    auto resend    = [this, playerKey]() {
                                                                                        auto level = ll::service::getLevel();
                                                                                        if (!level.has_value())
                                                                                            return;

                                                                                        ::Player* target = nullptr;
                                                                                        level->forEachPlayer([&](::Player& p) -> bool {
                                                                                            if (p.getNetworkIdentifier().getIPAndPort() == playerKey) {
                                                                                                target = &p;
                                                                                                return true;
                                                                                            }
                                                                                            return false;
                                                                                        });
                                                                                        if (target)
                                                                                            resendVisibleToPlayer(*target);
                                                                                    };
                                                                                    auto& executor = ll::thread::ServerThreadExecutor::getDefault();
                                                                                    executor.executeAfter(resend, std::chrono::milliseconds(1000));
                                                                                    executor.executeAfter(resend, std::chrono::milliseconds(5000));
                                                                                });

    mTickListener = debugshape_export::core::listen<ll::event::ServerLevelTickEvent>(core::kServerLevelTickId,
                                                                                     [this](ll::event::ServerLevelTickEvent const&) {
                                                                                         tickResend(0.05f);
                                                                                     });
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

void PacketDebugRenderer::tickResend(float deltaTime) {
    mResendTimer += deltaTime;
    if (mResendTimer < 15.0f)
        return;
    mResendTimer = 0.0f;

    if (mShapes.empty())
        return;

    auto level = ll::service::getLevel();
    if (!level.has_value())
        return;
    level->forEachPlayer([this](::Player& p) -> bool {
        resendVisibleToPlayer(p, false);
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

            if (shape.targetPlayer != playerName)
                return;
        } else if (shape.proto.mDimensionId.has_value()) {

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
