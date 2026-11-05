#include "ParticleShapeManager.h"
#include "core/PacketSink.h"
#include "core/TickDriver.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/Packet.h>
#include <mc/network/packet/SpawnParticleEffectPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <sstream>

namespace debugshape_export {

namespace {

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

constexpr int TOTAL_CAP = 2048;

void sampleSegment(std::vector<float>& out, float ax, float ay, float az, float bx, float by, float bz, float step) {
    float dx = bx - ax, dy = by - ay, dz = bz - az;
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6f) {
        out.insert(out.end(), {ax, ay, az});
        return;
    }
    int count = std::max(1, (int)std::ceil(len / step));
    for (int i = 0; i <= count && (int)out.size() / 3 < TOTAL_CAP; i++) {
        float t = (float)i / (float)count;
        out.insert(out.end(), {ax + dx * t, ay + dy * t, az + dz * t});
    }
}

inline void axisToXYZ(int axis, float u, float v, float& x, float& y, float& z) {
    switch (axis) {
    case 0:
        x = u;
        y = v;
        z = 0;
        break;
    case 1:
        x = 0;
        y = v;
        z = u;
        break;
    default:
        x = u;
        y = 0;
        z = v;
        break;
    }
}

void rectCorners(int axis, float w, float h, float out[4][3]) {
    float hw = w / 2.0f, hh = h / 2.0f;
    float uv[4][2] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    for (int i = 0; i < 4; i++) {
        axisToXYZ(axis, uv[i][0], uv[i][1], out[i][0], out[i][1], out[i][2]);
    }
}

void eulerToMatrix(float rx, float ry, float rz, float m[9]) {
    float cx = std::cos(rx * 3.14159265358979f / 180.0f), sx = std::sin(rx * 3.14159265358979f / 180.0f);
    float cy = std::cos(ry * 3.14159265358979f / 180.0f), sy = std::sin(ry * 3.14159265358979f / 180.0f);
    float cz = std::cos(rz * 3.14159265358979f / 180.0f), sz = std::sin(rz * 3.14159265358979f / 180.0f);

    m[0] = cz * cy;
    m[1] = cz * sy * sx - sz * cx;
    m[2] = cz * sy * cx + sz * sx;
    m[3] = sz * cy;
    m[4] = sz * sy * sx + cz * cx;
    m[5] = sz * sy * cx - cz * sx;
    m[6] = -sy;
    m[7] = cy * sx;
    m[8] = cy * cx;
}

inline void applyMatrix(float const m[9], float x, float y, float z, float& ox, float& oy, float& oz) {
    ox = m[0] * x + m[1] * y + m[2] * z;
    oy = m[3] * x + m[4] * y + m[5] * z;
    oz = m[6] * x + m[7] * y + m[8] * z;
}

std::vector<std::vector<float>> parseTriples(std::string const& csv) {
    std::vector<std::vector<float>> out;
    std::stringstream               ss(csv);
    std::string                     item;
    while (std::getline(ss, item, ';')) {
        std::stringstream  ts(item);
        std::string        part;
        std::vector<float> v;
        while (std::getline(ts, part, ',')) {
            try {
                v.push_back(std::stof(part));
            } catch (...) {
                break;
            }
        }
        if (v.size() == 3)
            out.push_back(v);
    }
    return out;
}

} // namespace

ParticleShapeManager& ParticleShapeManager::getInstance() {
    static ParticleShapeManager instance;
    return instance;
}

void ParticleShapeManager::init() {
    mInitialized.store(true, std::memory_order_relaxed);
}

void ParticleShapeManager::shutdown() {
    mShapes.clear();
    mInitialized.store(false, std::memory_order_relaxed);
}

int64_t ParticleShapeManager::insertShape(Shape&& s) {
    auto const now = currentTick();
    s.endTick      = s.endTick > 0 ? now + (std::uint64_t)s.endTick : 0;
    s.nextEmitTick = now + 1;
    sampleLocal(s);
    int64_t const id = mShapes.insert(std::make_unique<Shape>(std::move(s)));
    mShapes.withObject(id, [&](Shape& shape) {
        shape.id = id;
    });
    return id;
}

