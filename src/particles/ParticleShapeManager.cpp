// ParticleShapeManager.cpp - 通用协议层粒子形状系统实现
#include "ParticleShapeManager.h"

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
#include <cstring>
#include <format>
#include <sstream>

namespace debugshape_export {

namespace {

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

constexpr int TOTAL_CAP = 2048; // 单形状单次发射总点数上限（网络保护）

// ── 局部点采样辅助（全部写入局部坐标, 锚点为原点）──

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

// 平面两轴 → 世界分量映射（axis: 0=XY 1=YZ 2=XZ; u/v 为平面内坐标, n 为法向分量）
inline void axisToXYZ(int axis, float u, float v, float& x, float& y, float& z) {
    switch (axis) {
    case 0: x = u; y = v; z = 0; break; // XY 平面
    case 1: x = 0; y = v; z = u; break; // YZ 平面（u 沿 Z）
    default: x = u; y = 0; z = v; break; // XZ 平面（v 沿 Z）
    }
}

// 矩形四角（局部, 中心为原点）
void rectCorners(int axis, float w, float h, float out[4][3]) {
    float hw = w / 2.0f, hh = h / 2.0f;
    float uv[4][2] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    for (int i = 0; i < 4; i++) {
        axisToXYZ(axis, uv[i][0], uv[i][1], out[i][0], out[i][1], out[i][2]);
    }
}

// ZYX 欧拉（度）→ 3×3 旋转矩阵（行主序）
void eulerToMatrix(float rx, float ry, float rz, float m[9]) {
    float cx = std::cos(rx * 3.14159265358979f / 180.0f), sx = std::sin(rx * 3.14159265358979f / 180.0f);
    float cy = std::cos(ry * 3.14159265358979f / 180.0f), sy = std::sin(ry * 3.14159265358979f / 180.0f);
    float cz = std::cos(rz * 3.14159265358979f / 180.0f), sz = std::sin(rz * 3.14159265358979f / 180.0f);
    // R = Rz · Ry · Rx
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

// CSV 解析: "a,b,c;d,e,f" → 浮点三元组
std::vector<std::vector<float>> parseTriples(std::string const& csv) {
    std::vector<std::vector<float>> out;
    std::stringstream ss(csv);
    std::string       item;
    while (std::getline(ss, item, ';')) {
        std::stringstream ts(item);
        std::string       part;
        std::vector<float> v;
        while (std::getline(ts, part, ',')) {
            try { v.push_back(std::stof(part)); } catch (...) { break; }
        }
        if (v.size() == 3) out.push_back(v);
    }
    return out;
}

} // namespace

ParticleShapeManager& ParticleShapeManager::getInstance() {
    static ParticleShapeManager instance;
    return instance;
}

void ParticleShapeManager::init() {
    std::lock_guard lock(mMutex);
    mInitialized = true;
}

void ParticleShapeManager::shutdown() {
    std::lock_guard lock(mMutex);
    mShapes.clear();
    mInitialized = false;
}

int64_t ParticleShapeManager::insertShape(Shape&& s) {
    std::lock_guard lock(mMutex);
    s.id         = mNextId++;
    auto const now = currentTick();
    s.endTick    = s.endTick > 0 ? now + (std::uint64_t)s.endTick : 0;
    s.nextEmitTick = now + 1;
    sampleLocal(s);
    int64_t id = s.id;
    mShapes.emplace(id, std::move(s));
    return id;
}

// ── 局部点采样（各形状几何 → localPts）──

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
        // 12 条边: 顶/底面各 4 + 垂直 4
        float const corners[8][3] = {
            {-s.hx, -s.hy, -s.hz}, {s.hx, -s.hy, -s.hz}, {s.hx, -s.hy, s.hz}, {-s.hx, -s.hy, s.hz},
            {-s.hx, s.hy, -s.hz},  {s.hx, s.hy, -s.hz},  {s.hx, s.hy, s.hz},  {-s.hx, s.hy, s.hz},
        };
        static int const edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // 底面
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // 顶面
            {0, 4}, {1, 5}, {2, 6}, {3, 7}, // 垂直
        };
        for (auto const& e : edges) {
            sampleSegment(
                pts,
                corners[e[0]][0], corners[e[0]][1], corners[e[0]][2],
                corners[e[1]][0], corners[e[1]][1], corners[e[1]][2],
                s.step
            );
        }
        break;
    }
    case Kind::BoxFaces: {
        // 6 面网格: 每面 = 环面填充（两轴展开, 法向轴固定 ±h）
        for (int face = 0; face < 6 && (int)pts.size() / 3 < TOTAL_CAP; face++) {
            // face: 0/1 = X±, 2/3 = Y±, 4/5 = Z±
            for (float u = -1.0f; u <= 1.0f + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; u += s.step) {
                for (float v = -1.0f; v <= 1.0f + 1e-3f && (int)pts.size() / 3 < TOTAL_CAP; v += s.step) {
                    float x, y, z;
                    switch (face) {
                    case 0: x = -s.hx; y = u * s.hy; z = v * s.hz; break;
                    case 1: x = s.hx; y = u * s.hy; z = v * s.hz; break;
                    case 2: x = u * s.hx; y = -s.hy; z = v * s.hz; break;
                    case 3: x = u * s.hx; y = s.hy; z = v * s.hz; break;
                    case 4: x = u * s.hx; y = v * s.hy; z = -s.hz; break;
                    default: x = u * s.hx; y = v * s.hy; z = s.hz; break;
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
            if (i >= n || j >= n) continue;
            sampleSegment(
                pts,
                s.polyVerts[i * 3], s.polyVerts[i * 3 + 1], s.polyVerts[i * 3 + 2],
                s.polyVerts[j * 3], s.polyVerts[j * 3 + 1], s.polyVerts[j * 3 + 2],
                s.step
            );
        }
        break;
    }
    }
    s.cacheValid = true;
}

// ── 创建 ──

int64_t ParticleShapeManager::createPoint(
    std::string const& playerName, int dimId,
    float x, float y, float z,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::Point;
    s.playerName = playerName;
    s.dimId = dimId;
    s.px = x; s.py = y; s.pz = z;
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    s.viewDistance = 0;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createLine(
    std::string const& playerName, int dimId,
    float x1, float y1, float z1, float x2, float y2, float z2, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::Line;
    s.playerName = playerName;
    s.dimId = dimId;
    // 锚点 = 中点（旋转绕中点）, 局部端点 = 端点 - 中点
    s.px = (x1 + x2) / 2.0f; s.py = (y1 + y2) / 2.0f; s.pz = (z1 + z2) / 2.0f;
    s.ax = x1 - s.px; s.ay = y1 - s.py; s.az = z1 - s.pz;
    s.bx = x2 - s.px; s.by = y2 - s.py; s.bz = z2 - s.pz;
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createRect(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float w, float h, int axis, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::Rect;
    s.playerName = playerName;
    s.dimId = dimId;
    s.px = cx; s.py = cy; s.pz = cz;
    s.w = std::max(0.0f, w); s.h = std::max(0.0f, h);
    s.axis = axis >= 0 && axis <= 2 ? axis : 0;
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createPlane(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float w, float h, int axis, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::Plane;
    s.playerName = playerName;
    s.dimId = dimId;
    s.px = cx; s.py = cy; s.pz = cz;
    s.w = std::max(0.0f, w); s.h = std::max(0.0f, h);
    s.axis = axis >= 0 && axis <= 2 ? axis : 0;
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createBox(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float hx, float hy, float hz, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::Box;
    s.playerName = playerName;
    s.dimId = dimId;
    s.px = cx; s.py = cy; s.pz = cz;
    s.hx = std::max(0.0f, hx); s.hy = std::max(0.0f, hy); s.hz = std::max(0.0f, hz);
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createBoxFaces(
    std::string const& playerName, int dimId,
    float cx, float cy, float cz, float hx, float hy, float hz, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    Shape s;
    s.kind = Kind::BoxFaces;
    s.playerName = playerName;
    s.dimId = dimId;
    s.px = cx; s.py = cy; s.pz = cz;
    s.hx = std::max(0.0f, hx); s.hy = std::max(0.0f, hy); s.hz = std::max(0.0f, hz);
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

int64_t ParticleShapeManager::createPoly(
    std::string const& playerName, int dimId,
    std::string const& vertsCsv, std::string const& edgesCsv, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    auto verts = parseTriples(vertsCsv);
    if (verts.size() < 2) return -1;
    // 边: "i-j;i-j"
    std::vector<std::int32_t> edges;
    {
        std::stringstream ss(edgesCsv);
        std::string       item;
        while (std::getline(ss, item, ';')) {
            auto dash = item.find('-');
            if (dash == std::string::npos) continue;
            try {
                auto i = std::stoi(item.substr(0, dash));
                auto j = std::stoi(item.substr(dash + 1));
                if (i >= 0 && j >= 0) { edges.push_back(i); edges.push_back(j); }
            } catch (...) {}
        }
    }
    std::vector<float> flat;
    flat.reserve(verts.size() * 3);
    for (auto const& v : verts) flat.insert(flat.end(), v.begin(), v.end());
    return createPoly(playerName, dimId, flat, edges, step, effect, intervalTicks, lifetimeTicks);
}

int64_t ParticleShapeManager::createPoly(
    std::string const& playerName, int dimId,
    std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
    std::string const& effect, int intervalTicks, int lifetimeTicks
) {
    size_t const n = verts.size() / 3;
    if (n < 2 || edges.empty() || (edges.size() % 2) != 0) return -1;
    for (auto idx : edges) {
        if (idx < 0 || (size_t)idx >= n) return -1;
    }
    Shape s;
    s.kind = Kind::Poly;
    s.playerName = playerName;
    s.dimId = dimId;
    // 锚点 = 顶点质心; 局部顶点 = 顶点 - 质心
    float mx = 0, my = 0, mz = 0;
    for (size_t i = 0; i < n; i++) {
        mx += verts[i * 3];
        my += verts[i * 3 + 1];
        mz += verts[i * 3 + 2];
    }
    mx /= (float)n; my /= (float)n; mz /= (float)n;
    s.px = mx; s.py = my; s.pz = mz;
    s.polyVerts.reserve(n * 3);
    for (size_t i = 0; i < n; i++) {
        s.polyVerts.insert(
            s.polyVerts.end(),
            {verts[i * 3] - mx, verts[i * 3 + 1] - my, verts[i * 3 + 2] - mz}
        );
    }
    s.polyEdges = edges;
    s.step = std::max(0.25f, step);
    s.effect = effect.empty() ? "minecraft:endrod" : effect;
    s.intervalTicks = std::max(1, intervalTicks);
    s.endTick = lifetimeTicks;
    return insertShape(std::move(s));
}

// ── 运行时控制 ──

bool ParticleShapeManager::setPos(int64_t id, float x, float y, float z) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.px = x; it->second.py = y; it->second.pz = z;
    it->second.followUuidStr.clear(); // 手动设位 = 解除跟随
    it->second.animActive = false;    // 手动设位 = 中断进行中的动画
    return true;
}

bool ParticleShapeManager::moveBy(int64_t id, float dx, float dy, float dz) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.px += dx; it->second.py += dy; it->second.pz += dz;
    return true;
}

bool ParticleShapeManager::moveTo(int64_t id, float x, float y, float z, int durationTicks) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    auto& s = it->second;
    if (durationTicks <= 0) {
        // 零时长 = 立即到达（等价 setPos）
        s.px = x; s.py = y; s.pz = z;
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
    s.followUuidStr.clear(); // 显式移动 = 解除跟随
    return true;
}

bool ParticleShapeManager::setRot(int64_t id, float rx, float ry, float rz) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.rotX = rx; it->second.rotY = ry; it->second.rotZ = rz;
    return true;
}

bool ParticleShapeManager::spin(int64_t id, float sx, float sy, float sz) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.spinX = sx; it->second.spinY = sy; it->second.spinZ = sz;
    return true;
}

bool ParticleShapeManager::setScale(int64_t id, float s) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.scale = s > 0.01f ? s : 0.01f;
    return true;
}

bool ParticleShapeManager::follow(int64_t id, std::string const& uuid, float offX, float offY, float offZ) {
    if (!mce::UUID::canParse(uuid)) return false;
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    auto& s = it->second;
    s.followUuidStr = uuid;
    s.followUuid = mce::UUID::fromString(uuid);
    s.offX = offX; s.offY = offY; s.offZ = offZ;
    return true;
}

bool ParticleShapeManager::unfollow(int64_t id) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.followUuidStr.clear();
    return true;
}

bool ParticleShapeManager::setEffect(int64_t id, std::string const& effect) {
    if (effect.empty()) return false;
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.effect = effect;
    it->second.framePosOff = static_cast<std::size_t>(-1); // 效果名变化 → 帧模板重建
    return true;
}

bool ParticleShapeManager::setVisible(int64_t id, std::string const& playersCsv) {
    std::vector<std::string> uuids;
    if (!playersCsv.empty()) {
        std::stringstream ss(playersCsv);
        std::string       item;
        while (std::getline(ss, item, ',')) {
            // trim
            auto b = item.find_first_not_of(" \t");
            auto e = item.find_last_not_of(" \t");
            if (b == std::string::npos) continue;
            uuids.push_back(item.substr(b, e - b + 1));
        }
    }
    return setVisiblePlayers(id, uuids);
}

bool ParticleShapeManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    auto& s = it->second;
    s.visiblePlayers.clear();
    for (auto const& uuid : playerUuids) {
        if (mce::UUID::canParse(uuid)) s.visiblePlayers.insert(uuid);
    }
    s.visibleAll = s.visiblePlayers.empty();
    return true;
}

