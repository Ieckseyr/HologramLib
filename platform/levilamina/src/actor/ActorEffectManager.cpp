#include "ActorEffectManager.h"

#include "kernel/PacketSink.h"
#include "core/TickDriver.h"
#include "kernel/PlayerGrid.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/server/ServerPlayer.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/ActorEvent.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/ActorEventPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/MobEffectPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>

#include <format>
#include <string_view>

namespace debugshape_export {

namespace {


::Player* playerByRuntimeId(std::uint64_t runtimeId) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<std::uint64_t>(player.getRuntimeID().rawID) == runtimeId) {
            found = &player;
            return false;
        }
        return true;
    });
    return found;
}

::ItemStack makeItem(std::string const& name) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return {};
    auto registry = level->getItemRegistry();
    auto  weak     = registry.getItem(::HashedString(name));
    if (!weak)
        return {};
    ::ItemStack stack{};
    stack.reinit(*weak, 1, 0);
    return stack;
}

void sendEquipment(std::uint64_t runtimeId, ::ItemStack const& stack, int slot, int containerId) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    ::MobEquipmentPacket packet{
        ::ActorRuntimeID{runtimeId},
        stack,
        static_cast<unsigned char>(slot),
        0,
        static_cast<::ContainerID>(containerId)
    };
    level->forEachPlayer([&](::Player& player) -> bool {
        packet.sendTo(player);
        return true;
    });
}

void actorFxTick() {
    ActorEffectManager::getInstance().tickFx();
}

tick::AutoRegister gActorFxRegister{"actorfx", &actorFxTick, tick::Order::ActorFx};

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

bool sendTo(::Player& player, int64_t runtimeId, int eventId, int data) {
    sculk::protocol::ActorEventPacket packet;
    packet.mRuntimeId = static_cast<std::uint64_t>(runtimeId);
    packet.mEventId   = static_cast<sculk::protocol::ActorEvent>(eventId);
    packet.mData      = data;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    wire::sendRaw(player, bytes);
    return true;
}

} // namespace

ActorEffectManager& ActorEffectManager::getInstance() {
    static ActorEffectManager instance;
    return instance;
}

bool ActorEffectManager::dispatch(std::string const& playerName, int64_t runtimeId, int eventId, int data) {
    if (runtimeId <= 0)
        return false;

    if (!playerName.empty()) {
        ::Player* player = findPlayer(playerName);
        if (player == nullptr)
            return false;
        return sendTo(*player, runtimeId, eventId, data);
    }

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;
    bool sent = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        sent = sendTo(player, runtimeId, eventId, data) || sent;
        return true;
    });
    return sent;
}

bool ActorEffectManager::playHurt(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::Hurt), 0);
}

bool ActorEffectManager::playDeath(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::Death), 0);
}

bool ActorEffectManager::playInstantDeath(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::InstantDeath), 0);
}

bool ActorEffectManager::play(int64_t entityRuntimeId, int eventId, int data) {
    return dispatch("", entityRuntimeId, eventId, data);
}

bool ActorEffectManager::playFor(
    std::string const& playerName,
    int64_t            entityRuntimeId,
    int                eventId,
    int                data) {
    return dispatch(playerName, entityRuntimeId, eventId, data);
}

bool ActorEffectManager::setFreeze(int64_t entityRuntimeId, float strength) {
    if (entityRuntimeId <= 0)
        return false;

    float const value = strength < 0.0f ? 0.0f : (strength > 1.0f ? 1.0f : strength);

    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId      = static_cast<std::uint64_t>(entityRuntimeId);
    packet.mTick                = core::serverTick();
    packet.mSynchedProperties   = {};
    packet.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::FreezingEffectStrength, value},
    };

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;

    bool sent = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        sent = wire::sendRaw(player, bytes) || sent;
        return true;
    });
    return sent;
}

bool ActorEffectManager::playAnimation(
    int64_t            entityRuntimeId,
    std::string const& animation,
    std::string const& nextState,
    std::string const& stopExpression,
    float              blendOutTime,
    std::string const& controller,
    int                molangVersion) {
    if (entityRuntimeId <= 0 || animation.empty())
        return false;

    sculk::protocol::AnimateEntityPacket packet;
    packet.mAnimation                   = animation;
    packet.mNextState                   = nextState;
    packet.mStopExpression              = stopExpression;
    packet.mStopExpressionMolangVersion = static_cast<sculk::protocol::MolangVersion>(molangVersion);
    packet.mController                  = controller;
    packet.mBlendOutTime                = blendOutTime;
    packet.mRuntimeIds                  = {static_cast<std::uint64_t>(entityRuntimeId)};

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;

    bool sent = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        sent = wire::sendRaw(player, bytes) || sent;
        return true;
    });
    return sent;
}

