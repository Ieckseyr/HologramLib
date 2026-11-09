#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#define HOLOGLIB_API_VERSION 0x012B00

inline constexpr std::uint32_t HOLOGLIB_ABI_STAMP = 2;

// 版本号分三类，各自独立变化：
//   ABI       C++ 头文件与虚表布局，改了就要重新编译
//   LSE       脚本能 import 到的导出面
//   Endstone  Python 侧的方法
inline constexpr char const* HOLOGLIB_ABI_VERSION      = "1.37.0";
inline constexpr char const* HOLOGLIB_LSE_API_VERSION  = "1.0.0";
inline constexpr char const* HOLOGLIB_ENDSTONE_VERSION = "1.0.0";

inline constexpr char const* HOLOGLIB_ABI_STAMP_SYMBOL = "hologramlib_abiStamp";

#ifdef HOLOGLIB_EXPORTS
#define HOLOGLIB_API __declspec(dllexport)
#else
#define HOLOGLIB_API __declspec(dllimport)
#endif

namespace hologramlib {

struct PerPlayerRotation {
    float yaw{0};
    float pitch{0};
};

enum class ShapeType : int {
    Text   = 0,
    Line   = 1,
    Box    = 2,
    Circle = 3,
    Sphere = 4,
    Arrow  = 5,

    Cylinder  = 7,
    Pyramid   = 8,
    Ellipsoid = 9,
    Cone      = 10
};

// 形状的圆柱/金字塔/椭球/圆锥与通用属性
class IShapeDrawerExt {
public:
    virtual ~IShapeDrawerExt() = default;

    virtual int64_t createCylinder(
        float x,
        float y,
        float z,
        float radiusX,
        float radiusZ,
        float height,
        int   segments = 16) = 0;

    virtual int64_t createPyramid(float x, float y, float z, float width, float height, float depth) = 0;

    virtual int64_t createEllipsoid(
        float x,
        float y,
        float z,
        float radiusX,
        float radiusY,
        float radiusZ,
        int   segmentsPerAxis = 12) = 0;

    virtual int64_t createCone(
        float x,
        float y,
        float z,
        float radiusX,
        float radiusZ,
        float height,
        int   segments = 16) = 0;

    virtual bool setMaxRenderDistance(int64_t id, float distance) = 0;

    virtual bool setAttachedEntity(int64_t id, int64_t entityUniqueId) = 0;

    virtual bool clearAttachedEntity(int64_t id) = 0;
};

// 基础形状：线、方框、球、圆、文本
class IShapeDrawer {
public:
    virtual ~IShapeDrawer() = default;

    virtual int64_t createText(float x, float y, float z, std::string const& text)          = 0;
    virtual int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2)  = 0;
    virtual int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2)   = 0;
    virtual int64_t createCircle(float x, float y, float z, float scale)                    = 0;
    virtual int64_t createSphere(float x, float y, float z, float scale)                    = 0;
    virtual int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2) = 0;

    virtual bool setColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool setScale(int64_t id, float scale)                        = 0;
    virtual bool setDuration(int64_t id, float seconds)                   = 0;
    virtual bool setDimension(int64_t id, int dimId)                      = 0;
    virtual bool setLocation(int64_t id, float x, float y, float z)       = 0;
    virtual bool setText(int64_t id, std::string const& text)             = 0;

    virtual bool setRotation(int64_t id, float pitch, float yaw, float roll) = 0;
    virtual bool clearRotation(int64_t id)                                   = 0;

    virtual bool draw(int64_t id)                                        = 0;
    virtual bool drawToPlayer(int64_t id, std::string const& playerName) = 0;
    virtual bool drawToDimension(int64_t id, int dimId)                  = 0;
    virtual bool remove(int64_t id)                                      = 0;
    virtual bool update(int64_t id)                                      = 0;

    virtual bool      destroy(int64_t id) = 0;
    virtual void      destroyAll()        = 0;
    virtual bool      exists(int64_t id)  = 0;
    virtual ShapeType type(int64_t id)    = 0;

    virtual bool setBackgroundColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool clearBackgroundColor(int64_t id)                                   = 0;

    virtual bool setDepthTest(int64_t id, bool enabled) = 0;
};

// 多行悬浮字（内容池/变量/跟随/渐变）
class IHologramText {
public:
    virtual ~IHologramText() = default;

    virtual int64_t create(float x, float y, float z) = 0;
    virtual bool    destroy(int64_t id)               = 0;
    virtual void    destroyAll()                      = 0;

    virtual bool addLine(int64_t id, std::string const& text)                    = 0;
    virtual bool setLineText(int64_t id, int lineIndex, std::string const& text) = 0;

    [[deprecated("1.26.0 移除: 行级缩放从未生效; 整块缩放请用 setScale")]]
    virtual bool setLineScale(int64_t, int, float) { return false; }
    virtual bool removeLine(int64_t id, int lineIndex) = 0;
    virtual bool clearLines(int64_t id)                = 0;
    virtual int  getLineCount(int64_t id)              = 0;

