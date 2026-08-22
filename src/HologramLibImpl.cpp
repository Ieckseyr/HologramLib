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
#include "lse/LseBridge.h"

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
};

class HologramLibImpl final : public IHologramLib {
public:
    IShapeDrawer&  shapes() override { return mShapes; }
    IHologramText& holograms() override { return mHolograms; }
    IItemDetail&   itemDetails() override { return mItemDetails; }

    bool isLseAvailable() override { return lse::isAttached(); }

    uint32_t version() override { return 0x010600; } // 1.6.0

    IItemDisplay& itemDisplays() override { return mItemDisplays; }

private:
    ShapeDrawerImpl  mShapes;
    HologramTextImpl mHolograms;
    ItemDetailImpl   mItemDetails;
    ItemDisplayImpl  mItemDisplays;
};

} // namespace

HOLOGLIB_API IHologramLib& IHologramLib::getInstance() {
    static HologramLibImpl instance;
    return instance;
}

} // namespace hologramlib