void ParticleShapeManager::sampleLocal(Shape& s) {
    s.localPts.clear();
    auto& pts = s.localPts;
    switch (s.kind) {
    case Kind::Point:
        pts.insert(pts.end(), {0.0f, 0.0f, 0.0f});
        break;
    case Kind::Line:
        sampleSegment(pts, s.ax, s.ay, s.az, s.bx, s.by, s.bz, s.step);
        break;
    case Kind::Rect: {
        float c[4][3];
        rectCorners(s.axis, s.w, s.h, c);
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            sampleSegment(pts, c[i][0], c[i][1], c[i][2], c[j][0], c[j][1], c[j][2], s.step);
        }
        break;
    }
    case Kind::Plane: {
        float hw = s.w / 2.0f, hh = s.h / 2.0f;
        for (float u = -hw; u <= hw + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; u += s.step) {
            for (float v = -hh; v <= hh + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; v += s.step) {
                float x, y, z;
                axisToXYZ(s.axis, u, v, x, y, z);
                pts.insert(pts.end(), {x, y, z});
            }
        }
        break;
    }
    case Kind::Box: {

        float const corners[8][3] = {
            {-s.hx, -s.hy, -s.hz},
            {s.hx, -s.hy, -s.hz},
            {s.hx, -s.hy, s.hz},
            {-s.hx, -s.hy, s.hz},
            {-s.hx, s.hy, -s.hz},
            {s.hx, s.hy, -s.hz},
            {s.hx, s.hy, s.hz},
            {-s.hx, s.hy, s.hz},
        };
        static int const edges[12][2] = {
            {0, 1},
            {1, 2},
            {2, 3},
            {3, 0},
            {4, 5},
            {5, 6},
            {6, 7},
            {7, 4},
            {0, 4},
            {1, 5},
            {2, 6},
            {3, 7},
        };
        for (auto const& e : edges) {
            sampleSegment(
                pts,
                corners[e[0]][0], corners[e[0]][1], corners[e[0]][2],
                corners[e[1]][0], corners[e[1]][1], corners[e[1]][2],
                s.step);
        }
        break;
    }
    case Kind::BoxFaces: {

        for (int face = 0; face < 6 && (int)pts.size() / 3 < TOTAL_CAP; face++) {

            for (float u = -1.0f; u <= 1.0f + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; u += s.step) {
                for (float v = -1.0f; v <= 1.0f + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; v += s.step) {
                    float x, y, z;
                    switch (face) {
                    case 0:
                        x = -s.hx;
                        y = u * s.hy;
                        z = v * s.hz;
                        break;
                    case 1:
                        x = s.hx;
                        y = u * s.hy;
                        z = v * s.hz;
                        break;
                    case 2:
                        x = u * s.hx;
                        y = -s.hy;
                        z = v * s.hz;
                        break;
                    case 3:
                        x = u * s.hx;
                        y = s.hy;
                        z = v * s.hz;
                        break;
                    case 4:
                        x = u * s.hx;
                        y = v * s.hy;
                        z = -s.hz;
                        break;
                    default:
                        x = u * s.hx;
                        y = v * s.hy;
                        z = s.hz;
                        break;
                    }
                    pts.insert(pts.end(), {x, y, z});
                }
            }
        }
        break;
    }
    case Kind::Poly: {
        size_t const n = s.polyVerts.size() / 3;
        for (size_t e = 0; e + 1 < s.polyEdges.size(); e += 2) {
            auto i = (size_t)s.polyEdges[e];
            auto j = (size_t)s.polyEdges[e + 1];
            if (i >= n || j >= n)
                continue;
            sampleSegment(
                pts,
                s.polyVerts[i * 3], s.polyVerts[i * 3 + 1], s.polyVerts[i * 3 + 2],
                s.polyVerts[j * 3], s.polyVerts[j * 3 + 1], s.polyVerts[j * 3 + 2],
                s.step);
        }
        break;
    }
    }
    s.cacheValid = true;
}

