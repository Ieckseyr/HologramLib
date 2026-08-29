// HologramLibImpl.cpp - 对外 C++ 接口实现（委托各 Manager 单例）
//
// HOLOGLIB_EXPORTS 定义于 xmake（仅本 target）,
// 编译时 IHologramLib::getInstance 以 dllexport 导出,
// 消费插件经 HologramLib.lib 链接调用。
#include "hologramlib/HologramLib.h"

#include "FloatingTextManager.h"
#include "PacketDebugRenderer.h"
#include "itemdetail/ItemDetailManager.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "customentity/CustomEntityManager.h"
#include "particles/ParticleShapeManager.h"
#include "playernpc/PlayerNpcExporter.h" // playerNpcAdapter()
#include "ghost/GhostInteractRouter.h"
// 单元合并：ghost/GhostInteractRouter.cpp 并入本编译单元
// （沙箱内无法重配置 xmake.lua 注册新源文件；脱离沙箱后可拆回 add_files("src/ghost/GhostInteractRouter.cpp")）
#include "ghost/GhostInteractRouter.cpp"
#include "lse/LseBridge.h"

#include <format>
#include <utility>

namespace hologramlib {

namespace {

class ShapeDrawerImpl final : public IShapeDrawer {
public:
    int64_t createText(float x, float y, float z, std::string const& text) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createText(x, y, z, text);
    }
    int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createLine(x1, y1, z1, x2, y2, z2);
    }
    int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createBox(x1, y1, z1, x2, y2, z2);
    }
    int64_t createCircle(float x, float y, float z, float scale) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createCircle(x, y, z, scale);
    }
    int64_t createSphere(float x, float y, float z, float scale) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createSphere(x, y, z, scale);
    }
    int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2) override {
        return debugshape_export::PacketDebugRenderer::getInstance().createArrow(x1, y1, z1, x2, y2, z2);
    }

    bool setColor(int64_t id, float r, float g, float b, float a) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setColor(id, r, g, b, a);
    }
    bool setScale(int64_t id, float scale) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setScale(id, scale);
    }
    bool setDuration(int64_t id, float seconds) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setDuration(id, seconds);
    }
    bool setDimension(int64_t id, int dimId) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setDimension(id, dimId);
    }
    bool setLocation(int64_t id, float x, float y, float z) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setLocation(id, x, y, z);
    }
    bool setText(int64_t id, std::string const& text) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setText(id, text);
    }
    bool setRotation(int64_t id, float pitch, float yaw, float roll) override {
        return debugshape_export::PacketDebugRenderer::getInstance().setRotation(id, pitch, yaw, roll);
    }
    bool clearRotation(int64_t id) override {
        return debugshape_export::PacketDebugRenderer::getInstance().clearRotation(id);
    }

    bool draw(int64_t id) override { return debugshape_export::PacketDebugRenderer::getInstance().draw(id); }
    bool drawToPlayer(int64_t id, std::string const& playerName) override {
        return debugshape_export::PacketDebugRenderer::getInstance().drawToPlayer(id, playerName);
    }
    bool drawToDimension(int64_t id, int dimId) override {
        return debugshape_export::PacketDebugRenderer::getInstance().drawToDimension(id, dimId);
    }
    bool remove(int64_t id) override { return debugshape_export::PacketDebugRenderer::getInstance().remove(id); }
    bool update(int64_t id) override { return debugshape_export::PacketDebugRenderer::getInstance().update(id); }

    bool destroy(int64_t id) override {
        return debugshape_export::PacketDebugRenderer::getInstance().destroy(id);
    }
    void destroyAll() override { debugshape_export::PacketDebugRenderer::getInstance().destroyAll(); }
    bool exists(int64_t id) override { return debugshape_export::PacketDebugRenderer::getInstance().exists(id); }
    ShapeType type(int64_t id) override {
        return static_cast<ShapeType>(debugshape_export::PacketDebugRenderer::getInstance().getShapeType(id));
    }
};

