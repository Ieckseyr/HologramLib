#pragma once

#include "core/Registry.h"

#include <atomic>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <mc/platform/UUID.h>

namespace debugshape_export {

// 粒子形状
class ParticleShapeManager {
public:
    enum class Kind : int {
        Point = 0,
        Line,
        Rect,
        Plane,
        Box,
        BoxFaces,
        Poly,
    };

    struct Shape {
        int64_t     id{};
        Kind        kind{};
        std::string playerName;

        float                     ax{}, ay{}, az{}, bx{}, by{}, bz{};
        float                     w{}, h{};
        int                       axis{};
        float                     hx{}, hy{}, hz{};
        std::vector<float>        polyVerts;
        std::vector<std::int32_t> polyEdges;
        float                     step{1.0f};

        float px{}, py{}, pz{};
        float rotX{}, rotY{}, rotZ{};
        float scale{1.0f};
        float spinX{}, spinY{}, spinZ{};

        mce::UUID   followUuid;
        std::string followUuidStr;
        float       offX{}, offY{}, offZ{};

        bool          animActive{};
        float         animFromX{}, animFromY{}, animFromZ{};
        float         animToX{}, animToY{}, animToZ{};
        std::uint64_t animStartTick{};
        int           animDurationTicks{};

        std::string           effect{"minecraft:endrod"};
        std::set<std::string> visiblePlayers;
        int                   viewDistance{96};
        int                   intervalTicks{4};
        int                   dimId{};

        std::uint64_t endTick{};
        std::uint64_t nextEmitTick{};
        bool          visibleAll{true};

        std::string frameTpl;
        std::size_t framePosOff{static_cast<std::size_t>(-1)};

        bool               cacheValid{false};
        std::vector<float> localPts;
    };

    static ParticleShapeManager& getInstance();

    void init();
    void shutdown();

    int64_t createPoint(
        std::string const& playerName, int dimId,
        float x, float y, float z,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createLine(
        std::string const& playerName, int dimId,
        float x1, float y1, float z1, float x2, float y2, float z2, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createRect(
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createPlane(
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createBox(
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createBoxFaces(
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);
    int64_t createPoly(
        std::string const& playerName, int dimId,
        std::string const& vertsCsv, std::string const& edgesCsv, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);

    int64_t createPoly(
        std::string const& playerName, int dimId,
        std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks);

    bool setPos(int64_t id, float x, float y, float z);
    bool moveBy(int64_t id, float dx, float dy, float dz);
    bool moveTo(int64_t id, float x, float y, float z, int durationTicks);
    bool setRot(int64_t id, float rx, float ry, float rz);
    bool spin(int64_t id, float sx, float sy, float sz);
    bool setScale(int64_t id, float s);
    bool follow(int64_t id, std::string const& uuid, float offX, float offY, float offZ);
    bool unfollow(int64_t id);
    bool setEffect(int64_t id, std::string const& effect);
    bool setVisible(int64_t id, std::string const& playersCsv);

    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids);
    bool clearVisiblePlayers(int64_t id);
    bool setInterval(int64_t id, int ticks);
    bool setViewDistance(int64_t id, int blocks);
    bool setLifetime(int64_t id, int ticks);

    bool                 destroy(int64_t id);
    void                 destroyAll();
    bool                 exists(int64_t id) const;
    std::vector<int64_t> getAllIds() const;
    std::string          getInfo(int64_t id) const;

    void tick();

private:
    int64_t     insertShape(Shape&& s);
    static void sampleLocal(Shape& s);
    bool        emitShape(Shape& s, std::uint64_t now);
    bool        buildFrameTemplate(Shape& s);

    core::Registry<Shape> mShapes;
    std::atomic_bool      mInitialized{false};
};

} // namespace debugshape_export