bool ParticleShapeManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.visiblePlayers.clear();
    it->second.visibleAll = true;
    return true;
}

bool ParticleShapeManager::setInterval(int64_t id, int ticks) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.intervalTicks = std::max(1, ticks);
    return true;
}

bool ParticleShapeManager::setViewDistance(int64_t id, int blocks) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    it->second.viewDistance = std::max(0, blocks);
    return true;
}

bool ParticleShapeManager::setLifetime(int64_t id, int ticks) {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return false;
    auto const now = currentTick();
    it->second.endTick = ticks > 0 ? now + (std::uint64_t)ticks : 0;
    return true;
}

bool ParticleShapeManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);
    return mShapes.erase(id) > 0;
}

void ParticleShapeManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mShapes.clear();
}

bool ParticleShapeManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mShapes.contains(id);
}

std::vector<int64_t> ParticleShapeManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mShapes.size());
    for (auto const& [id, s] : mShapes) {
        ids.push_back(id);
    }
    return ids;
}

std::string ParticleShapeManager::getInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto it = mShapes.find(id);
    if (it == mShapes.end()) return "";
    auto const& s = it->second;
    static char const* kindNames[] = {"point", "line", "rect", "plane", "box", "boxFaces", "poly"};
    return std::format(
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
            : std::string{}
    );
}