    virtual bool setColor(int64_t id, float r, float g, float b, float a) = 0;
    [[deprecated("1.26.0 移除: 行级颜色无协议支撑; 行级颜色请用文本内嵌 § 颜色代码")]]
    virtual bool setLineColor(int64_t, int, float, float, float, float) {
        return false;
    }
    [[deprecated("1.26.0 移除: 行级渐变无协议支撑")]]
    virtual bool setLineGradient(
        int64_t,
        int,
        float,
        float,
        float,
        float,
        float,
        float) {
        return false;
    }
    [[deprecated("1.26.0 移除: 彩虹模式会把文本涂白")]]
    virtual bool setLineRainbow(int64_t, int, float) { return false; }
    [[deprecated("1.26.0 移除: 依赖已删除的库内自驱动画")]]
    virtual bool setLineScroll(int64_t, int, int, float) { return false; }
    [[deprecated("1.26.0 移除: 依赖已删除的库内自驱动画")]]
    virtual bool setVerticalAnimation(int64_t, int, float, float) {
        return false;
    }
    [[deprecated("1.26.0 移除: 协议 2168 无行间距字段")]]
    virtual bool setLineSpacing(int64_t, float) { return false; }

    virtual bool setLocation(int64_t id, float x, float y, float z)                        = 0;
    virtual bool setFollowPlayer(int64_t id, std::string const& playerName, float offsetY) = 0;
    virtual bool clearFollowPlayer(int64_t id)                                             = 0;

    virtual bool draw(int64_t id)                                        = 0;
    virtual bool drawToDimension(int64_t id, int dimId)                  = 0;
    virtual bool drawToPlayer(int64_t id, std::string const& playerName) = 0;
    virtual bool remove(int64_t id)                                      = 0;
    virtual bool refresh(int64_t id)                                     = 0;

    [[deprecated("1.26.0 移除: 库内自驱已删; 跟随/变量在 draw / refresh 时就地解析")]]
    virtual void tick(float) {}

    virtual bool setDimension(int64_t id, int dimId) = 0;

    virtual bool setScale(int64_t id, float scale) = 0;

    virtual bool setBackgroundColor(int64_t id, float r, float g, float b, float a) = 0;
    virtual bool clearBackgroundColor(int64_t id)                                   = 0;

    virtual bool setDepthTest(int64_t id, bool enabled) = 0;

    virtual bool setRotation(int64_t id, float pitch, float yaw, float roll) = 0;
    virtual bool clearRotation(int64_t id)                                   = 0;

    virtual bool setLinePool(int64_t id, int lineIndex, std::vector<std::string> const& content, int intervalMs) = 0;

    virtual bool setLineParseVariables(int64_t id, int lineIndex, bool enabled) = 0;
};

// 物品详情悬浮字
class IItemDetail {
public:
    virtual ~IItemDetail() = default;

    virtual int64_t show(
        int                dimId,
        float              x,
        float              y,
        float              z,
        std::string const& itemId,
        int                aux,
        int                count,
        std::string const& customText = "") = 0;

    virtual bool hide(int64_t id) = 0;
};

struct ItemDisplayConfig {
    std::string item{"minecraft:diamond"};
    int         itemAux{0};
    float       x{0}, y{64}, z{0};
    int         dimension{0};

    std::string offsetX{"0"};
    std::string offsetY{"-4"};
    std::string offsetZ{"0"};
    std::string baseOffsetX{"0"};
    std::string baseOffsetY{"0"};
    std::string baseOffsetZ{"0"};

    std::string rotX{"180"};
    std::string rotY{"0"};
    std::string rotZ{"180"};

    std::string scale{"0.375"};

    std::string extendScale{"1"};
    std::string extendRotX{"-90"};
    std::string extendRotY{"0"};

    int    mode{0};
    double viewDistance{64.0};
    bool   enabled{true};

    std::string itemNbt{};

    bool itemGlint{false};

    float hitboxWidth{0.0f};
    float hitboxHeight{0.0f};
};

struct ItemDisplayAnim {
    float spinSpeed{0.0f};
    int   spinAxis{1};
    float bobAmplitude{0.0f};
    float bobPeriod{2.0f};
    float pulseAmplitude{0.0f};
    float pulsePeriod{2.0f};
    float swayAmplitude{0.0f};
    float swayPeriod{3.0f};
    float phase{0.0f};
    bool  billboard{false};

    float orbitRadius{0.0f};
    float orbitPeriod{4.0f};
    int   orbitPlane{2};
    float tumbleSpeed{0.0f};
    float wobbleAmplitude{0.0f};
    float wobbleSpeed{1.0f};
    float heartbeatAmplitude{0.0f};
    float heartbeatPeriod{1.2f};
    float pendulumAmplitude{0.0f};
    float pendulumPeriod{3.0f};
    bool  billboardPitch{false};
    float lodNear{0.0f};
    float lodFar{0.0f};
    float lodMinScale{1.0f};
    bool  hideInFirstPerson{false};
};

// 物品展示实体（图标/缩放/动画）
class IItemDisplay {
public:
    virtual ~IItemDisplay() = default;

    virtual int64_t create(ItemDisplayConfig const& config)       = 0;
    virtual bool    destroy(int64_t id)                           = 0;
    virtual void    destroyAll()                                  = 0;
    virtual bool    exists(int64_t id) const                      = 0;
    virtual bool    get(int64_t id, ItemDisplayConfig& out) const = 0;

