// ItemDisplayExporter.cpp - FMBE 物品悬浮显示 LSE 导出实现
#include "ItemDisplayExporter.h"
#include "ItemDisplayManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemDisplayExporter::exportAll() {
    auto& mgr = ItemDisplayManager::getInstance();

    // 创建/生命周期
    // itemDisplayCreate(x, y, z, dim, item, aux) -> int64（id; <0 失败）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayCreate",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.create(cfg);
        });

    // itemDisplayCreateAdvanced(x, y, z, dim, item, aux, offsetX, offsetY, offsetZ, rotX, rotY, rotZ, scale) -> int64
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateAdvanced",
        [&mgr](float               x,
               float               y,
               float               z,
               int                 dim,
               std::string const&  item,
               int                 aux,
               std::string const&  offX,
               std::string const&  offY,
               std::string const&  offZ,
               std::string const&  rotX,
               std::string const&  rotY,
               std::string const&  rotZ,
               std::string const&  scale) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            return mgr.create(cfg);
        });

    // 单发原语: 让某玩家把 uid 这只实体看成 FMBE 物品模型（一次性发全套包, 无状态）
    // itemDisplayEmitForEntity(playerName, uniqueId: i64, item: s, aux: i, offsetY: s,
    //                         rotX: s, rotY: s, rotZ: s, scale: s, nbt: s, glint: b) -> i（1=item 2=block, <0 失败）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayEmitForEntity",
        [&mgr](std::string const& playerName,
               int64_t            uniqueId,
               std::string const& item,
               int                aux,
               std::string const& offsetY,
               std::string const& rotX,
               std::string const& rotY,
               std::string const& rotZ,
               std::string const& scale,
               std::string const& nbt,
               bool               glint) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetY   = offsetY;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            cfg.itemNbt   = nbt;
            cfg.itemGlint = glint;
            return mgr.emitForEntity(playerName, uniqueId, cfg);
        });

    // 单发原语: 建展示时直接带 NBT / 附魔光效（省掉"建完再改"的那次重建）
    // itemDisplayCreateFull(x,y,z,dim,item,aux,offX,offY,offZ,rotX,rotY,rotZ,scale,nbt,glint) -> i64
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateFull",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux,
               std::string const& offX, std::string const& offY, std::string const& offZ,
               std::string const& rotX, std::string const& rotY, std::string const& rotZ,
               std::string const& scale, std::string const& nbt, bool glint) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x = x; cfg.y = y; cfg.z = z; cfg.dimension = dim;
            cfg.item = item; cfg.itemAux = aux;
            cfg.offsetX = offX; cfg.offsetY = offY; cfg.offsetZ = offZ;
            cfg.rotX = rotX; cfg.rotY = rotY; cfg.rotZ = rotZ;
            cfg.scale = scale; cfg.itemNbt = nbt; cfg.itemGlint = glint;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroy",
        [&mgr](int64_t id) -> bool { return mgr.destroy(id); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroyAll",
        [&mgr]() -> void { mgr.destroyAll(); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayExists",
        [&mgr](int64_t id) -> bool { return mgr.exists(id); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayGetAllIds",
        [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // 1.7.0: 随机 ID 创建（库自动生成不重复 ID, 成功返还 ID 值）
    // itemDisplayCreateRandom(x, y, z, dim, item, aux) -> int64（随机段 ID; <0 失败）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateRandom",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.createRandom(cfg);
        });

    // 1.7.0: 指定 ID 创建（持久化恢复用; desiredId<=0 或已占用返回 -2）
    // itemDisplayCreateWithId(x, y, z, dim, item, aux, desiredId) -> int64
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateWithId",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux, int64_t desiredId)
            -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.createWithId(cfg, desiredId);
        });

    // 1.7.0: 查询 ID 是否在用
    // itemDisplayIsIdUsed(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayIsIdUsed",
        [&mgr](int64_t id) -> bool { return mgr.isIdUsed(id); });

    // 1.7.1: 相对缩放（factor>1 放大, 0<factor<1 缩小; 常量直接乘, 表达式包裹乘法）
    // itemDisplayScaleBy(id, factor) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayScaleBy",
        [&mgr](int64_t id, float factor) -> bool { return mgr.scaleBy(id, factor); });

    // 1.8.0: 换物品（带附加数据; nbt 为 SNBT 字符串, 空串清除附魔等用户数据）
    // itemDisplaySetItemWithNbt(id, item, aux, nbt) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetItemWithNbt",
        [&mgr](int64_t id, std::string const& item, int aux, std::string const& nbt) -> bool {
            return mgr.setItemWithNbt(id, item, aux, nbt);
        });

    // itemDisplaySetGlint(id, on) -> bool  附魔光效开关（BDS 原生路径）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetGlint",
        [&mgr](int64_t id, bool on) -> bool { return mgr.setGlint(id, on); });

    // 1.10.0: 可见玩家白名单 —— 仅指定玩家可见（按玩家名/LSE realName 匹配）
    // itemDisplaySetVisiblePlayers(id, playerNames: [s]) -> bool（空列表 = 清除限制 = 全员可见）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetVisiblePlayers",
        [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
            return mgr.setVisiblePlayers(id, playerNames);
        });
    // itemDisplayClearVisiblePlayers(id) -> bool（恢复全员可见）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayClearVisiblePlayers",
        [&mgr](int64_t id) -> bool { return mgr.clearVisiblePlayers(id); });

    // 1.10.1: 标量版白名单 —— 单玩家（规避 LSE 数组编组差异, JS 侧推荐）
    // itemDisplaySetVisiblePlayer(id, playerName: s) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetVisiblePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.setVisiblePlayer(id, playerName);
        });
    // 1.10.1: 诊断探针 —— 返回展示运行态摘要字符串（dim/pos/mode/filter/shown）
    // itemDisplayGetInfo(id) -> s（找不到返回 "not_found"）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayGetInfo",
        [&mgr](int64_t id) -> std::string { return mgr.getDebugInfo(id); });

    // 1.13.0: animateScale 导出已移除（前置纯洁性——动画调度归 LSE 消费者,
    // 库只保留 scaleTo 纯 setter 基元, 由 LSE 步进驱动实现渐变）

    // 1.12.0: 单次跳变缩放 —— 以新 scale 常量原地重发完整方块动画序列
    // （无逐帧动画、无 respawn; v.scale 写入者立即一致, 渐变路径的逐 tick 竞争抖动不存在）
    // itemDisplayScaleTo(id, targetScale: f) -> b   仅方块模式(mode=0 auto/2)
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayScaleTo",
        [&mgr](int64_t id, double targetScale) -> bool { return mgr.scaleTo(id, targetScale); });

    // 1.12.0: 无感创建 —— mode/视距/白名单在首次 spawn 前写入, 全程只发一次 Add 序列
    // （ItemPhys 无感创建等价; 创建后无需 setMode/setViewDistance/setVisiblePlayer, 无 respawn 闪烁）
    // itemDisplayCreateSeamless(x,y,z,dim,item,aux,offX,offY,offZ,rotX,rotY,rotZ,scale,
    //                           mode, viewDistance, visiblePlayer) -> int64
    //   mode: -1=默认(auto) 1=item 2=block; viewDistance: -1=默认 0=不限; visiblePlayer: 空串=全员
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateSeamless",
        [&mgr](float               x,
               float               y,
               float               z,
               int                 dim,
               std::string const&  item,
               int                 aux,
               std::string const&  offX,
               std::string const&  offY,
               std::string const&  offZ,
               std::string const&  rotX,
               std::string const&  rotY,
               std::string const&  rotZ,
               std::string const&  scale,
               int                 mode,
               double              viewDistance,
               std::string const&  visiblePlayer) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            return mgr.createSeamless(cfg, mode, viewDistance, visiblePlayer);
        });

    // 属性
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetItem",
        [&mgr](int64_t id, std::string const& item, int aux) -> bool {
            return mgr.setItem(id, item, aux);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetPosition",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
            return mgr.setPosition(id, x, y, z, dim);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetOffset",
        [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
            return mgr.setOffset(id, ox, oy, oz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetBaseOffset",
        [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
            return mgr.setBaseOffset(id, ox, oy, oz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetRotation",
        [&mgr](int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) -> bool {
            return mgr.setRotation(id, rx, ry, rz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetScale",
        [&mgr](int64_t id, std::string const& scale) -> bool { return mgr.setScale(id, scale); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetExtend",
        [&mgr](int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) -> bool {
            return mgr.setExtend(id, scale, rx, ry);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetMode",
        [&mgr](int64_t id, int mode) -> bool { return mgr.setMode(id, mode); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetEnabled",
        [&mgr](int64_t id, bool enabled) -> bool { return mgr.setEnabled(id, enabled); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetViewDistance",
        [&mgr](int64_t id, float dist) -> bool { return mgr.setViewDistance(id, dist); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayRotateY",
        [&mgr](int64_t id, float delta) -> bool { return mgr.rotateY(id, delta); });

    // itemDisplayFindNearest(x, y, z, dim, maxDist) -> int64（最近查找; 无匹配 -1）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayFindNearest",
        [&mgr](float x, float y, float z, int dim, float maxDist) -> int64_t {
            return mgr.findNearest(x, y, z, dim, maxDist);
        });

    // 1.17.0: 展示跟随玩家 —— 每 tick 同步目标玩家实时坐标（含跨维度自动 respawn）,
    // 对已见玩家发 MoveActorAbsolute（非 teleport, 客户端插值）平滑位移, 全程无 respawn。
    // itemDisplayFollow(id, playerName: s, offX: f, offY: f, offZ: f) -> b
    //   玩家下线自动解除跟随（方块原地保留）; setPosition 手动设位解除跟随
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayFollow",
        [&mgr](int64_t id, std::string const& playerName, float offX, float offY, float offZ) -> bool {
            return mgr.follow(id, playerName, offX, offY, offZ);
        });
    // itemDisplayUnfollow(id) -> b（解除跟随; 无跟随关系返回 true）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayUnfollow",
        [&mgr](int64_t id) -> bool { return mgr.unfollow(id); });

    // 1.17.0: AABB 判定体积 —— 对已见玩家广播 SetActorDataPacket(R53/R54), 即时无 respawn;
    // 数值持久入配置（后续 respawn 自动按当前值发包）; 0/0 = 恢复不可命中。
    // itemDisplaySetHitbox(id, width: f, height: f) -> b
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetHitbox",
        [&mgr](int64_t id, float width, float height) -> bool {
            return mgr.setHitbox(id, width, height);
        });

    // ── 26.40.12: 非等比拉伸 + live 变换（只重发 FMBE 动画, 不 respawn）──
    // itemDisplaySetStretch(id, xz: s, y: s) -> b（持久化 + 一次 respawn）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetStretch",
        [&mgr](int64_t id, std::string const& xz, std::string const& y) -> bool {
            return mgr.setStretch(id, xz, y);
        });
    // itemDisplaySetStretchLive(id, xz: s, y: s) -> b（只重发动画包, 无闪烁）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetStretchLive",
        [&mgr](int64_t id, std::string const& xz, std::string const& y) -> bool {
            return mgr.setStretchLive(id, xz, y);
        });
    // itemDisplaySetRotationLive(id, rx: s, ry: s, rz: s) -> b
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetRotationLive",
        [&mgr](int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) -> bool {
            return mgr.setRotationLive(id, rx, ry, rz);
        });
    // itemDisplaySetOffsetLive(id, ox: s, oy: s, oz: s) -> b
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetOffsetLive",
        [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
            return mgr.setOffsetLive(id, ox, oy, oz);
        });
    // itemDisplaySetScaleLive(id, scale: s) -> b
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetScaleLive",
        [&mgr](int64_t id, std::string const& scale) -> bool { return mgr.setScaleLive(id, scale); });
    // itemDisplaySetTransformLive(id, ox,oy,oz, rx,ry,rz, scale: s×7) -> b（一次重发）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetTransformLive",
        [&mgr](int64_t id,
               std::string const& ox, std::string const& oy, std::string const& oz,
               std::string const& rx, std::string const& ry, std::string const& rz,
               std::string const& scale) -> bool {
            return mgr.setTransformLive(id, ox, oy, oz, rx, ry, rz, scale);
        });

    // ── 26.40.12: 动画预设（库内逻辑, 组合成 Molang 后 live 重发）──
    // itemDisplaySetAnim(id, spinSpeed, spinAxis, bobAmp, bobPeriod, pulseAmp, pulsePeriod,
    //                    swayAmp, swayPeriod, phase, billboard) -> b
    //   spinSpeed 度/秒; spinAxis 0=X 1=Y 2=Z; period 秒; phase 秒; billboard 仅物品模式
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetAnim",
        [&mgr](int64_t id,
               float spinSpeed, int spinAxis,
               float bobAmp, float bobPeriod,
               float pulseAmp, float pulsePeriod,
               float swayAmp, float swayPeriod,
               float phase, bool billboard) -> bool {
            ItemDisplayAnim anim;
            anim.spinSpeed      = spinSpeed;
            anim.spinAxis       = spinAxis;
            anim.bobAmplitude   = bobAmp;
            anim.bobPeriod      = bobPeriod;
            anim.pulseAmplitude = pulseAmp;
            anim.pulsePeriod    = pulsePeriod;
            anim.swayAmplitude  = swayAmp;
            anim.swayPeriod     = swayPeriod;
            anim.phase          = phase;
            anim.billboard      = billboard;
            return mgr.setAnim(id, anim);
        });
    // itemDisplayClearAnim(id) -> b（恢复 setAnim 之前的 offset/rot/scale）
    hologramlib::lse::exportAs(
        NAMESPACE, "itemDisplayClearAnim", [&mgr](int64_t id) -> bool { return mgr.clearAnim(id); });

    // ── 26.40.13: 单通道预设 ──
    // itemDisplaySetPreset(id, preset, a, b, c) -> b
    //   spin(a=度/秒, c=轴0/1/2) · bob(a=幅度, b=周期) · pulse(a=比例, b=周期)
    //   sway(a=度, b=周期) · orbit(a=半径, b=周期, c=平面0/1/2)
    //   tumble(a=度/秒) · wobble(a=度, b=倍率) · heartbeat(a=比例, b=周期)
    //   pendulum(a=度, b=周期) · billboard(a=1开, c=1带俯仰)
    //   lod(a=近格, b=远格, c=远端百分比缩放) · firstperson(a=1开) · clear
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetPreset",
        [&mgr](int64_t id, std::string const& preset, float a, float b, int c) -> bool {
            return mgr.setAnimPreset(id, preset, a, b, c);
        });
}

} // namespace debugshape_export
