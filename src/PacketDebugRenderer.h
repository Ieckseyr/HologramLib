

#pragma once

#include "core/Registry.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ProtocolPackets.h"
#include "ProtocolShape.h"

#include <ll/api/event/Listener.h>

namespace debugshape_export {

struct ShapeData {
    int64_t      id;
    LSEShapeType type;
    ProtoShape   proto;
    bool         visible{false};

    enum class Target : uint8_t { None,
                                  All,
                                  Dimension,
                                  Player };
    Target      target{Target::None};
    std::string targetPlayer;
};

class PacketDebugRenderer {
public:
    static PacketDebugRenderer& getInstance();

    PacketDebugRenderer(const PacketDebugRenderer&)            = delete;
    PacketDebugRenderer& operator=(const PacketDebugRenderer&) = delete;

    int64_t createText(float x, float y, float z, const std::string& text);
    int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2);
    int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2);
    int64_t createCircle(float x, float y, float z, float scale);
    int64_t createSphere(float x, float y, float z, float scale);
    int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2);

    int64_t createCylinder(float x, float y, float z, float radiusX, float radiusZ, float height, int segments);
    int64_t createPyramid(float x, float y, float z, float width, float height, float depth);
    int64_t createEllipsoid(float x, float y, float z, float radiusX, float radiusY, float radiusZ, int segmentsPerAxis);
    int64_t createCone(float x, float y, float z, float radiusX, float radiusZ, float height, int segments);

    bool setText(int64_t id, const std::string& text);
    bool setLocation(int64_t id, float x, float y, float z);
    bool setColor(int64_t id, float r, float g, float b, float a);
    bool setScale(int64_t id, float scale);
    bool setDuration(int64_t id, float seconds);
    bool setDimension(int64_t id, int dimId);
    bool setRotation(int64_t id, float pitch, float yaw, float roll);
    bool clearRotation(int64_t id);

    bool setMaxRenderDistance(int64_t id, float distance);
    bool setAttachedEntity(int64_t id, int64_t entityUniqueId);
    bool clearAttachedEntity(int64_t id);

    bool setBackgroundColor(int64_t id, float r, float g, float b, float a);
    bool clearBackgroundColor(int64_t id);
    bool setDepthTest(int64_t id, bool enabled);

    std::string        getText(int64_t id);
    std::vector<float> getLocation(int64_t id);
    std::vector<float> getColor(int64_t id);
    int                getShapeType(int64_t id);
    std::vector<float> getRotation(int64_t id);

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

    std::vector<int64_t> findTextByLocation(float x, float y, float z, float radius);
    int64_t              findTextByLocationAndContent(float x, float y, float z, float radius, const std::string& text);
    std::vector<int64_t> getAllShapeIds();
    bool                 exists(int64_t id);

    bool destroy(int64_t id);
    bool destroyBatch(const std::vector<int64_t>& ids);
    void destroyAll();

    uint64_t getNetworkId(int64_t id);

    void init();
    void shutdown();

private:
    void tickResend(float deltaTime);

private:
    PacketDebugRenderer()  = default;
    ~PacketDebugRenderer() = default;

    uint64_t generateNetworkId();

    void resendVisibleToPlayer(::Player& player, bool log = true);

    core::Registry<ShapeData> mShapes;

    std::atomic<uint64_t> mNextNetworkId{0x00FFFFFF};

    ll::event::ListenerPtr mJoinListener{nullptr};
    ll::event::ListenerPtr mTickListener{nullptr};
    float                  mResendTimer{0};
};

} // namespace debugshape_export