    virtual bool setItem(int64_t id, std::string const& item, int aux)                                          = 0;
    virtual bool setPosition(int64_t id, float x, float y, float z, int dim)                                    = 0;
    virtual bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz)     = 0;
    virtual bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) = 0;
    virtual bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz)   = 0;
    virtual bool setScale(int64_t id, std::string const& scale)                                                 = 0;
    virtual bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry)  = 0;
    virtual bool setMode(int64_t id, int mode)                                                                  = 0;
    virtual bool setEnabled(int64_t id, bool enabled)                                                           = 0;
    virtual bool setViewDistance(int64_t id, double dist)                                                       = 0;
    virtual bool rotateY(int64_t id, float delta)                                                               = 0;

    virtual std::vector<int64_t> getAllIds() const = 0;

    virtual int64_t createRandom(ItemDisplayConfig const& config) = 0;

    virtual int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId) = 0;

    virtual bool isIdUsed(int64_t id) const = 0;

    virtual bool scaleBy(int64_t id, double factor) = 0;

    virtual bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt) = 0;

    virtual bool setGlint(int64_t id, bool on) = 0;

    virtual bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ) = 0;

    virtual bool unfollow(int64_t id) = 0;

    virtual bool setHitbox(int64_t id, float width, float height) = 0;

    virtual bool setStretch(int64_t id, std::string const& xz, std::string const& y)     = 0;
    virtual bool setStretchLive(int64_t id, std::string const& xz, std::string const& y) = 0;

    virtual bool setRotationLive(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) = 0;
    virtual bool setOffsetLive(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz)   = 0;
    virtual bool setScaleLive(int64_t id, std::string const& scale)                                               = 0;

    virtual bool setTransformLive(
        int64_t            id,
        std::string const& ox, std::string const& oy, std::string const& oz,
        std::string const& rx, std::string const& ry, std::string const& rz,
        std::string const& scale) = 0;

    virtual bool setAnim(int64_t id, ItemDisplayAnim const& anim) = 0;

    virtual bool setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c) = 0;

    virtual bool clearAnim(int64_t id) = 0;
};

struct CustomEntityEquipment {
    std::string name;
    int         aux{0};
    std::string nbt{};
};

struct EntityMobProperty {
    std::string name;
    std::string value;
};

struct CustomEntityConfig {
    std::string  identifier{"minecraft:armor_stand"};
    float        x{0}, y{64}, z{0};
    int          dimension{0};
    float        yaw{0}, pitch{0};
    std::string  nametag{};
    bool         nametagAlwaysShow{false};
    float        scale{1.0f};
    int          variant{0};
    int          markVariant{0};
    int          colorIndex{0};
    std::int64_t flags{0};
    bool         invisible{false};
    double       viewDistance{64.0};
    bool         enabled{true};
    int          pose{0};

    CustomEntityEquipment equipment[6];

    std::string ridePlayerName{};
    int64_t     rideEntityId{0};

    std::vector<EntityMobProperty> mobProperties;
};

// 自定义实体（模型/动画/装备/骑乘）
class ICustomEntity {
public:
    virtual ~ICustomEntity() = default;

    virtual int64_t              create(CustomEntityConfig const& config)                          = 0;
    virtual int64_t              createRandom(CustomEntityConfig const& config)                    = 0;
    virtual int64_t              createWithId(CustomEntityConfig const& config, int64_t desiredId) = 0;
    virtual bool                 destroy(int64_t id)                                               = 0;
    virtual void                 destroyAll()                                                      = 0;
    virtual bool                 exists(int64_t id) const                                          = 0;
    virtual bool                 get(int64_t id, CustomEntityConfig& out) const                    = 0;
    virtual bool                 isIdUsed(int64_t id) const                                        = 0;
    virtual std::vector<int64_t> getAllIds() const                                                 = 0;

    virtual bool setIdentifier(int64_t id, std::string const& identifier)    = 0;
    virtual bool setPosition(int64_t id, float x, float y, float z, int dim) = 0;
    virtual bool setRotation(int64_t id, float yaw, float pitch)             = 0;
    virtual bool setNametag(int64_t id, std::string const& text)             = 0;
    virtual bool setScale(int64_t id, float scale)                           = 0;
    virtual bool setVariant(int64_t id, int variant)                         = 0;
    virtual bool setMarkVariant(int64_t id, int markVariant)                 = 0;
    virtual bool setColorIndex(int64_t id, int colorIndex)                   = 0;
    virtual bool setFlags(int64_t id, std::int64_t flags)                    = 0;
    virtual bool setInvisible(int64_t id, bool on)                           = 0;
    virtual bool setEnabled(int64_t id, bool enabled)                        = 0;
    virtual bool setViewDistance(int64_t id, double dist)                    = 0;

    virtual bool setPose(int64_t id, int pose) = 0;

    virtual bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) = 0;

    virtual bool setMobProperty(int64_t id, std::string const& name, std::string const& value) = 0;
    virtual bool clearMobProperties(int64_t id)                                                = 0;

    virtual int64_t findNearest(float x, float y, float z, int dim, double maxDist) const = 0;

    virtual bool scaleBy(int64_t id, double factor) = 0;

    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) = 0;
    virtual bool clearVisiblePlayers(int64_t id)                                            = 0;
    virtual bool setVisiblePlayer(int64_t id, std::string const& playerName)                = 0;

    virtual std::string getDebugInfo(int64_t id) const = 0;

    virtual bool setRidePlayer(int64_t id, std::string const& playerName) = 0;
    virtual bool setRideEntity(int64_t id, int64_t vehicleEntityId)       = 0;
    virtual bool clearRide(int64_t id)                                    = 0;

    virtual bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) = 0;

    virtual bool playAnimationTo(
        int64_t id, std::string const& playerName,
        std::string const& animation, std::string const& stopExpression, int durationTicks) = 0;

    using EntitySpawnCallback                                         = std::function<void(int64_t id, std::string const& playerName)>;
    virtual void setEntitySpawnCallback(EntitySpawnCallback callback) = 0;

    virtual bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw, float pitch) = 0;

    virtual bool clearPlayerRotation(int64_t id, std::string const& playerName) = 0;

    virtual bool clearPlayerRotations(int64_t id) = 0;

    virtual bool setPlayerNametag(int64_t id, std::string const& playerName, std::string const& text) = 0;

    virtual bool setPlayerScale(int64_t id, std::string const& playerName, float scale) = 0;

    virtual bool setPlayerEquipmentSlot(
        int64_t id, std::string const& playerName, int slot, std::string const& name, int aux, std::string const& nbt) = 0;

    virtual bool clearPlayerAppearance(int64_t id, std::string const& playerName) = 0;

    virtual bool stopAnimation(int64_t id)                                  = 0;
    virtual bool stopAnimationTo(int64_t id, std::string const& playerName) = 0;
};