int64_t ParticleShapeManager::createPoint(
    std::string const& playerName, int dimId,
    float x, float y, float z,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind          = Kind::Point;
    s.playerName    = playerName;
    s.dimId         = dimId;
    s.px            = x;
    s.py            = y;
    s.pz            = z;
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    s.viewDistance  = 0;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createLine(
    std::string const& playerName, int dimId,
    float x1, float y1, float z1, float x2, float y2, float z2, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind       = Kind::Line;
    s.playerName = playerName;
    s.dimId      = dimId;

    s.px            = (x1 + x2) / 2.0f;
    s.py            = (y1 + y2) / 2.0f;
    s.pz            = (z1 + z2) / 2.0f;
    s.ax            = x1 - s.px;
    s.ay            = y1 - s.py;
    s.az            = z1 - s.pz;
    s.bx            = x2 - s.px;
    s.by            = y2 - s.py;
    s.bz            = z2 - s.pz;
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createRect(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float w, float h, int axis, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind          = Kind::Rect;
    s.playerName    = playerName;
    s.dimId         = dimId;
    s.px            = cx;
    s.py            = cy;
    s.pz            = cz;
    s.w             = std::max(0.0f, w);
    s.h             = std::max(0.0f, h);
    s.axis          = axis >= 0 && axis <= 2 ? axis : 0;
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createPlane(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float w, float h, int axis, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind          = Kind::Plane;
    s.playerName    = playerName;
    s.dimId         = dimId;
    s.px            = cx;
    s.py            = cy;
    s.pz            = cz;
    s.w             = std::max(0.0f, w);
    s.h             = std::max(0.0f, h);
    s.axis          = axis >= 0 && axis <= 2 ? axis : 0;
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createBox(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float hx, float hy, float hz, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind          = Kind::Box;
    s.playerName    = playerName;
    s.dimId         = dimId;
    s.px            = cx;
    s.py            = cy;
    s.pz            = cz;
    s.hx            = std::max(0.0f, hx);
    s.hy            = std::max(0.0f, hy);
    s.hz            = std::max(0.0f, hz);
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createBoxFaces(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float hx, float hy, float hz, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    Shape s;
    s.kind          = Kind::BoxFaces;
    s.playerName    = playerName;
    s.dimId         = dimId;
    s.px            = cx;
    s.py            = cy;
    s.pz            = cz;
    s.hx            = std::max(0.0f, hx);
    s.hy            = std::max(0.0f, hy);
    s.hz            = std::max(0.0f, hz);
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createPoly(
    std::string const& playerName, int dimId,
    std::string const& vertsCsv, std::string const& edgesCsv, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    auto verts = parseTriples(vertsCsv);
    if (verts.size() < 2)
        return -1;

    std::vector<std::int32_t> edges;
    {
        std::stringstream ss(edgesCsv);
        std::string       item;
        while (std::getline(ss, item, ';')) {
            auto dash = item.find('-');
            if (dash == std::string::npos)
                continue;
            try {
                auto i = std::stoi(item.substr(0, dash));
                auto j = std::stoi(item.substr(dash + 1));
                if (i >= 0 && j >= 0) {
                    edges.push_back(i);
                    edges.push_back(j);
                }
            } catch (...) {
            }
        }
    }
    std::vector<float> flat;
    flat.reserve(verts.size() * 3);
    for (auto const& v : verts)
        flat.insert(flat.end(), v.begin(), v.end());
    return createPoly(playerName, dimId, flat, edges, step, effect, intervalTicks, lifetimeTicks);
}

int64_t ParticleShapeManager::createPoly(
    std::string const& playerName, int dimId,
    std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks) {
    size_t const n = verts.size() / 3;
    if (n < 2 || edges.empty() || (edges.size() % 2) != 0)
        return -1;
    for (auto idx : edges) {
        if (idx < 0 || (size_t)idx >= n)
            return -1;
    }
    Shape s;
    s.kind       = Kind::Poly;
    s.playerName = playerName;
    s.dimId      = dimId;

    float mx = 0, my = 0, mz = 0;
    for (size_t i = 0; i < n; i++) {
        mx += verts[i * 3];
        my += verts[i * 3 + 1];
        mz += verts[i * 3 + 2];
    }
    mx /= (float)n;
    my /= (float)n;
    mz /= (float)n;
    s.px = mx;
    s.py = my;
    s.pz = mz;
    s.polyVerts.reserve(n * 3);
    for (size_t i = 0; i < n; i++) {
        s.polyVerts.insert(
            s.polyVerts.end(),
            {verts[i * 3] - mx, verts[i * 3 + 1] - my, verts[i * 3 + 2] - mz});
    }
    s.polyEdges     = edges;
    s.step          = std::max(0.25f, step);
    s.effect        = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick       = lifetimeTicks;
    return insertShape(std::move(s));
}

bool ParticleShapeManager::setPos(int64_t id, float x, float y, float z) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.px = x;
        s.py = y;
        s.pz = z;
        s.followUuidStr.clear();
        s.animActive = false;
    });
}

bool ParticleShapeManager::moveBy(int64_t id, float dx, float dy, float dz) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.px += dx;
        s.py += dy;
        s.pz += dz;
    });
}