class HologramTextImpl final : public IHologramText {
public:
    int64_t create(float x, float y, float z) override {
        return debugshape_export::FloatingTextManager::getInstance().create(x, y, z);
    }
    bool destroy(int64_t id) override {
        return debugshape_export::FloatingTextManager::getInstance().destroy(id);
    }
    void destroyAll() override { debugshape_export::FloatingTextManager::getInstance().destroyAll(); }

    bool addLine(int64_t id, std::string const& text) override {
        return debugshape_export::FloatingTextManager::getInstance().addLine(id, text);
    }
    bool setLineText(int64_t id, int lineIndex, std::string const& text) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineText(id, lineIndex, text);
    }
    bool setLineScale(int64_t id, int lineIndex, float scale) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineScale(id, lineIndex, scale);
    }
    bool removeLine(int64_t id, int lineIndex) override {
        return debugshape_export::FloatingTextManager::getInstance().removeLine(id, lineIndex);
    }
    bool clearLines(int64_t id) override {
        return debugshape_export::FloatingTextManager::getInstance().clearLines(id);
    }
    int getLineCount(int64_t id) override {
        return debugshape_export::FloatingTextManager::getInstance().getLineCount(id);
    }

    bool setColor(int64_t id, float r, float g, float b, float a) override {
        return debugshape_export::FloatingTextManager::getInstance().setColor(id, r, g, b, a);
    }
    bool setLineColor(int64_t id, int lineIndex, float r, float g, float b, float a) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineColor(id, lineIndex, r, g, b, a);
    }
    bool setLineGradient(
        int64_t id,
        int     lineIndex,
        float   r1,
        float   g1,
        float   b1,
        float   r2,
        float   g2,
        float   b2
    ) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineGradient(
            id,
            lineIndex,
            r1,
            g1,
            b1,
            r2,
            g2,
            b2
        );
    }
    bool setLineRainbow(int64_t id, int lineIndex, float speed) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineRainbow(id, lineIndex, speed);
    }

    bool setLineScroll(int64_t id, int lineIndex, int direction, float speed) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineScroll(id, lineIndex, direction, speed);
    }
    bool setVerticalAnimation(int64_t id, int type, float speed, float range) override {
        return debugshape_export::FloatingTextManager::getInstance().setVerticalAnimation(id, type, speed, range);
    }
    bool setLineSpacing(int64_t id, float spacing) override {
        return debugshape_export::FloatingTextManager::getInstance().setLineSpacing(id, spacing);
    }

    bool setLocation(int64_t id, float x, float y, float z) override {
        return debugshape_export::FloatingTextManager::getInstance().setLocation(id, x, y, z);
    }
    bool setFollowPlayer(int64_t id, std::string const& playerName, float offsetY) override {
        return debugshape_export::FloatingTextManager::getInstance().setFollowPlayer(id, playerName, offsetY);
    }
    bool clearFollowPlayer(int64_t id) override {
        return debugshape_export::FloatingTextManager::getInstance().clearFollowPlayer(id);
    }

    bool draw(int64_t id) override { return debugshape_export::FloatingTextManager::getInstance().draw(id); }
    bool drawToDimension(int64_t id, int dimId) override {
        return debugshape_export::FloatingTextManager::getInstance().drawToDimension(id, dimId);
    }
    bool drawToPlayer(int64_t id, std::string const& playerName) override {
        return debugshape_export::FloatingTextManager::getInstance().drawToPlayer(id, playerName);
    }
    bool remove(int64_t id) override { return debugshape_export::FloatingTextManager::getInstance().remove(id); }
    bool refresh(int64_t id) override { return debugshape_export::FloatingTextManager::getInstance().refresh(id); }

    void tick(float deltaTime) override {
        debugshape_export::FloatingTextManager::getInstance().tick(deltaTime);
    }

    bool setDimension(int64_t id, int dimId) override {
        return debugshape_export::FloatingTextManager::getInstance().setDimension(id, dimId);
    }
};