// 粒子形状（点/线/面/盒/多边形）
class IParticleShape {
public:
    virtual ~IParticleShape() = default;

    virtual int64_t createPoint(
        std::string const& owner, int dimId,
        float x, float y, float z,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;
    virtual int64_t createLine(
        std::string const& owner, int dimId,
        float x1, float y1, float z1, float x2, float y2, float z2, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;

    virtual int64_t createRect(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;
    virtual int64_t createPlane(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;

    virtual int64_t createBox(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;
    virtual int64_t createBoxFaces(
        std::string const& owner, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;

    virtual int64_t createPoly(
        std::string const& owner, int dimId,
        std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks) = 0;

    virtual bool setPos(int64_t id, float x, float y, float z)    = 0;
    virtual bool moveBy(int64_t id, float dx, float dy, float dz) = 0;

    virtual bool setRot(int64_t id, float rx, float ry, float rz) = 0;

    virtual bool spin(int64_t id, float sx, float sy, float sz) = 0;
    virtual bool setScale(int64_t id, float scale)              = 0;

    virtual bool follow(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) = 0;
    virtual bool unfollow(int64_t id)                                                                  = 0;

    virtual bool setEffect(int64_t id, std::string const& effect) = 0;

    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids) = 0;
    virtual bool clearVisiblePlayers(int64_t id)                                            = 0;
    virtual bool setInterval(int64_t id, int ticks)                                         = 0;
    virtual bool setViewDistance(int64_t id, int blocks)                                    = 0;
    virtual bool setLifetime(int64_t id, int ticks)                                         = 0;

    virtual bool                 destroy(int64_t id)      = 0;
    virtual void                 destroyAll()             = 0;
    virtual bool                 exists(int64_t id) const = 0;
    virtual std::vector<int64_t> getAllIds() const        = 0;

    virtual std::string getDebugInfo(int64_t id) const = 0;

    virtual bool moveTo(int64_t id, float x, float y, float z, int durationTicks) = 0;
};

struct ActorInteractEvent {
    std::string   playerName;
    std::string   targetType;
    int           action{0};
    std::uint64_t runtimeId{0};
    std::int64_t  uniqueId{0};
    bool          hasPos{false};
    float         x{0}, y{0}, z{0};
};

struct GhostInteractEvent {
    std::string playerName;
    int         action{0};
    std::string domain;
    int64_t     id{-1};
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};

struct PlayerNpcSkin {
    std::string pngPath{};
    std::string skinId{};

    std::string geometry{"geometry.humanoid.custom"};

    std::string armSize{"wide"};

    std::string geometryData{};
};

struct PlayerNpcConfig {
    std::string name{"NPC"};
    std::string skinId{"default"};
    float       x{0}, y{64}, z{0};
    int         dimension{0};
    float       yaw{0};
    double      viewDistance{96.0};
    float       scale{1.0f};
    bool        enabled{true};
};

// 假玩家 NPC（皮肤/姿势/装备/骑乘）
class IPlayerNpc {
public:
    virtual ~IPlayerNpc() = default;

    virtual bool registerSkin(PlayerNpcSkin const& skin) = 0;

    virtual bool                     captureSkin(std::string const& skinId, std::string const& playerName) = 0;
    virtual bool                     hasSkin(std::string const& skinId) const                              = 0;
    virtual bool                     unregisterSkin(std::string const& skinId)                             = 0;
    virtual std::vector<std::string> getSkinIds() const                                                    = 0;

    virtual int64_t              create(PlayerNpcConfig const& config)                          = 0;
    virtual int64_t              createRandom(PlayerNpcConfig const& config)                    = 0;
    virtual int64_t              createWithId(PlayerNpcConfig const& config, int64_t desiredId) = 0;
    virtual bool                 destroy(int64_t id)                                            = 0;
    virtual void                 destroyAll()                                                   = 0;
    virtual bool                 exists(int64_t id) const                                       = 0;
    virtual bool                 get(int64_t id, PlayerNpcConfig& out) const                    = 0;
    virtual bool                 isIdUsed(int64_t id) const                                     = 0;
    virtual std::vector<int64_t> getAllIds() const                                              = 0;

    virtual bool setPosition(int64_t id, float x, float y, float z, int dim) = 0;
    virtual bool setRotation(int64_t id, float yaw)                          = 0;
    virtual bool setNametag(int64_t id, std::string const& text)             = 0;

    virtual bool setSkin(int64_t id, std::string const& skinId) = 0;
    virtual bool setViewDistance(int64_t id, double dist)       = 0;
    virtual bool setScale(int64_t id, float scale)              = 0;
    virtual bool setEnabled(int64_t id, bool enabled)           = 0;

    virtual bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) = 0;
    virtual bool clearVisiblePlayers(int64_t id)                                            = 0;
    virtual bool setVisiblePlayer(int64_t id, std::string const& playerName)                = 0;

    virtual std::string getDebugInfo(int64_t id) const = 0;

    virtual int importSkins(std::string const& dirPath) = 0;

    virtual bool getSkinBlob(std::string const& skinId, std::string& out) const = 0;

    virtual bool registerSkinFromBlob(std::string const& blob) = 0;

    virtual bool setRotationLight(int64_t id, float yaw) = 0;

    virtual bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw) = 0;

    virtual bool clearPlayerRotation(int64_t id, std::string const& playerName) = 0;

    virtual bool clearPlayerRotations(int64_t id) = 0;

    virtual bool setPositionLight(int64_t id, float x, float y, float z, int dim) = 0;

    virtual bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId) = 0;

    virtual bool injectSkinAll(std::string const& targetName, std::string const& skinId) = 0;

    virtual bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) = 0;

    virtual bool playAnimationTo(
        int64_t            id,
        std::string const& playerName,
        std::string const& animation,
        std::string const& stopExpression,
        int                durationTicks) = 0;

    using EntitySpawnCallback                                         = std::function<void(int64_t id, std::string const& playerName)>;
    virtual void setEntitySpawnCallback(EntitySpawnCallback callback) = 0;

    virtual bool stopAnimation(int64_t id)                                  = 0;
    virtual bool stopAnimationTo(int64_t id, std::string const& playerName) = 0;

    virtual bool setPose(int64_t id, int pose) = 0;

    virtual bool setEquipment(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) = 0;

    virtual bool setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId) = 0;
    virtual bool setRidePlayer(int64_t id, std::string const& playerName)  = 0;

