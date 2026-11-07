#include "ParticleShapeManager.h"
#include "core/TickDriver.h"
#include "kernel/PacketSink.h"

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

} // namespace

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

} // namespace debugshape_export