bool ActorEffectManager::playCreeperSwelling(int64_t entityRuntimeId) {
    // 朋友的⑨
    // 客户端按 q.life_time 逐帧算膨胀，服务端只发一次，整段是平滑的；
    // 玩家模型没有苦力怕那套骨骼，下半身不会跟着动（客户端模型限制）。
    static constexpr std::string_view kSwellingExpression =
        "v.swelling_sin=math.sin(q.life_time*720);"
        "v.swelling_scale1=1.0-0.1*math.max(v.swelling_sin,0)+0.15*math.max(-v.swelling_sin,0);"
        "v.swelling_scale2=1.0+0.1*math.max(v.swelling_sin,0)-0.4*math.max(-v.swelling_sin,0);";

    bool const played = playAnimation(
        entityRuntimeId,
        "animation.creeper.swelling",
        "none",
        std::string{kSwellingExpression},
        0.0f,
        "",
        13
    );

    // 非玩家实体顺带转圈（本地玩家模型朝向被客户端锁死，转了也看不见，跳过）
    if (played) {
        auto level = ::ll::service::getLevel();
        if (level) {
            auto const rid   = static_cast<std::uint64_t>(entityRuntimeId);
            auto*      actor = level->getRuntimeEntity(::ActorRuntimeID{rid}, false);
            if (actor != nullptr && !actor->isPlayer()) {
                std::lock_guard lock(mFxMutex);
                for (auto it = mSpins.begin(); it != mSpins.end();) {
                    if (it->runtimeId == rid) {
                        it = mSpins.erase(it);
                    } else {
                        ++it;
                    }
                }
                mSpins.push_back({rid, core::serverTick() + 200, 0.0f});
            }
        }
    }
    return played;
}

bool ActorEffectManager::playTotem(int64_t entityRuntimeId, int holdTicks) {
    // 图腾：只发 MobEquipment 客户端不认自己的图腾生效，得真动服务端副手让引擎同步背包，
    // 发完 ActorEvent 65 再把原副手换回去。
    if (entityRuntimeId <= 0)
        return false;

    auto const rid    = static_cast<std::uint64_t>(entityRuntimeId);
    auto*      player = playerByRuntimeId(rid);
    if (player == nullptr)
        return false;

    ::ItemStack original{};
    {
        auto hands = player->getAllHandEquipment();
        if (hands.size() > 1 && hands[1] != nullptr) {
            original = *hands[1];
        }
    }

    auto totem = makeItem("minecraft:totem_of_undying");
    if (totem.isNull())
        return false;

    auto& server = static_cast<::ServerPlayer&>(*player);
    server.setOffhandSlot(totem);
    player->sendInventory(false);

    bool const played = play(entityRuntimeId, 65, 0);

    int const ticks = holdTicks > 0 ? holdTicks : 20;
    {
        std::lock_guard lock(mFxMutex);
        for (auto it = mFxPending.begin(); it != mFxPending.end();) {
            if (it->runtimeId == rid) {
                it = mFxPending.erase(it);
            } else {
                ++it;
            }
        }
        mFxPending.push_back(
            {rid,
             core::serverTick() + static_cast<std::uint64_t>(ticks),
             original,
             1,
             static_cast<int>(::ContainerID::Offhand),
             true}
        );
    }
    return played;
}

bool ActorEffectManager::playOminousDrink(std::string const& playerName, int holdTicks) {
    // 灾厄之兆：主手换成不祥之瓶（服务端背包同步）+ ActorEvent 29（DrinkPotion），到点还原。
    auto* player = findPlayer(playerName);
    if (player == nullptr)
        return false;
    auto const rid = static_cast<std::uint64_t>(player->getRuntimeID().rawID);

    int const    slot     = player->getSelectedItemSlot();
    ::ItemStack  original = player->getSelectedItem();

    auto bottle = makeItem("minecraft:ominous_bottle");
    if (bottle.isNull())
        return false;

    player->getInventory().setItem(slot, bottle);
    player->sendInventory(false);

    bool const played = play(static_cast<int64_t>(rid), 29, 0);

    int const ticks = holdTicks > 0 ? holdTicks : 40;
    {
        std::lock_guard lock(mFxMutex);
        for (auto it = mFxPending.begin(); it != mFxPending.end();) {
            if (it->runtimeId == rid) {
                it = mFxPending.erase(it);
            } else {
                ++it;
            }
        }
        mFxPending.push_back(
            {rid,
             core::serverTick() + static_cast<std::uint64_t>(ticks),
             original,
             slot,
             static_cast<int>(::ContainerID::Inventory),
             true}
        );
    }
    return played;
}