class ItemDetailImpl final : public IItemDetail {
public:
    int64_t show(
        int                dimId,
        float              x,
        float              y,
        float              z,
        std::string const& itemId,
        int                aux,
        int                count,
        std::string const& customText
    ) override {
        return debugshape_export::ItemDetailManager::getInstance().show(dimId, x, y, z, itemId, aux, count, customText);
    }
    bool hide(int64_t id) override { return debugshape_export::ItemDetailManager::getInstance().hide(id); }
};

class ItemDisplayImpl final : public IItemDisplay {
public:
    int64_t create(ItemDisplayConfig const& config) override {
        return debugshape_export::ItemDisplayManager::getInstance().create(config);
    }
    bool destroy(int64_t id) override {
        return debugshape_export::ItemDisplayManager::getInstance().destroy(id);
    }
    void destroyAll() override { debugshape_export::ItemDisplayManager::getInstance().destroyAll(); }
    bool exists(int64_t id) const override {
        return debugshape_export::ItemDisplayManager::getInstance().exists(id);
    }
    bool get(int64_t id, ItemDisplayConfig& out) const override {
        return debugshape_export::ItemDisplayManager::getInstance().get(id, out);
    }
    bool setItem(int64_t id, std::string const& item, int aux) override {
        return debugshape_export::ItemDisplayManager::getInstance().setItem(id, item, aux);
    }
    bool setPosition(int64_t id, float x, float y, float z, int dim) override {
        return debugshape_export::ItemDisplayManager::getInstance().setPosition(id, x, y, z, dim);
    }
    bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) override {
        return debugshape_export::ItemDisplayManager::getInstance().setOffset(id, ox, oy, oz);
    }
    bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) override {
        return debugshape_export::ItemDisplayManager::getInstance().setBaseOffset(id, ox, oy, oz);
    }
    bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) override {
        return debugshape_export::ItemDisplayManager::getInstance().setRotation(id, rx, ry, rz);
    }
    bool setScale(int64_t id, std::string const& scale) override {
        return debugshape_export::ItemDisplayManager::getInstance().setScale(id, scale);
    }
    bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) override {
        return debugshape_export::ItemDisplayManager::getInstance().setExtend(id, scale, rx, ry);
    }
    bool setMode(int64_t id, int mode) override {
        return debugshape_export::ItemDisplayManager::getInstance().setMode(id, mode);
    }
    bool setEnabled(int64_t id, bool enabled) override {
        return debugshape_export::ItemDisplayManager::getInstance().setEnabled(id, enabled);
    }
    bool setViewDistance(int64_t id, double dist) override {
        return debugshape_export::ItemDisplayManager::getInstance().setViewDistance(id, dist);
    }
    bool rotateY(int64_t id, float delta) override {
        return debugshape_export::ItemDisplayManager::getInstance().rotateY(id, delta);
    }
    std::vector<int64_t> getAllIds() const override {
        return debugshape_export::ItemDisplayManager::getInstance().getAllIds();
    }
    int64_t createRandom(ItemDisplayConfig const& config) override {
        return debugshape_export::ItemDisplayManager::getInstance().createRandom(config);
    }
    int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId) override {
        return debugshape_export::ItemDisplayManager::getInstance().createWithId(config, desiredId);
    }
    bool isIdUsed(int64_t id) const override {
        return debugshape_export::ItemDisplayManager::getInstance().isIdUsed(id);
    }
    bool scaleBy(int64_t id, double factor) override {
        return debugshape_export::ItemDisplayManager::getInstance().scaleBy(id, factor);
    }
    bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt) override {
        return debugshape_export::ItemDisplayManager::getInstance().setItemWithNbt(id, item, aux, nbt);
    }

    bool setGlint(int64_t id, bool on) override {
        return debugshape_export::ItemDisplayManager::getInstance().setGlint(id, on);
    }
    // 1.17.0: display follow / hitbox glue
    bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ) override {
        return debugshape_export::ItemDisplayManager::getInstance().follow(id, playerName, offX, offY, offZ);
    }
    bool unfollow(int64_t id) override {
        return debugshape_export::ItemDisplayManager::getInstance().unfollow(id);
    }
    bool setHitbox(int64_t id, float width, float height) override {
        return debugshape_export::ItemDisplayManager::getInstance().setHitbox(id, width, height);
    }
};