    virtual bool setRideEntity(int64_t id, int64_t vehicleEntityId) = 0;
    virtual bool clearRide(int64_t id)                              = 0;

    virtual bool playEmote(int64_t id, std::string const& pieceId, int lengthTicks = 0) = 0;
};

inline constexpr int kTradeTierNovice = 1;
inline constexpr int kTradeTierMaster = 5;

struct TradeMenuItem {
    std::string              type;
    int                      count{1};
    int                      damage{0};
    std::string              name;
    std::vector<std::string> lore;
    bool                     isBlock{false};
    int                      blockVersion{18168865};
};

struct TradeMenuOffer {
    TradeMenuItem buyA;
    TradeMenuItem buyB;
    TradeMenuItem sell;

    int tier{kTradeTierNovice};
    int maxUses{16};
    int traderExp{0};

    bool locked{false};
};

struct TradeMenuSpec {
    std::string tradeType{"entity.villager.butcher"};

    int tier{kTradeTierNovice};
    int experience{0};

    bool                        usePacketOffers{true};
    std::int64_t                carrierUniqueIdOverride{0};
    std::vector<TradeMenuOffer> offers;

    std::string carrierIdentifier{"minecraft:villager_v2"};

    bool settleLocally{false};
};

struct TradeSettlementEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         offerIndex{-1};
    bool        ok{false};
    std::string reason;
};

// 交易菜单
class ITradeMenu {
public:
    virtual ~ITradeMenu() = default;

    virtual int64_t open(std::string const& playerName, TradeMenuSpec const& spec) = 0;
    virtual bool    update(int64_t menuId, TradeMenuSpec const& spec)              = 0;
    virtual bool    close(int64_t menuId)                                          = 0;
    virtual void    closeAll()                                                     = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t menuId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const            = 0;

    virtual bool addOffer(int64_t menuId, TradeMenuOffer const& offer) = 0;

    virtual bool setTier(int64_t menuId, int tier, int experience) = 0;

    virtual bool               setSettleLocally(int64_t menuId, bool on) = 0;
    [[nodiscard]] virtual bool isSettleLocally(int64_t menuId) const     = 0;

    virtual uint64_t addSettlementListener(std::function<void(TradeSettlementEvent const&)> listener) = 0;
    virtual bool     removeSettlementListener(uint64_t token)                                         = 0;

    virtual std::vector<std::string> pollSettlements() = 0;
};

struct NpcDialogButton {
    std::string              label;
    std::vector<std::string> commands;
    std::string              actionId;

    int mode{0};
};

struct NpcDialogSpec {

    std::string carrierIdentifier{"minecraft:npc"};
    std::string npcName{"NPC"};
    std::string sceneName{"main"};
    std::string dialogue;

    std::int64_t                 npcUniqueIdOverride{0};
    std::vector<NpcDialogButton> buttons;
    std::string                  rawActionJson;

    int         avatarSkinVariant{-1};
    std::string avatarViewSpec;
};

struct NpcDialogClickEvent {
    std::string playerName;
    int64_t     dialogId{-1};
    std::string sceneName;
    int         buttonIndex{-1};
    std::string actionId;
    bool        closed{false};

    std::vector<std::string> commands;
};

struct ContainerMenuItem {
    std::string              type;
    int                      count{1};
    int                      damage{0};
    std::string              name;
    std::vector<std::string> lore;
};

struct ContainerMenuSpec {
    int                            rows{3};
    std::vector<ContainerMenuItem> items;

    bool useMinecart{false};

