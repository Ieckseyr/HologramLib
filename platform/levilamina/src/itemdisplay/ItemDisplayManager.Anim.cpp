#include "ItemDisplayManager.h"
#include "ItemDisplayRuntimeOps.h"
#include "core/DiagFlags.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"

#include "kernel/PacketSink.h"

#include "core/TickDriver.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/deps/nbt/ShortTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/enchanting/Enchant.h>
#include <mc/world/item/enchanting/EnchantmentInstance.h>
#include <mc/world/item/enchanting/ItemEnchants.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>


#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>

namespace debugshape_export {
using namespace ide_ops;

void ItemDisplayManager::enqueueFmbeAnimsLocked(
    int64_t                  id,
    ItemDisplayConfig const& data,
    Runtime const&           rt,
    mce::UUID const&         uuid,
    std::uint64_t            base,
    bool                     block) {
    auto const tag  = "." + std::to_string(id);
    auto       push = [&](std::uint64_t at, std::string anim, std::string ctrl, std::string stop) {
        AnimEntry e{uuid, rt.runtimeId, std::move(anim), std::move(ctrl), std::move(stop)};
        animQueue().emplace(at, std::move(e));
    };
    if (block) {
        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, buildBlockMatrixExpr(data));
        push(base + 3, "animation.creeper.swelling", "wiki.fmbe.3d_blocks.anim1" + tag, buildBlockSwellingExpr(rt.stretchXZ, rt.stretchY));
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.fmbe.3d_blocks.anim2" + tag, std::string{kBlockHeadPosExpr});
        push(base + 5, "animation.warden.move", "wiki.fmbe.3d_blocks.anim3" + tag, std::string{kBlockBodyRotExpr});
        push(base + 6, "animation.player.attack.rotations", "wiki.fmbe.3d_blocks.anim4" + tag, std::string{kBlockAttackRotExpr});
    } else {

        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, "");
        push(base + 3, "animation.creeper.swelling", "wiki.scale" + tag, buildItemScaleExpr(data, rt.stretchXZ, rt.stretchY));
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.posrot" + tag, std::string{kItemPosRotExpr});
    }
}

bool ItemDisplayManager::resendFmbeLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt) {
    if (rt.shownPlayers.empty())
        return false;

    auto& q = animQueue();
    for (auto it = q.begin(); it != q.end();) {

        if (it->second.runtimeId == rt.runtimeId && !it->second.equip) {
            it = q.erase(it);
        } else {
            ++it;
        }
    }
    bool block = false;
    if (data.mode == 2) {
        block = true;
    } else if (data.mode == 0) {
        if (!rt.cachedStack)
            return false;
        block = (effectiveMode(data, *rt.cachedStack) == 2);
    }
    auto const base = currentTick();
    for (auto const& uuid : rt.shownPlayers) {
        enqueueFmbeAnimsLocked(id, data, rt, uuid, base, block);
    }
    return true;
}

bool ItemDisplayManager::setAnim(int64_t id, ItemDisplayAnim const& anim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;

    if (!rt.animActive) {
        rt.animBase.ox    = it->offsetX;
        rt.animBase.oy    = it->offsetY;
        rt.animBase.oz    = it->offsetZ;
        rt.animBase.rx    = it->rotX;
        rt.animBase.ry    = it->rotY;
        rt.animBase.rz    = it->rotZ;
        rt.animBase.scale = it->scale;
        rt.animActive     = true;
    }
    rt.anim = anim;

    bool itemMode = true;
    if (it->mode == 2) {
        itemMode = false;
    } else if (it->mode == 0 && rt.cachedStack) {
        itemMode = (effectiveMode((*it), *rt.cachedStack) == 1);
    }

    applyAnimToConfig((*it), anim, rt.animBase, itemMode);
    return resendFmbeLocked(id, (*it), rt);
}

bool ItemDisplayManager::setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr || configOf(id) == nullptr)
        return false;
    auto anim = rit->anim;
    if (preset == "spin") {
        anim.spinSpeed = a;
        anim.spinAxis  = (c >= 0 && c <= 2) ? c : 1;
    } else if (preset == "bob") {
        anim.bobAmplitude = a;
        anim.bobPeriod    = b > 0.01f ? b : 2.0f;
    } else if (preset == "pulse") {
        anim.pulseAmplitude = a;
        anim.pulsePeriod    = b > 0.01f ? b : 2.0f;
    } else if (preset == "sway") {
        anim.swayAmplitude = a;
        anim.swayPeriod    = b > 0.01f ? b : 3.0f;
    } else if (preset == "orbit") {
        anim.orbitRadius = a;
        anim.orbitPeriod = b > 0.01f ? b : 4.0f;
        anim.orbitPlane  = (c >= 0 && c <= 2) ? c : 2;
    } else if (preset == "tumble") {
        anim.tumbleSpeed = a;
    } else if (preset == "wobble") {
        anim.wobbleAmplitude = a;
        anim.wobbleSpeed     = b > 0.01f ? b : 1.0f;
    } else if (preset == "heartbeat") {
        anim.heartbeatAmplitude = a;
        anim.heartbeatPeriod    = b > 0.01f ? b : 1.2f;
    } else if (preset == "pendulum") {
        anim.pendulumAmplitude = a;
        anim.pendulumPeriod    = b > 0.01f ? b : 3.0f;
    } else if (preset == "billboard") {
        anim.billboard      = (a != 0.0f);
        anim.billboardPitch = (c != 0);
    } else if (preset == "lod") {
        anim.lodNear     = a;
        anim.lodFar      = b;
        anim.lodMinScale = c > 0 ? static_cast<float>(c) / 100.0f : 0.5f;
    } else if (preset == "firstperson") {
        anim.hideInFirstPerson = (a != 0.0f);
    } else if (preset == "clear") {
        return clearAnim(id);
    } else {
        return false;
    }
    return setAnim(id, anim);
}

bool ItemDisplayManager::clearAnim(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;
    if (!rt.animActive)
        return true;

    it->offsetX   = rt.animBase.ox;
    it->offsetY   = rt.animBase.oy;
    it->offsetZ   = rt.animBase.oz;
    it->rotX      = rt.animBase.rx;
    it->rotY      = rt.animBase.ry;
    it->rotZ      = rt.animBase.rz;
    it->scale     = rt.animBase.scale;
    rt.animActive = false;
    rt.anim       = {};
    return resendFmbeLocked(id, (*it), rt);
}

} // namespace debugshape_export