// ── tick 驱动 ──

void ParticleShapeManager::tick() {
    auto const now = currentTick();
    std::vector<int64_t> due;
    {
        std::lock_guard lock(mMutex);
        if (mShapes.empty()) return;
        for (auto it = mShapes.begin(); it != mShapes.end();) {
            auto& s = it->second;
            if (s.endTick != 0 && now >= s.endTick) {
                it = mShapes.erase(it);
                continue;
            }
            // 自旋累积（每 tick, 与发射节奏解耦）
            if (s.spinX != 0.0f || s.spinY != 0.0f || s.spinZ != 0.0f) {
                s.rotX += s.spinX;
                s.rotY += s.spinY;
                s.rotZ += s.spinZ;
            }
            // moveTo 平滑移动步进（easeOutCubic 插值锚点）
            if (s.animActive) {
                std::uint64_t elapsed = now - s.animStartTick;
                if (elapsed >= (std::uint64_t)s.animDurationTicks) {
                    s.px = s.animToX; s.py = s.animToY; s.pz = s.animToZ;
                    s.animActive = false;
                } else {
                    float t = (float)elapsed / (float)s.animDurationTicks;
                    t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // easeOutCubic
                    s.px = s.animFromX + (s.animToX - s.animFromX) * t;
                    s.py = s.animFromY + (s.animToY - s.animFromY) * t;
                    s.pz = s.animFromZ + (s.animToZ - s.animFromZ) * t;
                }
            }
            if (now >= s.nextEmitTick) due.push_back(it->first);
            ++it;
        }
    }
    if (due.empty()) return;

    for (auto const& id : due) {
        std::lock_guard lock(mMutex);
        auto it = mShapes.find(id);
        if (it == mShapes.end()) continue;
        auto& s = it->second;
        if (emitShape(s, now)) {
            s.nextEmitTick = now + (std::uint64_t)s.intervalTicks;
        } else {
            mShapes.erase(it);
        }
    }
}