    std::string title{"虚拟容器"};

    int openDelayTicks{7};

    bool interactive{false};
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};
    bool        closed{false};
};

// 虚拟容器菜单
class IContainerMenu {
public:
    virtual ~IContainerMenu() = default;

    virtual int64_t open(std::string const& playerName, ContainerMenuSpec const& spec) = 0;
    virtual bool    close(int64_t menuId)                                              = 0;
    virtual void    closeAll()                                                         = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t menuId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const            = 0;

    virtual uint64_t addClickListener(std::function<void(ContainerClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token)                                        = 0;

    virtual bool update(int64_t menuId, ContainerMenuSpec const& spec) = 0;

    virtual bool setItem(int64_t menuId, int slot, ContainerMenuItem const& item) = 0;

    virtual bool setInteractive(int64_t menuId, bool on) = 0;

    virtual bool blockEvent(int64_t menuId, int b0, int b1) = 0;
};

// NPC 对话框
class INpcDialogue {
public:
    virtual ~INpcDialogue() = default;

    virtual int64_t open(std::string const& playerName, NpcDialogSpec const& spec) = 0;
    virtual bool    close(int64_t dialogId)                                        = 0;
    virtual void    closeAll()                                                     = 0;

    [[nodiscard]] virtual bool                 isOpen(int64_t dialogId) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const              = 0;

    virtual uint64_t addClickListener(std::function<void(NpcDialogClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token)                                        = 0;

    virtual bool update(int64_t dialogId, NpcDialogSpec const& spec) = 0;
};

enum class ClientInputDevice {
    Unknown = 0,
    KeyboardMouse,
    Touch,
    Gamepad,
    MotionController,
};

// 客户端输入设备感知
class IPlayerSensing {
public:
    virtual ~IPlayerSensing() = default;

    [[nodiscard]] virtual ClientInputDevice inputDeviceOf(std::string const& playerName) const = 0;

    [[nodiscard]] virtual bool isTouch(std::string const& playerName) const = 0;
};

inline constexpr int kFakeInventorySlots = 36;

struct FakeInventorySpec {
    std::vector<ContainerMenuItem> items;

    int refreshIntervalTicks{20};
};

struct FakeInventoryClickEvent {
    std::string playerName;
    int         slot{-1};
};

// 伪造背包（整包替换客户端背包）
class IFakeInventory {
public:
    virtual ~IFakeInventory() = default;

    virtual bool apply(std::string const& playerName, FakeInventorySpec const& spec) = 0;

    virtual bool setSlot(std::string const& playerName, int slot, ContainerMenuItem const& item) = 0;

    virtual bool refresh(std::string const& playerName) = 0;

    virtual bool clear(std::string const& playerName) = 0;
    virtual void clearAll()                           = 0;

    [[nodiscard]] virtual bool                     isActive(std::string const& playerName) const = 0;
    [[nodiscard]] virtual std::vector<std::string> getActivePlayers() const                      = 0;

    virtual uint64_t addClickListener(std::function<void(FakeInventoryClickEvent const&)> listener) = 0;
    virtual bool     removeClickListener(uint64_t token)                                            = 0;
};

struct SulfurDisplaySpec {
    float x{0.0f};
    float y{64.0f};
    float z{0.0f};
    int   dim{0};

    ContainerMenuItem block;

    std::string              archetype{"regular"};
    int                      variant{2};
    bool                     invisible{true};
    float                    scale{1.0f};
    double                   viewDistance{0.0};
    std::vector<std::string> visiblePlayers;
};

// 硫磺立方体展示
class ISulfurDisplay {
public:
    virtual ~ISulfurDisplay() = default;

    virtual int64_t create(SulfurDisplaySpec const& spec) = 0;

    virtual bool setBlock(int64_t id, ContainerMenuItem const& item) = 0;

    virtual bool setArchetype(int64_t id, std::string const& archetype) = 0;
    virtual bool setInvisible(int64_t id, bool on)                      = 0;
    virtual bool setScale(int64_t id, float scale)                      = 0;
    virtual bool destroy(int64_t id)                                    = 0;
    virtual void destroyAll()                                           = 0;

    [[nodiscard]] virtual bool                 exists(int64_t id) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const        = 0;

    [[nodiscard]] virtual int64_t entityIdOf(int64_t id) const = 0;
};

struct ItemActorSpec {
    std::string itemId;
    int         aux{0};
    int         count{1};
    float       x{0}, y{0}, z{0};
    int         dimension{0};
    float       vx{0}, vy{0}, vz{0};
};

// 掉落物（虚拟）
class IItemActor {
public:
    virtual ~IItemActor() = default;

    virtual int64_t spawn(ItemActorSpec const& spec) = 0;

    virtual bool setMotion(int64_t id, float vx, float vy, float vz) = 0;

    virtual bool moveTo(int64_t id, float x, float y, float z) = 0;

    virtual bool take(int64_t id, std::string const& playerName) = 0;
    virtual bool destroy(int64_t id)                             = 0;
    virtual void destroyAll()                                    = 0;

    [[nodiscard]] virtual bool                 exists(int64_t id) const = 0;
    [[nodiscard]] virtual std::vector<int64_t> getAllIds() const        = 0;
};

// 强制关闭客户端表单
class IFormControl {
public:
    virtual ~IFormControl() = default;

    virtual bool closeForm(std::string const& playerName) = 0;