bool ActorEffectManager::setPose(int64_t entityRuntimeId, int poseIndex) {
    if (entityRuntimeId <= 0 || poseIndex < 0)
        return false;

    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId      = static_cast<std::uint64_t>(entityRuntimeId);
    packet.mTick                = core::serverTick();
    packet.mSynchedProperties   = {};
    packet.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::PoseIndex, static_cast<std::int32_t>(poseIndex)},
    };

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;

    bool sent = false;
    level->forEachPlayer([&](::Player& p) -> bool {
        sent = wire::sendRaw(p, bytes) || sent;
        return true;
    });
    return sent;
}

bool ActorEffectManager::playCelebrate(std::string const& playerName, float seconds) {
    // 村庄英雄：原版没有玩家专用动作，用 humanoid 的举手庆祝（循环动画，靠 anim_time 停）
    auto* player = findPlayer(playerName);
    if (player == nullptr)
        return false;
    auto const rid = static_cast<std::uint64_t>(player->getRuntimeID().rawID);

    float const stopAt = seconds > 0.0f ? seconds : 3.0f;
    auto const  stop   = std::format("q.anim_time > {:.2f}", stopAt);

    return playAnimation(
        static_cast<int64_t>(rid),
        "animation.humanoid.celebrating",
        "none",
        stop,
        0.0f,
        "",
        13
    );
}

bool ActorEffectManager::showEffect(
    std::string const& playerName,
    int                effectId,
    int                durationTicks,
    int                amplifier
) {
    auto* player = findPlayer(playerName);
    if (player == nullptr || effectId < 0)
        return false;

    sculk::protocol::MobEffectPacket packet;
    packet.mActorRuntimeId      = static_cast<std::uint64_t>(player->getRuntimeID().rawID);
    packet.mEventId             = sculk::protocol::MobEffectPacket::Event::Add;
    packet.mEffectId            = effectId;
    packet.mEffectAmplifier     = amplifier < 0 ? 0 : amplifier;
    packet.mShowParticles       = true;
    packet.mEffectDurationTicks = durationTicks > 0 ? durationTicks : 200;
    packet.mTick                = core::serverTick();
    packet.mAmbient             = false;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;

    bool sent = false;
    level->forEachPlayer([&](::Player& p) -> bool {
        sent = wire::sendRaw(p, bytes) || sent;
        return true;
    });
    return sent;
}

void ActorEffectManager::tickFx() {
    std::lock_guard lock(mFxMutex);

    if (!mSpins.empty()) {
        auto level = ::ll::service::getLevel();
        if (!level) {
            mSpins.clear();
        } else {
            auto const now = core::serverTick();
            auto       rotByte = [](float deg) -> std::uint8_t {
                float d = std::fmod(deg, 360.0f);
                if (d < 0.0f)
                    d += 360.0f;
                return static_cast<std::uint8_t>((d * 256.0f) / 360.0f);
            };
            for (auto it = mSpins.begin(); it != mSpins.end();) {
                if (it->endTick <= now) {
                    it = mSpins.erase(it);
                    continue;
                }
                auto* actor = level->getRuntimeEntity(::ActorRuntimeID{it->runtimeId}, false);
                if (actor == nullptr) {
                    it = mSpins.erase(it);
                    continue;
                }
                auto const& pos = actor->getPosition();
                auto const& rot = actor->getRotation();
                float const yaw = rot.y + it->yaw;

                sculk::protocol::MoveActorAbsolutePacket packet;
                packet.mActorRuntimeId = it->runtimeId;
                packet.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::ForceMove;
                packet.mPosition       = {pos.x, pos.y, pos.z};
                packet.mRotationX      = rotByte(rot.x);
                packet.mRotationY      = rotByte(yaw);
                packet.mRotationYHead  = rotByte(yaw);

                auto bytes = wire::encode(packet);
                if (!bytes.empty()) {
                    level->forEachPlayer([&](::Player& player) -> bool {
                        wire::sendRaw(player, bytes);
                        return true;
                    });
                    it->yaw += 36.0f;
                }
                ++it;
            }
        }
    }

    if (mFxPending.empty())
        return;

    auto const now = core::serverTick();
    for (auto it = mFxPending.begin(); it != mFxPending.end();) {
        if (it->endTick > now) {
            ++it;
            continue;
        }

        if (auto* player = playerByRuntimeId(it->runtimeId); player != nullptr && it->engine) {
            if (it->containerId == static_cast<int>(::ContainerID::Offhand)) {
                static_cast<::ServerPlayer&>(*player).setOffhandSlot(it->original);
            } else {
                player->getInventory().setItem(it->slot, it->original);
            }
            player->sendInventory(false);
        } else {
            sendEquipment(it->runtimeId, it->original, it->slot, it->containerId);
        }
        it = mFxPending.erase(it);
    }
}

} // namespace debugshape_export