// 预序列化帧模板: 一次完整序列化（marker 坐标）→ 扫描定位坐标偏移。
// 此后每粒子只补丁 12 字节浮点坐标, 静态部分（维度/actorId/效果名/Molang）
// 永不重复序列化。setEffect / 跟随跨维度时置 framePosOff=-1 失效重建。
bool ParticleShapeManager::buildFrameTemplate(Shape& s) {
    auto packet = MinecraftPackets::createPacket(MinecraftPacketIds::SpawnParticleEffect);
    if (!packet) return false;
    auto* spe = static_cast<SpawnParticleEffectPacket*>(packet.get());
    spe->mVanillaDimensionId = (uchar)s.dimId;
    spe->mActorId            = ::ActorUniqueID{-1};
    spe->mEffectName         = s.effect;
    spe->mMolangVariables    = std::nullopt;
    // marker 坐标: 三段特殊浮点位型, 效果名(ASCII)中不可能出现
    constexpr float mkX = -98765.5f, mkY = 87654.25f, mkZ = -76543.125f;
    spe->mPos = ::Vec3{mkX, mkY, mkZ};

    BinaryStream body;
    spe->write(body);
    float const marker[3] = {mkX, mkY, mkZ};
    std::string const markerStr{reinterpret_cast<char const*>(marker), sizeof marker};
    auto off = body.mBuffer.find(markerStr);
    if (off == std::string::npos) return false;

    // varuint 包头（pid | 发送者/接收者子客户端位全 0）
    std::string head;
    std::uint32_t v = static_cast<std::uint32_t>(spe->getId());
    while (v & ~0x7Fu) {
        head.push_back((char)((v & 0x7F) | 0x80));
        v >>= 7;
    }
    head.push_back((char)v);

    s.frameTpl.assign(head);
    s.frameTpl += body.mBuffer;
    s.framePosOff = head.size() + off;
    return true;
}

