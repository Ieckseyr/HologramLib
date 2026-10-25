// PlayerVisibilityManager.cpp - 按玩家屏蔽出站包
#include "PlayerVisibilityManager.h"

#include "view/ViewOverrideManager.h"

#include <ll/api/service/Bedrock.h>

#include <mc/network/Packet.h> // Packet + ll::PayloadPacket
#include <mc/network/packet/ActorEventPacketPayload.h>
#include <mc/network/packet/AddPlayerPacketPayload.h>
#include <mc/network/packet/AnimatePacketPayload.h>
#include <mc/network/packet/MobEffectPacketPayload.h>
#include <mc/network/packet/MoveActorAbsolutePacketPayload.h>
#include <mc/network/packet/MovePlayerPacketPayload.h>
#include <mc/network/packet/PlayerListPacketPayload.h>
#include <mc/network/packet/PlayerSkinPacketPayload.h>
#include <mc/network/packet/SetActorDataPacketPayload.h>
#include <mc/network/packet/SetActorMotionPacketPayload.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <cstdint>

namespace debugshape_export {

namespace {

// TypedStorage 在小尺寸/对齐时可能退化成透明别名（没有 .get()），两种形态统一取值
template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}

template <class PayloadT>
PayloadT const& payloadOf(::Packet const& packet) {
    return static_cast<PayloadT const&>(static_cast<::ll::PayloadPacket<PayloadT> const&>(packet));
}

enum class Category { Unknown,
                      Spawn,
                      Tab,
                      Motion,
                      State,
                      Skin };

// 按 runtimeId 认主体；认不出来的返回 0
std::uint64_t runtimeIdOf(::Packet const& packet, Category& category) {
    switch (packet.getId()) {
    case ::MinecraftPacketIds::AddPlayer:
        category = Category::Spawn;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::AddPlayerPacketPayload>(packet).mRuntimeId).rawID);
    case ::MinecraftPacketIds::MovePlayer:
        category = Category::Motion;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::MovePlayerPacketPayload>(packet).mPlayerID).rawID);
    case ::MinecraftPacketIds::MoveAbsoluteActor:
        category = Category::Motion;
        return static_cast<std::uint64_t>(
            unwrap(unwrap(payloadOf<::MoveActorAbsolutePacketPayload>(packet).mMoveData).mRuntimeId).rawID);
    case ::MinecraftPacketIds::SetActorMotion:
        category = Category::Motion;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::SetActorMotionPacketPayload>(packet).mRuntimeId).rawID);
    case ::MinecraftPacketIds::SetActorData:
        category = Category::State;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::SetActorDataPacketPayload>(packet).mId).rawID);
    case ::MinecraftPacketIds::Animate:
        category = Category::State;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::AnimatePacketPayload>(packet).mRuntimeId).rawID);
    case ::MinecraftPacketIds::ActorEvent:
        category = Category::State;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::ActorEventPacketPayload>(packet).mRuntimeId).rawID);
    case ::MinecraftPacketIds::MobEffect:
        category = Category::State;
        return static_cast<std::uint64_t>(unwrap(payloadOf<::MobEffectPacketPayload>(packet).mRuntimeId).rawID);
    default:
        category = Category::Unknown;
        return 0;
    }
}

::Player* playerByRuntimeId(std::uint64_t runtimeId) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<std::uint64_t>(unwrap(player.getRuntimeID()).rawID) == runtimeId) {
            found = &player;
            return false;
        }
        return true;
    });
    return found;
}

::Player* playerByUuid(::mce::UUID const& uuid) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (player.getUuid() == uuid) {
            found = &player;
            return false;
        }
        return true;
    });
    return found;
}

} // namespace

PlayerVisibilityManager& PlayerVisibilityManager::getInstance() {
    static PlayerVisibilityManager instance;
    return instance;
}

bool PlayerVisibilityManager::anyActive() const {
    std::lock_guard lock(mMutex);
    return !mEntries.empty();
}

bool PlayerVisibilityManager::setMask(
    std::string const&                   playerName,
    hologramlib::PlayerPacketMask const& mask,
    bool                                 alsoSelf) {
    auto level = ::ll::service::getLevel();
    if (!level || level->getPlayer(playerName) == nullptr)
        return false;

    std::lock_guard lock(mMutex);
    auto&           entry = mEntries[playerName];
    entry.mask            = mask;
    entry.alsoSelf        = alsoSelf;
    if (!entry.hidden && !mask.spawn && !mask.tab && !mask.motion && !mask.state && !mask.skin) {
        mEntries.erase(playerName); // 全关 = 不占表
    }
    return true;
}