class CustomEntityImpl final : public ICustomEntity {
public:
    int64_t create(CustomEntityConfig const& config) override {
        return debugshape_export::CustomEntityManager::getInstance().create(config);
    }
    int64_t createRandom(CustomEntityConfig const& config) override {
        return debugshape_export::CustomEntityManager::getInstance().createRandom(config);
    }
    int64_t createWithId(CustomEntityConfig const& config, int64_t desiredId) override {
        return debugshape_export::CustomEntityManager::getInstance().createWithId(config, desiredId);
    }
    bool destroy(int64_t id) override {
        return debugshape_export::CustomEntityManager::getInstance().destroy(id);
    }
    void destroyAll() override { debugshape_export::CustomEntityManager::getInstance().destroyAll(); }
    bool exists(int64_t id) const override {
        return debugshape_export::CustomEntityManager::getInstance().exists(id);
    }
    bool get(int64_t id, CustomEntityConfig& out) const override {
        return debugshape_export::CustomEntityManager::getInstance().get(id, out);
    }
    bool isIdUsed(int64_t id) const override {
        return debugshape_export::CustomEntityManager::getInstance().isIdUsed(id);
    }
    std::vector<int64_t> getAllIds() const override {
        return debugshape_export::CustomEntityManager::getInstance().getAllIds();
    }
    bool setIdentifier(int64_t id, std::string const& identifier) override {
        return debugshape_export::CustomEntityManager::getInstance().setIdentifier(id, identifier);
    }
    bool setPosition(int64_t id, float x, float y, float z, int dim) override {
        return debugshape_export::CustomEntityManager::getInstance().setPosition(id, x, y, z, dim);
    }
    bool setRotation(int64_t id, float yaw, float pitch) override {
        return debugshape_export::CustomEntityManager::getInstance().setRotation(id, yaw, pitch);
    }
    bool setNametag(int64_t id, std::string const& text) override {
        return debugshape_export::CustomEntityManager::getInstance().setNametag(id, text);
    }
    bool setScale(int64_t id, float scale) override {
        return debugshape_export::CustomEntityManager::getInstance().setScale(id, scale);
    }
    bool setVariant(int64_t id, int variant) override {
        return debugshape_export::CustomEntityManager::getInstance().setVariant(id, variant);
    }
    bool setMarkVariant(int64_t id, int markVariant) override {
        return debugshape_export::CustomEntityManager::getInstance().setMarkVariant(id, markVariant);
    }
    bool setColorIndex(int64_t id, int colorIndex) override {
        return debugshape_export::CustomEntityManager::getInstance().setColorIndex(id, colorIndex);
    }
    bool setFlags(int64_t id, std::int64_t flags) override {
        return debugshape_export::CustomEntityManager::getInstance().setFlags(id, flags);
    }
    bool setInvisible(int64_t id, bool on) override {
        return debugshape_export::CustomEntityManager::getInstance().setInvisible(id, on);
    }
    bool setEnabled(int64_t id, bool enabled) override {
        return debugshape_export::CustomEntityManager::getInstance().setEnabled(id, enabled);
    }
    bool setViewDistance(int64_t id, double dist) override {
        return debugshape_export::CustomEntityManager::getInstance().setViewDistance(id, dist);
    }
    bool setPose(int64_t id, int pose) override {
        return debugshape_export::CustomEntityManager::getInstance().setPose(id, pose);
    }
    bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) override {
        return debugshape_export::CustomEntityManager::getInstance().setEquipmentSlot(id, slot, name, aux, nbt);
    }
    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const override {
        return debugshape_export::CustomEntityManager::getInstance().findNearest(x, y, z, dim, maxDist);
    }
    bool scaleBy(int64_t id, double factor) override {
        return debugshape_export::CustomEntityManager::getInstance().scaleBy(id, factor);
    }
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) override {
        return debugshape_export::CustomEntityManager::getInstance().setVisiblePlayers(id, playerNames);
    }
    bool clearVisiblePlayers(int64_t id) override {
        return debugshape_export::CustomEntityManager::getInstance().clearVisiblePlayers(id);
    }
    bool setVisiblePlayer(int64_t id, std::string const& playerName) override {
        return debugshape_export::CustomEntityManager::getInstance().setVisiblePlayer(id, playerName);
    }
    std::string getDebugInfo(int64_t id) const override {
        return debugshape_export::CustomEntityManager::getInstance().getDebugInfo(id);
    }
    bool setRidePlayer(int64_t id, std::string const& playerName) override {
        return debugshape_export::CustomEntityManager::getInstance().setRidePlayer(id, playerName);
    }
    bool setRideEntity(int64_t id, int64_t vehicleEntityId) override {
        return debugshape_export::CustomEntityManager::getInstance().setRideEntity(id, vehicleEntityId);
    }
    bool clearRide(int64_t id) override {
        return debugshape_export::CustomEntityManager::getInstance().clearRide(id);
    }
    bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks
    ) override {
        return debugshape_export::CustomEntityManager::getInstance().playAnimation(
            id, animation, stopExpression, durationTicks
        );
    }
};
class ParticleShapeImpl final : public IParticleShape {
public:
    int64_t createPoint(
        std::string const& owner, int dimId,
        float x, float y, float z,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createPoint(
            owner, dimId, x, y, z, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createLine(
        std::string const& owner, int dimId,
        float x1, float y1, float z1, float x2, float y2, float z2, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createLine(
            owner, dimId, x1, y1, z1, x2, y2, z2, step, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createRect(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createRect(
            owner, dimId, cx, cy, cz, w, h, axis, step, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createPlane(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createPlane(
            owner, dimId, cx, cy, cz, w, h, axis, step, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createBox(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createBox(
            owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createBoxFaces(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createBoxFaces(
            owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, intervalTicks, lifetimeTicks
        );
    }
    int64_t createPoly(
        std::string const& owner, int dimId,
        std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    ) override {
        return debugshape_export::ParticleShapeManager::getInstance().createPoly(
            owner, dimId, verts, edges, step, effect, intervalTicks, lifetimeTicks
        );
    }

    bool setPos(int64_t id, float x, float y, float z) override {
        return debugshape_export::ParticleShapeManager::getInstance().setPos(id, x, y, z);
    }
    bool moveBy(int64_t id, float dx, float dy, float dz) override {
        return debugshape_export::ParticleShapeManager::getInstance().moveBy(id, dx, dy, dz);
    }
    bool setRot(int64_t id, float rx, float ry, float rz) override {
        return debugshape_export::ParticleShapeManager::getInstance().setRot(id, rx, ry, rz);
    }
    bool spin(int64_t id, float sx, float sy, float sz) override {
        return debugshape_export::ParticleShapeManager::getInstance().spin(id, sx, sy, sz);
    }
    bool setScale(int64_t id, float scale) override {
        return debugshape_export::ParticleShapeManager::getInstance().setScale(id, scale);
    }
    bool follow(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) override {
        return debugshape_export::ParticleShapeManager::getInstance().follow(id, playerUuid, offX, offY, offZ);
    }
    bool unfollow(int64_t id) override {
        return debugshape_export::ParticleShapeManager::getInstance().unfollow(id);
    }

    bool setEffect(int64_t id, std::string const& effect) override {
        return debugshape_export::ParticleShapeManager::getInstance().setEffect(id, effect);
    }
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids) override {
        return debugshape_export::ParticleShapeManager::getInstance().setVisiblePlayers(id, playerUuids);
    }
    bool clearVisiblePlayers(int64_t id) override {
        return debugshape_export::ParticleShapeManager::getInstance().clearVisiblePlayers(id);
    }
    bool setInterval(int64_t id, int ticks) override {
        return debugshape_export::ParticleShapeManager::getInstance().setInterval(id, ticks);
    }
    bool setViewDistance(int64_t id, int blocks) override {
        return debugshape_export::ParticleShapeManager::getInstance().setViewDistance(id, blocks);
    }
    bool setLifetime(int64_t id, int ticks) override {
        return debugshape_export::ParticleShapeManager::getInstance().setLifetime(id, ticks);
    }

    bool destroy(int64_t id) override {
        return debugshape_export::ParticleShapeManager::getInstance().destroy(id);
    }
    void destroyAll() override { debugshape_export::ParticleShapeManager::getInstance().destroyAll(); }
    bool exists(int64_t id) const override {
        return debugshape_export::ParticleShapeManager::getInstance().exists(id);
    }
    std::vector<int64_t> getAllIds() const override {
        return debugshape_export::ParticleShapeManager::getInstance().getAllIds();
    }
    std::string getDebugInfo(int64_t id) const override {
        return debugshape_export::ParticleShapeManager::getInstance().getInfo(id);
    }

    // 1.15.0 追加（尾部）
    bool moveTo(int64_t id, float x, float y, float z, int durationTicks) override {
        return debugshape_export::ParticleShapeManager::getInstance().moveTo(id, x, y, z, durationTicks);
    }
};

class HologramLibImpl final : public IHologramLib {
public:
    IShapeDrawer&  shapes() override { return mShapes; }
    IHologramText& holograms() override { return mHolograms; }
    IItemDetail&   itemDetails() override { return mItemDetails; }

    bool isLseAvailable() override { return lse::isAttached(); }

    uint32_t version() override { return HOLOGLIB_API_VERSION; } // 跟随宏, 永不再手写数值

    IItemDisplay& itemDisplays() override { return mItemDisplays; }

    int64_t findNearestItemDisplay(float x, float y, float z, int dim, double maxDist) override {
        return debugshape_export::ItemDisplayManager::getInstance().findNearest(x, y, z, dim, maxDist);
    }

    ICustomEntity& customEntities() override { return mCustomEntities; }

    IParticleShape& particleShapes() override { return mParticleShapes; }

    IPlayerNpc& playerNpcs() override { return debugshape_export::playerNpcAdapter(); }

    void setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener) override {
        debugshape_export::GhostInteractRouter::getInstance().setListener(std::move(listener));
    }
    void clearGhostInteractListener() override {
        debugshape_export::GhostInteractRouter::getInstance().clearListener();
    }
    std::vector<std::string> pollGhostInteractions() override {
        auto events = debugshape_export::GhostInteractRouter::getInstance().poll();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events) {
            auto line = std::format(
                "player={} action={} domain={} id={}",
                ev.playerName,
                ev.action,
                ev.domain,
                ev.id
            );
            if (ev.hasPos) {
                line += std::format(" pos=({:.2f},{:.2f},{:.2f})", ev.x, ev.y, ev.z);
            }
            out.push_back(std::move(line));
        }
        return out;
    }

private:
    ShapeDrawerImpl  mShapes;
    HologramTextImpl mHolograms;
    ItemDetailImpl   mItemDetails;
    ItemDisplayImpl  mItemDisplays;
    CustomEntityImpl mCustomEntities;
    ParticleShapeImpl mParticleShapes;
};

} // namespace

HOLOGLIB_API IHologramLib& IHologramLib::getInstance() {
    static HologramLibImpl instance;
    return instance;
}

} // namespace hologramlib
