// PacketDebugRenderer.h - 基于 Protocol v944 的形状渲染器
//
// 直接持有 v944 DebugShape，通过 DebugDrawerPacket + NetworkPeer::sendPacket 发送。
#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ProtocolShape.h"
#include "ProtocolPackets.h"

namespace debugshape_export {

// 形状数据 - 直接持有 v944 DebugShape
struct ShapeData {
    int64_t     id;          // 插件层ID
    LSEShapeType type;       // 形状类型（LSE 兼容）
    ProtoShape  proto;       // v944 协议层数据
    bool        visible{false};
};

class PacketDebugRenderer {
public:
    static PacketDebugRenderer& getInstance();

    PacketDebugRenderer(const PacketDebugRenderer&) = delete;
    PacketDebugRenderer& operator=(const PacketDebugRenderer&) = delete;

    // 创建形状（返回插件层ID）
    int64_t createText(float x, float y, float z, const std::string& text);
    int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2);
    int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2);
    int64_t createCircle(float x, float y, float z, float scale);
    int64_t createSphere(float x, float y, float z, float scale);
    int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2);

    // 属性设置
    bool setText(int64_t id, const std::string& text);
    bool setLocation(int64_t id, float x, float y, float z);
    bool setColor(int64_t id, float r, float g, float b, float a);
    bool setScale(int64_t id, float scale);
    bool setDuration(int64_t id, float seconds);
    bool setDimension(int64_t id, int dimId);
    bool setRotation(int64_t id, float pitch, float yaw, float roll);
    bool clearRotation(int64_t id);

    // 属性获取
    std::string getText(int64_t id);
    std::vector<float> getLocation(int64_t id);
    std::vector<float> getColor(int64_t id);
    int getShapeType(int64_t id);
    std::vector<float> getRotation(int64_t id);

    // 显示控制
    bool draw(int64_t id);
    bool drawToPlayer(int64_t id, const std::string& playerName);
    bool drawToDimension(int64_t id, int dimId);
    bool drawBatch(const std::vector<int64_t>& ids);

    bool remove(int64_t id);
    bool removeToPlayer(int64_t id, const std::string& playerName);
    bool removeToDimension(int64_t id, int dimId);

    bool update(int64_t id);
    bool updateToPlayer(int64_t id, const std::string& playerName);
    bool updateToDimension(int64_t id, int dimId);

    // 查询
    std::vector<int64_t> findTextByLocation(float x, float y, float z, float radius);
    int64_t findTextByLocationAndContent(float x, float y, float z, float radius, const std::string& text);
    std::vector<int64_t> getAllShapeIds();
    bool exists(int64_t id);

    // 生命周期
    bool destroy(int64_t id);
    bool destroyBatch(const std::vector<int64_t>& ids);
    void destroyAll();

    void tick(float deltaTime);

    // 获取形状的 networkId（供高级管理器使用）
    uint64_t getNetworkId(int64_t id);

private:
    PacketDebugRenderer() = default;
    ~PacketDebugRenderer() = default;

    ShapeData* getShape(int64_t id);
    const ShapeData* getShape(int64_t id) const;

    uint64_t generateNetworkId();

    std::unordered_map<int64_t, std::unique_ptr<ShapeData>> mShapes;
    int64_t  mNextId{1};
    uint64_t mNextNetworkId{UINT64_MAX};
    std::mutex mMutex;
};

} // namespace debugshape_export
