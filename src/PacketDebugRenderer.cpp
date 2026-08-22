// PacketDebugRenderer.cpp - 基于 Protocol v944 的形状渲染器实现
//
// 直接持有 v944 DebugShape，通过 DebugDrawerPacket + NetworkPeer::sendPacket 发送。
#include "PacketDebugRenderer.h"

#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <algorithm>
#include <cmath>

namespace debugshape_export {

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
    return ProtocolPacketWriter::sendToAll({shape->proto});
}

bool PacketDebugRenderer::drawToPlayer(int64_t id, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto* shape = getShape(id);
    if (!shape) return false;
    shape->visible = true;
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
    return ProtocolPacketWriter::sendToDimension(dimId, {shape->proto});
}

bool PacketDebugRenderer::drawBatch(const std::vector<int64_t>& ids) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<ProtoShape> shapesList;
    for (int64_t id : ids) {
        auto* shape = getShape(id);
        if (shape) {
            shape->visible = true;
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