    virtual int closeAllForms() = 0;
};

struct PlayerPacketMask {
    bool spawn{false};
    bool tab{false};
    bool motion{false};
    bool state{false};
    bool skin{false};
};

// 玩家出站包屏蔽（出生/Tab/移动/状态/皮肤）
class IPlayerVisibility {
public:
    virtual ~IPlayerVisibility() = default;

    virtual bool setMask(std::string const& playerName, PlayerPacketMask const& mask, bool alsoSelf = false) = 0;

    virtual bool setHidden(std::string const& playerName, bool hidden) = 0;

    virtual bool restore(std::string const& playerName) = 0;

    [[nodiscard]] virtual PlayerPacketMask maskOf(std::string const& playerName) const   = 0;
    [[nodiscard]] virtual bool             isHidden(std::string const& playerName) const = 0;
};

struct EntityView {

    std::string identifier;

    std::string asPlayer;

    std::string skinId;

    float scale{0.0f};

    float yOffset{0.0f};

    bool hidden{false};

    bool        hasNametag{false};
    std::string nametag;
    bool        nametagAlwaysShow{false};

    int  pose{0};
    bool hasPose{false};

    float metaScale{1.0f};
    bool  hasMetaScale{false};

    int  variant{0};
    bool hasVariant{false};
    int  markVariant{0};
    bool hasMarkVariant{false};

    int  colorIndex{0};
    bool hasColorIndex{false};
    int  color2{0};
    bool hasColor2{false};
};

struct BlockView {
    std::string type;
};

// 客户端视图覆盖（换模型/换皮/隐藏）
// 服务端真实实体的信息（先查再覆盖）
struct EntityInfo {
    std::string identifier;   // minecraft:zombie
    std::string nameTag;      // 玩家看到的显示名（可能带 §）
    bool        hasNameTag{false};
    bool        isPlayer{false};
    float       health{0.0f};
    std::int32_t variant{0};
    std::int32_t markVariant{0};
};

class IViewOverride {
public:
    virtual ~IViewOverride() = default;

    virtual bool overrideEntity(std::string const& playerName, std::int64_t entityUniqueId, EntityView const& view) = 0;

    virtual bool overrideBlock(std::string const& playerName, int x, int y, int z, BlockView const& view) = 0;

    virtual bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId) = 0;

    virtual bool clearBlock(std::string const& playerName, int x, int y, int z) = 0;

    virtual void clearAll(std::string const& playerName) = 0;

    [[nodiscard]] virtual std::string describeFor(std::string const& playerName) const = 0;

    // 1.36.0：读真实实体信息 / 把覆盖参数对齐到真实实体
    [[nodiscard]] virtual EntityInfo queryEntity(std::int64_t entityUniqueId) const = 0;

    virtual bool alignEntity(std::string const& playerName, std::int64_t entityUniqueId) = 0;
};

enum class BossBarColor : int {
    Pink          = 0,
    Blue          = 1,
    Red           = 2,
    Green         = 3,
    Yellow        = 4,
    Purple        = 5,
    RebeccaPurple = 6,
    White         = 7
};

enum class BossBarOverlay : int {
    Progress  = 0,
    Notched6  = 1,
    Notched10 = 2,
    Notched12 = 3,
    Notched20 = 4
};

enum class HudElement : int {
    PaperDoll     = 0,
    Armor         = 1,
    ToolTips      = 2,
    TouchControls = 3,
    Crosshair     = 4,
    HotBar        = 5,
    Health        = 6,
    ProgressBar   = 7,
    Hunger        = 8,
    AirBubbles    = 9,
    HorseHealth   = 10,
    StatusEffects = 11,
    ItemText      = 12
};

// Boss 血量条
class IBossBar {
public:
    virtual ~IBossBar() = default;

    virtual int64_t add(
        std::string const& playerName,
        std::string const& title,
        float              healthPercent,
        BossBarColor       color   = BossBarColor::Purple,
        BossBarOverlay     overlay = BossBarOverlay::Progress) = 0;

    virtual bool              setTitle(int64_t id, std::string const& title)                   = 0;
    virtual bool              setHealth(int64_t id, float healthPercent)                       = 0;
    virtual bool              setStyle(int64_t id, BossBarColor color, BossBarOverlay overlay) = 0;
    virtual bool              remove(int64_t id)                                               = 0;
    virtual void              removeAll(std::string const& playerName)                         = 0;
    [[nodiscard]] virtual int count(std::string const& playerName) const                       = 0;
};

// HUD 元素显隐
class IHud {
public:
    virtual ~IHud() = default;

    virtual bool setElement(std::string const& playerName, HudElement element, bool visible) = 0;

    virtual int setElements(std::string const& playerName, int mask, bool visible) = 0;

    virtual bool resetAll(std::string const& playerName) = 0;
};

// 实体表现事件（受伤/死亡/任意 ActorEvent）
class IActorEffect {
public:
    virtual ~IActorEffect() = default;

    virtual bool playHurt(int64_t entityRuntimeId)         = 0;
    virtual bool playDeath(int64_t entityRuntimeId)        = 0;
    virtual bool playInstantDeath(int64_t entityRuntimeId) = 0;

    virtual bool play(int64_t entityRuntimeId, int eventId, int data = 0) = 0;
};

// 侧边栏计分板
class ISidebar {
public:
    virtual ~ISidebar() = default;