bool ParticleShapeManager::moveTo(int64_t id, float x, float y, float z, int durationTicks) {
    return mShapes.withObject(id, [&](Shape& s) -> bool {
        if (durationTicks <= 0) {

            s.px         = x;
            s.py         = y;
            s.pz         = z;
            s.animActive = false;
            s.followUuidStr.clear();
            return true;
        }
        s.animActive        = true;
        s.animFromX         = s.px;
        s.animFromY         = s.py;
        s.animFromZ         = s.pz;
        s.animToX           = x;
        s.animToY           = y;
        s.animToZ           = z;
        s.animStartTick     = currentTick();
        s.animDurationTicks = durationTicks;
        s.followUuidStr.clear();
        return true;
    });
}

bool ParticleShapeManager::setRot(int64_t id, float rx, float ry, float rz) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.rotX = rx;
        s.rotY = ry;
        s.rotZ = rz;
    });
}

bool ParticleShapeManager::spin(int64_t id, float sx, float sy, float sz) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.spinX = sx;
        s.spinY = sy;
        s.spinZ = sz;
    });
}

bool ParticleShapeManager::setScale(int64_t id, float value) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.scale = value > 0.01f ? value : 0.01f;
    });
}

bool ParticleShapeManager::follow(int64_t id, std::string const& uuid, float offX, float offY, float offZ) {
    if (!mce::UUID::canParse(uuid))
        return false;
    return mShapes.withObject(id, [&](Shape& s) {
        s.followUuidStr = uuid;
        s.followUuid    = mce::UUID::fromString(uuid);
        s.offX          = offX;
        s.offY          = offY;
        s.offZ          = offZ;
    });
}

bool ParticleShapeManager::unfollow(int64_t id) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.followUuidStr.clear();
    });
}

bool ParticleShapeManager::setEffect(int64_t id, std::string const& effect) {
    if (effect.empty())
        return false;
    return mShapes.withObject(id, [&](Shape& s) {
        s.effect      = effect;
        s.framePosOff = static_cast<std::size_t>(-1);
    });
}

bool ParticleShapeManager::setVisible(int64_t id, std::string const& playersCsv) {
    std::vector<std::string> uuids;
    if (!playersCsv.empty()) {
        std::stringstream ss(playersCsv);
        std::string       item;
        while (std::getline(ss, item, ',')) {

            auto b = item.find_first_not_of(" \t");
            auto e = item.find_last_not_of(" \t");
            if (b == std::string::npos)
                continue;
            uuids.push_back(item.substr(b, e - b + 1));
        }
    }
    return setVisiblePlayers(id, uuids);
}

bool ParticleShapeManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.visiblePlayers.clear();
        for (auto const& uuid : playerUuids) {
            if (mce::UUID::canParse(uuid))
                s.visiblePlayers.insert(uuid);
        }
        s.visibleAll = s.visiblePlayers.empty();
    });
}

bool ParticleShapeManager::clearVisiblePlayers(int64_t id) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.visiblePlayers.clear();
        s.visibleAll = true;
    });
}

bool ParticleShapeManager::setInterval(int64_t id, int ticks) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.intervalTicks = std::max(1, ticks);
    });
}

bool ParticleShapeManager::setViewDistance(int64_t id, int blocks) {
    return mShapes.withObject(id, [&](Shape& s) {
        s.viewDistance = std::max(0, blocks);
    });
}

bool ParticleShapeManager::setLifetime(int64_t id, int ticks) {
    auto const now = currentTick();
    return mShapes.withObject(id, [&](Shape& s) {
        s.endTick = ticks > 0 ? now + (std::uint64_t)ticks : 0;
    });
}

bool ParticleShapeManager::destroy(int64_t id) {
    return mShapes.erase(id);
}

void ParticleShapeManager::destroyAll() {
    mShapes.clear();
}

bool ParticleShapeManager::exists(int64_t id) const {
    return mShapes.exists(id);
}

std::vector<int64_t> ParticleShapeManager::getAllIds() const {
    return mShapes.snapshotAll();
}