bool ParticleShapeManager::emitShape(Shape& s, std::uint64_t now) {
    auto level = ll::service::getLevel();
    if (!level) return true; // level 未就绪: 保留待发

    // ── 目标玩家（白名单或维度全员）──
    std::vector<Player*> targets;
    auto collectDim = [&](int dim) {
        level->forEachPlayer([&](Player& pl) -> bool {
            if (pl.getDimensionId().id == dim) targets.push_back(&pl);
            return true;
        });
    };
    if (s.visibleAll) {
        collectDim(s.dimId);
    } else {
        for (auto const& uuidStr : s.visiblePlayers) {
            auto* pl = level->getPlayer(mce::UUID::fromString(uuidStr));
            if (pl && pl->getDimensionId().id == s.dimId) targets.push_back(pl);
        }
    }
    // 全员可见但无人在该维度 → 空转保留（等人来）; 白名单模式无有效目标 → 移除
    if (targets.empty()) return s.visibleAll;

    // ── 跟随: 玩家位置覆盖锚点（跟随目标不在目标列表也没关系, 锚点独立解析）──
    float anchorX = s.px, anchorY = s.py, anchorZ = s.pz;
    if (!s.followUuidStr.empty()) {
        auto* follower = level->getPlayer(s.followUuid);
        if (follower == nullptr) return true; // 跟随目标离线: 空转保留
        auto const& fpos = follower->getPosition();
        anchorX = (float)fpos.x + s.offX;
        anchorY = (float)fpos.y + s.offY;
        anchorZ = (float)fpos.z + s.offZ;
        // 跟随时形状维度 = 跟随者维度（跨维度自动跟随）
        if (follower->getDimensionId().id != s.dimId) {
            s.dimId = follower->getDimensionId().id;
            s.framePosOff = static_cast<std::size_t>(-1); // 维度字节变化 → 帧模板重建
            targets.clear();
            collectDim(s.dimId);
            if (s.visibleAll && targets.empty()) return true;
        }
    }

    // ── 采样缓存 ──
    if (!s.cacheValid || s.localPts.empty()) sampleLocal(s);
    auto const& pts = s.localPts;
    if (pts.empty()) return true;

    // ── 变换（欧拉 ZYX + 缩放 + 平移）──
    float m[9];
    eulerToMatrix(s.rotX, s.rotY, s.rotZ, m);
    std::vector<float> world(pts.size());
    for (size_t i = 0; i + 2 < pts.size(); i += 3) {
        float x, y, z;
        applyMatrix(m, pts[i] * s.scale, pts[i + 1] * s.scale, pts[i + 2] * s.scale, x, y, z);
        world[i] = x + anchorX;
        world[i + 1] = y + anchorY;
        world[i + 2] = z + anchorZ;
    }
    size_t const n = pts.size() / 3;

    // 发送
    if (s.framePosOff == static_cast<std::size_t>(-1) && !buildFrameTemplate(s)) return true;

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem) return true;

    static thread_local std::string frame; // 复用缓冲: 粒子间只改 12 字节
    frame = s.frameTpl;
    auto patchAndSend = [&](NetworkPeer* peer, size_t i) {
        std::memcpy(&frame[s.framePosOff],     &world[i * 3],     4);
        std::memcpy(&frame[s.framePosOff + 4], &world[i * 3 + 1], 4);
        std::memcpy(&frame[s.framePosOff + 8], &world[i * 3 + 2], 4);
        peer->sendPacket(frame, NetworkPeer::Reliability::Reliable, Compressibility::Compressible);
    };

    for (auto* player : targets) {
        auto* peer = networkSystem->getPeerForUser(player->getNetworkIdentifier());
        if (peer == nullptr) continue; // 连接失效: 跳过该玩家
        if (s.viewDistance > 0) {
            auto const& ppos = player->getPosition();
            double const px = ppos.x, py = ppos.y, pz = ppos.z;
            double const r2 = (double)s.viewDistance * (double)s.viewDistance;
            for (size_t i = 0; i < n; i++) {
                double dx = world[i * 3] - px, dy = world[i * 3 + 1] - py, dz = world[i * 3 + 2] - pz;
                if (dx * dx + dy * dy + dz * dz > r2) continue;
                patchAndSend(peer, i);
            }
        } else {
            for (size_t i = 0; i < n; i++) {
                patchAndSend(peer, i);
            }
        }
    }
    return true;
}


LL_TYPE_INSTANCE_HOOK(ParticleShapeTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    ParticleShapeManager::getInstance().tick();
}

static ll::memory::HookRegistrar<ParticleShapeTickHook> gParticleShapeTickHookRegistrar;

} // namespace debugshape_export