    virtual bool set(std::string const& playerName, std::string const& title, std::vector<std::string> const& lines) = 0;
    virtual bool setTitle(std::string const& playerName, std::string const& title)                                   = 0;
    virtual bool setLine(std::string const& playerName, int index, std::string const& text)                          = 0;
    virtual bool remove(std::string const& playerName)                                                               = 0;
    virtual void removeAll()                                                                                         = 0;

    [[nodiscard]] virtual bool isShown(std::string const& playerName) const   = 0;
    [[nodiscard]] virtual int  lineCount(std::string const& playerName) const = 0;
};

// 死亡动画后的复活（补发 RespawnPacket）
class IPlayerRespawn {
public:
    virtual ~IPlayerRespawn() = default;

    virtual bool respawn(std::string const& playerName, int state = 2) = 0;

    virtual bool respawnAt(std::string const& playerName, float x, float y, float z, int state = 2) = 0;
};

// 按坐标预览真实容器（只读）
class IContainerPreview {
public:
    virtual ~IContainerPreview() = default;

    virtual int64_t preview(
        std::string const& playerName,
        int                dim,
        int                x,
        int                y,
        int                z,
        std::string const& title = {}) = 0;
};

// 拦截受伤/死亡等表现事件
class IActorEffectFilter {
public:
    virtual ~IActorEffectFilter() = default;

    virtual void block(int eventId)                                           = 0;
    virtual void unblock(int eventId)                                         = 0;
    virtual void blockForPlayer(std::string const& playerName, int eventId)   = 0;
    virtual void unblockForPlayer(std::string const& playerName, int eventId) = 0;
    virtual void blockForEntity(int64_t runtimeId, int eventId)               = 0;
    virtual void unblockForEntity(int64_t runtimeId, int eventId)             = 0;
    virtual void clear()                                                      = 0;

    virtual std::string stats() const = 0;
};

// 库入口：按域取接口；消费方在 enable 里比一次：库比消费方编译时旧就拒绝连接，避免调用不存在的尾部槽位
inline bool abiCompatible(std::uint32_t runtimeVersion) {
    return runtimeVersion >= HOLOGLIB_API_VERSION;
}

// 27 / 32 号槽占位（1.36.0 在此删过 locatorBar，虚表中部缩短；1.37.0 放回）。
// 空接口，当前无方法；后续新域直接替换类型，并抬 API 版本与 ABI stamp。
struct IReservedDomain {
    virtual ~IReservedDomain() = default;
};

class IHologramLib {
public:
    HOLOGLIB_API static IHologramLib& getInstance();

    virtual ~IHologramLib() = default;

    virtual IShapeDrawer&  shapes()      = 0;
    virtual IHologramText& holograms()   = 0;
    virtual IItemDetail&   itemDetails() = 0;

    virtual bool isLseAvailable() = 0;

    virtual uint32_t version() = 0;

    virtual IItemDisplay& itemDisplays() = 0;

    virtual int64_t findNearestItemDisplay(float x, float y, float z, int dim, double maxDist) = 0;

    virtual ICustomEntity& customEntities() = 0;

    virtual void setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener) = 0;
    virtual void clearGhostInteractListener()                                                      = 0;

    virtual std::vector<std::string> pollGhostInteractions() = 0;

    virtual IParticleShape& particleShapes() = 0;

    virtual IPlayerNpc& playerNpcs() = 0;

    virtual uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener) = 0;
    virtual bool     removeGhostInteractListener(uint64_t token)                                       = 0;

    virtual ITradeMenu& tradeMenus() = 0;

    virtual INpcDialogue& npcDialogs() = 0;

    virtual IContainerMenu& containerMenus() = 0;

    virtual IPlayerSensing& playerSensing() = 0;

    virtual IFakeInventory& fakeInventories() = 0;

    virtual ISulfurDisplay& sulfurDisplays() = 0;

    virtual IViewOverride& viewOverrides() = 0;

    virtual uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener) = 0;
    virtual bool     removeActorInteractListener(uint64_t token)                                       = 0;

    virtual IItemActor&        itemActors()       = 0;
    virtual IFormControl&      formControl()      = 0;
    virtual IReservedDomain& reservedSlot27() = 0; // 保留槽位（原 locatorBar）

    virtual IPlayerVisibility& playerVisibility() = 0;

    virtual IShapeDrawerExt& shapeExt() = 0;

    virtual IBossBar&       bossBars()      = 0;
    virtual IHud&           hud()           = 0;

    virtual IReservedDomain& reservedSlot32() = 0; // 保留槽位（原 locatorBarExt）

    virtual IActorEffect&   actorEffects()  = 0;
    virtual ISidebar&       sidebar()       = 0;

    virtual IPlayerRespawn& playerRespawn() = 0;

    virtual IContainerPreview&  containerPreview()  = 0;
    virtual IActorEffectFilter& actorEffectFilter() = 0;
};

HOLOGLIB_API bool wireTraceStart(std::string const& tag);
HOLOGLIB_API void wireTraceStop();
HOLOGLIB_API void wireTraceClear();
HOLOGLIB_API bool wireTraceActive();
HOLOGLIB_API std::size_t wireTraceCount();
HOLOGLIB_API std::string wireTraceDumpJson();
HOLOGLIB_API void        wireTraceMark(std::string const& tag);

} // namespace hologramlib

inline constexpr char const*            HOLOGLIB_CONSUMER_STAMP_SYMBOL             = "hologramlib_consumerAbiStamp";
extern "C" inline __declspec(dllexport) std::uint32_t hologramlib_consumerAbiStamp = HOLOGLIB_ABI_STAMP;