std::string ParticleShapeManager::getInfo(int64_t id) const {
    std::string out;
    bool const  found = mShapes.withObject(id, [&](Shape const& s) {
        static char const* kindNames[] = {"point", "line", "rect", "plane", "box", "boxFaces", "poly"};
        out                            = std::format(
            "particle shape #{}: kind={} owner={} dim={} anchor=({:.2f},{:.2f},{:.2f}) rot=({:.1f},{:.1f},{:.1f}) "
            "spin=({:.1f},{:.1f},{:.1f}) scale={:.3f} pts={} step={} effect={} visible={} view={} interval={}t end={} "
            "moving={}{}",
            s.id,
            kindNames[(int)s.kind],
            s.playerName,
            s.dimId,
            s.px, s.py, s.pz,
            s.rotX, s.rotY, s.rotZ,
            s.spinX, s.spinY, s.spinZ,
            s.scale,
            s.localPts.size() / 3,
            s.step,
            s.effect,
            s.visibleAll ? "all" : std::to_string(s.visiblePlayers.size()) + " players",
            s.viewDistance,
            s.intervalTicks,
            s.endTick,
            s.animActive ? "yes" : "no",
            s.animActive
                ? std::format(" ->({:.2f},{:.2f},{:.2f})/{}t", s.animToX, s.animToY, s.animToZ, s.animDurationTicks)
                : std::string{});
    });
    return found ? out : "";
}

void ParticleShapeManager::tick() {

    static bool const kParticlesDisabled = [] {
        char const* v = std::getenv("HLIB_NO_PARTICLES");
        return v != nullptr && v[0] == '1';
    }();
    if (kParticlesDisabled)
        return;
    auto const           now = currentTick();
    std::vector<int64_t> due;
    std::vector<int64_t> expired;
    mShapes.forEachLocked([&](int64_t id, Shape& s) {
        if (s.endTick != 0 && now >= s.endTick) {
            expired.push_back(id);
            return;
        }

        if (s.spinX != 0.0f || s.spinY != 0.0f || s.spinZ != 0.0f) {
            s.rotX += s.spinX;
            s.rotY += s.spinY;
            s.rotZ += s.spinZ;
        }

        if (s.animActive) {
            std::uint64_t elapsed = now - s.animStartTick;
            if (elapsed >= (std::uint64_t)s.animDurationTicks) {
                s.px         = s.animToX;
                s.py         = s.animToY;
                s.pz         = s.animToZ;
                s.animActive = false;
            } else {
                float t = (float)elapsed / (float)s.animDurationTicks;
                t       = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
                s.px    = s.animFromX + (s.animToX - s.animFromX) * t;
                s.py    = s.animFromY + (s.animToY - s.animFromY) * t;
                s.pz    = s.animFromZ + (s.animToZ - s.animFromZ) * t;
            }
        }
        if (now >= s.nextEmitTick)
            due.push_back(id);
    });

    for (auto const id : expired) {
        mShapes.erase(id);
    }
    if (due.empty())
        return;

    std::vector<int64_t> dropAfterEmit;
    for (auto const& id : due) {
        bool const keep = mShapes.withObject(id, [&](Shape& s) -> bool {
            if (emitShape(s, now)) {
                s.nextEmitTick = now + (std::uint64_t)s.intervalTicks;
                return true;
            }
            return false;
        });
        if (!keep)
            dropAfterEmit.push_back(id);
    }
    for (auto const id : dropAfterEmit) {
        mShapes.erase(id);
    }
}

bool ParticleShapeManager::buildFrameTemplate(Shape& s) {
    auto packet = MinecraftPackets::createPacket(MinecraftPacketIds::SpawnParticleEffect);
    if (!packet)
        return false;
    auto* spe                = static_cast<SpawnParticleEffectPacket*>(packet.get());
    spe->mVanillaDimensionId = (uchar)s.dimId;
    spe->mActorId            = ::ActorUniqueID{-1};
    spe->mEffectName         = s.effect;
    spe->mMolangVariables    = std::nullopt;

    constexpr float mkX = -98765.5f, mkY = 87654.25f, mkZ = -76543.125f;
    spe->mPos = ::Vec3{mkX, mkY, mkZ};

    BinaryStream body;
    spe->write(body);
    float const       marker[3] = {mkX, mkY, mkZ};
    std::string const markerStr{reinterpret_cast<char const*>(marker), sizeof marker};
    auto              off = body.mBuffer.find(markerStr);
    if (off == std::string::npos)
        return false;

    auto const        id = static_cast<std::uint32_t>(spe->getId());
    std::size_t const hl = wire::headerLength(id);

    s.frameTpl    = wire::withHeader(id, std::string{body.mBuffer});
    s.framePosOff = hl + off;
    return true;
}