bool PlayerVisibilityManager::setHidden(std::string const& playerName, bool hidden) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return false;
    ::Player* player = level->getPlayer(playerName);
    if (player == nullptr)
        return false;

    auto const uniqueId = static_cast<std::int64_t>(unwrap(player->getOrCreateUniqueID()).rawID);

    auto& viewManager = view::ViewOverrideManager::getInstance();
    if (hidden) {
        hologramlib::EntityView view;
        view.hidden = true;
        if (!viewManager.overrideEntity("", uniqueId, view))
            return false;
    } else {
        viewManager.clearEntity("", uniqueId);
        viewManager.clearEntity(playerName, uniqueId);
    }

    std::lock_guard lock(mMutex);
    auto&           entry = mEntries[playerName];
    entry.hidden          = hidden;
    if (!hidden && !entry.mask.spawn && !entry.mask.tab && !entry.mask.motion && !entry.mask.state && !entry.mask.skin) {
        mEntries.erase(playerName);
    }
    return true;
}

void PlayerVisibilityManager::forceRespawn(std::string const& playerName) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    ::Player* player = level->getPlayer(playerName);
    if (player == nullptr)
        return;

    auto const uniqueId = static_cast<std::int64_t>(unwrap(player->getOrCreateUniqueID()).rawID);

    // 借视图覆盖域的"撤销即补发真身"流程：先挂一条隐藏覆盖，再撤销 ——
    // 撤销那一步会把真身的出生包重新发给所有观看者，正好是"出生包恢复发出"。
    hologramlib::EntityView view;
    view.hidden   = true;
    auto& manager = view::ViewOverrideManager::getInstance();
    manager.overrideEntity("", uniqueId, view);
    manager.clearEntity("", uniqueId);
}

bool PlayerVisibilityManager::restore(std::string const& playerName) {
    auto level = ::ll::service::getLevel();
    if (!level || level->getPlayer(playerName) == nullptr)
        return false;

    {
        std::lock_guard lock(mMutex);
        mEntries.erase(playerName);
    }
    forceRespawn(playerName);
    return true;
}

hologramlib::PlayerPacketMask PlayerVisibilityManager::maskOf(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mEntries.find(playerName);
    if (it == mEntries.end())
        return {};
    return it->second.mask;
}

bool PlayerVisibilityManager::isHidden(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mEntries.find(playerName);
    return it != mEntries.end() && it->second.hidden;
}

bool PlayerVisibilityManager::shouldDrop(::Player& recipient, ::Packet const& packet) {
    if (!anyActive())
        return false; // 快路径: 没有任何屏蔽

    Category      category = Category::Unknown;
    std::string   subject;
    std::uint64_t runtimeId = runtimeIdOf(packet, category);

    if (runtimeId != 0) {
        auto* subjectPlayer = playerByRuntimeId(runtimeId);
        if (subjectPlayer == nullptr)
            return false;
        subject = subjectPlayer->getRealName();
    } else {
        switch (packet.getId()) {
        case ::MinecraftPacketIds::PlayerList: {
            auto const& entries = unwrap(payloadOf<::PlayerListPacketPayload>(packet).mEntries);
            if (entries.empty())
                return false;

            // 一个包可能带多条（批量进服/离服）；只有"整包都是同一个人"时才按个体判定
            std::string onlyName;
            bool        allAdd = true;
            for (auto const& entry : entries) {
                ::mce::UUID uuid;
                bool        isAdd = false;
                if (auto const* add = ::std::get_if<::PlayerListPacketPayload::AddEntry>(&entry)) {
                    uuid  = unwrap(add->mUUID);
                    isAdd = true;
                } else if (auto const* rem = ::std::get_if<::PlayerListPacketPayload::RemoveEntry>(&entry)) {
                    uuid = unwrap(rem->mUUID);
                } else {
                    return false;
                }
                auto* p = playerByUuid(uuid);
                if (p == nullptr)
                    return false;
                if (onlyName.empty())
                    onlyName = p->getRealName();
                else if (onlyName != p->getRealName())
                    return false; // 混着多个人 → 放行, 不冒险
                allAdd = allAdd && isAdd;
            }
            subject  = onlyName;
            category = allAdd ? Category::Spawn : Category::Tab;
            break;
        }
        case ::MinecraftPacketIds::PlayerSkin: {
            auto const& payload     = payloadOf<::PlayerSkinPacketPayload>(packet);
            auto*       subjectByUu = playerByUuid(unwrap(payload.mUUID));
            if (subjectByUu == nullptr)
                return false;
            subject  = subjectByUu->getRealName();
            category = Category::Skin;
            break;
        }
        default:
            return false; // 认不出来 = 放行
        }
    }

    if (subject.empty())
        return false;

    std::lock_guard lock(mMutex);
    auto            it = mEntries.find(subject);
    if (it == mEntries.end())
        return false;

    if (subject == recipient.getRealName()) {
        return it->second.alsoSelf; // 发给本人：只有 alsoSelf 才丢
    }

    auto const& mask = it->second.mask;
    switch (category) {
    case Category::Spawn:
        return mask.spawn;
    case Category::Tab:
        return mask.tab;
    case Category::Motion:
        return mask.motion;
    case Category::State:
        return mask.state;
    case Category::Skin:
        return mask.skin;
    case Category::Unknown:
    default:
        return false;
    }
}

void PlayerVisibilityManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mEntries.erase(playerName);
}

void PlayerVisibilityManager::shutdown() {
    std::lock_guard lock(mMutex);
    mEntries.clear();
}

} // namespace debugshape_export