bool ParticleShapeManager::emitShape(Shape& s, [[maybe_unused]] std::uint64_t now) {
    auto level = ll::service::getLevel();
    if (!level)
        return true;

    std::vector<Player*> targets;
    auto                 collectDim = [&](int dim) {
        level->forEachPlayer([&](Player& pl) -> bool {
            if (((int)pl.getDimensionId()) == dim)
                targets.push_back(&pl);
            return true;
        });
    };
    if (s.visibleAll) {
        collectDim(s.dimId);
    } else {
        for (auto const& uuidStr : s.visiblePlayers) {
            auto* pl = level->getPlayer(mce::UUID::fromString(uuidStr));
            if (pl && ((int)pl->getDimensionId()) == s.dimId)
                targets.push_back(pl);
        }
    }

    if (targets.empty())
        return s.visibleAll;

    float anchorX = s.px, anchorY = s.py, anchorZ = s.pz;
    if (!s.followUuidStr.empty()) {
        auto* follower = level->getPlayer(s.followUuid);
        if (follower == nullptr)
            return true;
        auto const& fpos = follower->getPosition();
        anchorX          = (float)fpos.x + s.offX;
        anchorY          = (float)fpos.y + s.offY;
        anchorZ          = (float)fpos.z + s.offZ;

        if (((int)follower->getDimensionId()) != s.dimId) {
            s.dimId       = ((int)follower->getDimensionId());
            s.framePosOff = static_cast<std::size_t>(-1);
            targets.clear();
            collectDim(s.dimId);
            if (s.visibleAll && targets.empty())
                return true;
        }
    }

    if (!s.cacheValid || s.localPts.empty())
        sampleLocal(s);
    auto const& pts = s.localPts;
    if (pts.empty())
        return true;

    float m[9];
    eulerToMatrix(s.rotX, s.rotY, s.rotZ, m);
    std::vector<float> world(pts.size());
    for (size_t i = 0; i + 2 < pts.size(); i += 3) {
        float x, y, z;
        applyMatrix(m, pts[i] * s.scale, pts[i + 1] * s.scale, pts[i + 2] * s.scale, x, y, z);
        world[i]     = x + anchorX;
        world[i + 1] = y + anchorY;
        world[i + 2] = z + anchorZ;
    }
    size_t const n = pts.size() / 3;

    if (s.framePosOff == static_cast<std::size_t>(-1) && !buildFrameTemplate(s))
        return true;

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem)
        return true;

    static thread_local std::string frame;
    frame             = s.frameTpl;
    auto patchAndSend = [&](NetworkPeer& peer, size_t i, ::Player* pl) {
        std::memcpy(&frame[s.framePosOff], &world[i * 3], 4);
        std::memcpy(&frame[s.framePosOff + 4], &world[i * 3 + 1], 4);
        std::memcpy(&frame[s.framePosOff + 8], &world[i * 3 + 2], 4);
        ::debugshape_export::wire::sendRawToPeer(
            peer,
            *pl,
            frame,
            ::debugshape_export::wire::Reliability::Reliable,
            "SpawnParticleEffectPacket",
            static_cast<std::uint32_t>(MinecraftPacketIds::SpawnParticleEffect));
    };

    for (auto* player : targets) {
        auto* peer = networkSystem->getPeerForUser(player->getNetworkIdentifier());
        if (peer == nullptr)
            continue;
        if (s.viewDistance > 0) {
            auto const&  ppos = player->getPosition();
            double const px = ppos.x, py = ppos.y, pz = ppos.z;
            double const r2 = (double)s.viewDistance * (double)s.viewDistance;
            for (size_t i = 0; i < n; i++) {
                double dx = world[i * 3] - px, dy = world[i * 3 + 1] - py, dz = world[i * 3 + 2] - pz;
                if (dx * dx + dy * dy + dz * dz > r2)
                    continue;
                patchAndSend(*peer, i, player);
            }
        } else {
            for (size_t i = 0; i < n; i++) {
                patchAndSend(*peer, i, player);
            }
        }
    }
    return true;
}

void particleShapeTick() {
    ParticleShapeManager::getInstance().tick();
}

tick::AutoRegister gParticleShapeTickHookRegistrar{"particles", &particleShapeTick, tick::Order::ParticleShape};

} // namespace debugshape_export
