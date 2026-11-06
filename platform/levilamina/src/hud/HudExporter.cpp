#include "HudExporter.h"

#include "../actor/ActorEffectManager.h"
#include "../actor/PlayerRespawnManager.h"
#include "../anchor/AnchorPool.h"
#include "../effect/ActorEventFilter.h"
#include "../scoreboard/SidebarManager.h"
#include "../ui/LocatorBarManager.h"
#include "BossBarManager.h"
#include "HudManager.h"
#include "customentity/CustomEntityManager.h"
#include "lse/LseBridge.h"

#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

class BossBarAdapter final : public hologramlib::IBossBar {
public:
    int64_t add(
        std::string const&          playerName,
        std::string const&          title,
        float                       healthPercent,
        hologramlib::BossBarColor   color,
        hologramlib::BossBarOverlay overlay) override {
        return BossBarManager::getInstance().add(playerName, title, healthPercent, color, overlay);
    }
    bool setTitle(int64_t id, std::string const& title) override {
        return BossBarManager::getInstance().setTitle(id, title);
    }
    bool setHealth(int64_t id, float healthPercent) override {
        return BossBarManager::getInstance().setHealth(id, healthPercent);
    }
    bool setStyle(int64_t id, hologramlib::BossBarColor color, hologramlib::BossBarOverlay overlay) override {
        return BossBarManager::getInstance().setStyle(id, color, overlay);
    }
    bool remove(int64_t id) override { return BossBarManager::getInstance().remove(id); }
    void removeAll(std::string const& playerName) override {
        BossBarManager::getInstance().removeAll(playerName);
    }
    [[nodiscard]] int count(std::string const& playerName) const override {
        return BossBarManager::getInstance().count(playerName);
    }
};

class HudAdapter final : public hologramlib::IHud {
public:
    bool setElement(std::string const& playerName, hologramlib::HudElement element, bool visible) override {
        return HudManager::getInstance().setElement(playerName, element, visible);
    }
    int setElements(std::string const& playerName, int mask, bool visible) override {
        return HudManager::getInstance().setElements(playerName, mask, visible);
    }
    bool resetAll(std::string const& playerName) override {
        return HudManager::getInstance().resetAll(playerName);
    }
};

class LocatorBarExtAdapter final : public hologramlib::ILocatorBarExt {
public:
    bool setIcon(std::string const& playerName, std::string const& key, std::string const& texturePath) override {
        return LocatorBarManager::getInstance().setIcon(playerName, key, texturePath);
    }
    bool setIconSize(std::string const& playerName, std::string const& key, float width, float height) override {
        return LocatorBarManager::getInstance().setIconSize(playerName, key, width, height);
    }
    bool attachEntity(std::string const& playerName, std::string const& key, int64_t entityUniqueId) override {
        return LocatorBarManager::getInstance().attachEntity(playerName, key, entityUniqueId);
    }
    bool clearAttach(std::string const& playerName, std::string const& key) override {
        return LocatorBarManager::getInstance().clearAttach(playerName, key);
    }
    bool setGroup(std::string const& playerName, std::string const& key, std::string const& groupKey) override {
        return LocatorBarManager::getInstance().setGroup(playerName, key, groupKey);
    }
};

class SidebarAdapter final : public hologramlib::ISidebar {
public:
    bool set(std::string const& playerName, std::string const& title, std::vector<std::string> const& lines) override {
        return SidebarManager::getInstance().set(playerName, title, lines);
    }
    bool setTitle(std::string const& playerName, std::string const& title) override {
        return SidebarManager::getInstance().setTitle(playerName, title);
    }
    bool setLine(std::string const& playerName, int index, std::string const& text) override {
        return SidebarManager::getInstance().setLine(playerName, index, text);
    }
    bool remove(std::string const& playerName) override {
        return SidebarManager::getInstance().remove(playerName);
    }
    void               removeAll() override { SidebarManager::getInstance().removeAll(); }
    [[nodiscard]] bool isShown(std::string const& playerName) const override {
        return SidebarManager::getInstance().isShown(playerName);
    }
    [[nodiscard]] int lineCount(std::string const& playerName) const override {
        return SidebarManager::getInstance().lineCount(playerName);
    }
};

class ActorEffectFilterAdapter final : public hologramlib::IActorEffectFilter {
public:
    void block(int eventId) override { effect::ActorEventFilter::getInstance().block(eventId); }
    void unblock(int eventId) override { effect::ActorEventFilter::getInstance().unblock(eventId); }
    void blockForPlayer(std::string const& playerName, int eventId) override {
        effect::ActorEventFilter::getInstance().blockForPlayer(playerName, eventId);
    }
    void unblockForPlayer(std::string const& playerName, int eventId) override {
        effect::ActorEventFilter::getInstance().unblockForPlayer(playerName, eventId);
    }
    void blockForEntity(int64_t runtimeId, int eventId) override {
        effect::ActorEventFilter::getInstance().blockForEntity(runtimeId, eventId);
    }
    void unblockForEntity(int64_t runtimeId, int eventId) override {
        effect::ActorEventFilter::getInstance().unblockForEntity(runtimeId, eventId);
    }
    void                      clear() override { effect::ActorEventFilter::getInstance().clear(); }
    [[nodiscard]] std::string stats() const override { return effect::ActorEventFilter::getInstance().stats(); }
};

class PlayerRespawnAdapter final : public hologramlib::IPlayerRespawn {
public:
    bool respawn(std::string const& playerName, int state) override {
        return PlayerRespawnManager::getInstance().respawn(playerName, state);
    }
    bool respawnAt(std::string const& playerName, float x, float y, float z, int state) override {
        return PlayerRespawnManager::getInstance().respawnAt(playerName, x, y, z, state);
    }
};

class ActorEffectAdapter final : public hologramlib::IActorEffect {
public:
    bool playHurt(int64_t runtimeId) override { return ActorEffectManager::getInstance().playHurt(runtimeId); }
    bool playDeath(int64_t runtimeId) override { return ActorEffectManager::getInstance().playDeath(runtimeId); }
    bool playInstantDeath(int64_t runtimeId) override {
        return ActorEffectManager::getInstance().playInstantDeath(runtimeId);
    }
    bool play(int64_t runtimeId, int eventId, int data) override {
        return ActorEffectManager::getInstance().play(runtimeId, eventId, data);
    }
};

} // namespace

hologramlib::IBossBar& bossBarAdapter() {
    static BossBarAdapter adapter;
    return adapter;
}

hologramlib::IHud& hudAdapter() {
    static HudAdapter adapter;
    return adapter;
}

hologramlib::ILocatorBarExt& locatorBarExtAdapter() {
    static LocatorBarExtAdapter adapter;
    return adapter;
}

hologramlib::IActorEffect& actorEffectAdapter() {
    static ActorEffectAdapter adapter;
    return adapter;
}

hologramlib::ISidebar& sidebarAdapter() {
    static SidebarAdapter adapter;
    return adapter;
}

hologramlib::IActorEffectFilter& actorEffectFilterAdapter() {
    static ActorEffectFilterAdapter adapter;
    return adapter;
}

hologramlib::IPlayerRespawn& playerRespawnAdapter() {
    static PlayerRespawnAdapter adapter;
    return adapter;
}

void BossBarExporter::exportAll() {
    auto& mgr = BossBarManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "bossBarAdd",
        [&mgr](
            std::string const& playerName,
            std::string const& title,
            float              healthPercent,
            int                color,
            int                overlay) -> int64_t {
            return mgr.add(
                playerName,
                title,
                healthPercent,
                static_cast<hologramlib::BossBarColor>(color),
                static_cast<hologramlib::BossBarOverlay>(overlay));
        });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetTitle", [&mgr](int64_t id, std::string const& title) -> bool {
        return mgr.setTitle(id, title);
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetHealth", [&mgr](int64_t id, float percent) -> bool {
        return mgr.setHealth(id, percent);
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetStyle", [&mgr](int64_t id, int color, int overlay) -> bool {
        return mgr.setStyle(
            id,
            static_cast<hologramlib::BossBarColor>(color),
            static_cast<hologramlib::BossBarOverlay>(overlay));
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarRemove", [&mgr](int64_t id) -> bool {
        return mgr.remove(id);
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarRemoveAll", [&mgr](std::string const& playerName) {
        mgr.removeAll(playerName);
    });

    hologramlib::lse::exportAs(NAMESPACE, "anchorAcquire", [](std::string const& playerName) -> int64_t {
        auto level = ll::service::getLevel();
        if (!level)
            return -1;
        auto* player = level->getPlayer(playerName);
        if (player == nullptr)
            return -1;
        return AnchorPool::getInstance().acquire(static_cast<int>(player->getDimensionId()));
    });

    hologramlib::lse::exportAs(NAMESPACE, "anchorRelease", [](int64_t uniqueId) -> bool {
        return AnchorPool::getInstance().release(uniqueId);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "bossBarAddOn",
        [&mgr](
            std::string const& playerName,
            int64_t            targetId,
            std::string const& title,
            float              percent,
            int                color,
            int                overlay) -> int64_t {
            return mgr.addOn(
                playerName,
                targetId,
                title,
                percent,
                static_cast<hologramlib::BossBarColor>(color),
                static_cast<hologramlib::BossBarOverlay>(overlay));
        });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarProbe", [&mgr]() -> std::string {
        return mgr.probeWire();
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarGetInfo", [&mgr](int64_t id) -> std::string {
        return mgr.getDebugInfo(id);
    });

    hologramlib::lse::exportAs(NAMESPACE, "bossBarCount", [&mgr](std::string const& playerName) -> int {
        return mgr.count(playerName);
    });
}

void HudExporter::exportAll() {
    auto& mgr = HudManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "hudSetElement",
        [&mgr](std::string const& playerName, int element, bool visible) -> bool {
            return mgr.setElement(playerName, static_cast<hologramlib::HudElement>(element), visible);
        });

    hologramlib::lse::exportAs(NAMESPACE, "hudSetElements", [&mgr](std::string const& playerName, int mask, bool visible) -> int {
        return mgr.setElements(playerName, mask, visible);
    });

    hologramlib::lse::exportAs(NAMESPACE, "hudSetRaw", [&mgr](std::string const& playerName, int mask, int visibility) -> bool {
        return mgr.setRaw(playerName, mask, visibility);
    });

    hologramlib::lse::exportAs(NAMESPACE, "hudProbe", [&mgr]() -> std::string {
        return mgr.probeWire();
    });

    hologramlib::lse::exportAs(NAMESPACE, "hudResetAll", [&mgr](std::string const& playerName) -> bool {
        return mgr.resetAll(playerName);
    });
}

void LocatorBarExtExporter::exportAll() {
    auto& mgr = LocatorBarManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetEx",
        [&mgr](
            std::string const& playerName,
            std::string const& key,
            float              x,
            float              y,
            float              z,
            int                dim,
            int                color,
            bool               visible,
            std::string const& texturePath,
            float              iconWidth,
            float              iconHeight,
            bool               authority) -> bool {
            return mgr.setWaypointEx(
                playerName,
                key,
                x,
                y,
                z,
                dim,
                color,
                visible,
                texturePath,
                iconWidth,
                iconHeight,
                authority);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetClientAuthority",
        [&mgr](std::string const& playerName, std::string const& key, bool enabled) -> bool {
            return mgr.setClientAuthority(playerName, key, enabled);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetIcon",
        [&mgr](std::string const& playerName, std::string const& key, std::string const& texturePath) -> bool {
            return mgr.setIcon(playerName, key, texturePath);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetIconSize",
        [&mgr](std::string const& playerName, std::string const& key, float width, float height) -> bool {
            return mgr.setIconSize(playerName, key, width, height);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarAttachEntity",
        [&mgr](std::string const& playerName, std::string const& key, int64_t entityUniqueId) -> bool {
            return mgr.attachEntity(playerName, key, entityUniqueId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarClearAttach",
        [&mgr](std::string const& playerName, std::string const& key) -> bool {
            return mgr.clearAttach(playerName, key);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetGroup",
        [&mgr](std::string const& playerName, std::string const& key, std::string const& groupKey) -> bool {
            return mgr.setGroup(playerName, key, groupKey);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSyncGroup",
        [&mgr](std::string const& playerName, std::string const& groupKey) -> int {
            return mgr.syncGroup(playerName, groupKey);
        });
}

void SidebarExporter::exportAll() {
    auto& mgr = SidebarManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "sidebarSet",
        [&mgr](std::string const& playerName, std::string const& title, std::vector<std::string> lines) -> bool {
            return mgr.set(playerName, title, lines);
        });

    hologramlib::lse::exportAs(NAMESPACE, "sidebarSetTitle", [&mgr](std::string const& playerName, std::string const& title) -> bool {
        return mgr.setTitle(playerName, title);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "sidebarSetLine",
        [&mgr](std::string const& playerName, int index, std::string const& text) -> bool {
            return mgr.setLine(playerName, index, text);
        });

    hologramlib::lse::exportAs(NAMESPACE, "sidebarRemove", [&mgr](std::string const& playerName) -> bool {
        return mgr.remove(playerName);
    });

    hologramlib::lse::exportAs(NAMESPACE, "sidebarRemoveAll", [&mgr]() {
        mgr.removeAll();
    });

    hologramlib::lse::exportAs(NAMESPACE, "sidebarIsShown", [&mgr](std::string const& playerName) -> bool {
        return mgr.isShown(playerName);
    });

    hologramlib::lse::exportAs(NAMESPACE, "sidebarLineCount", [&mgr](std::string const& playerName) -> int {
        return mgr.lineCount(playerName);
    });
}

void ActorEffectExporter::exportAll() {
    auto& mgr = ActorEffectManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "actorPlayHurt", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playHurt(runtimeId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorPlayDeath", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playDeath(runtimeId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorPlayInstantDeath", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playInstantDeath(runtimeId);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayEvent",
        [&mgr](int64_t runtimeId, int eventId, int data) -> bool {
            return mgr.play(runtimeId, eventId, data);
        });

    hologramlib::lse::exportAs(NAMESPACE, "actorRuntimeIdOfPlayer", [](std::string const& playerName) -> int64_t {
        auto level = ll::service::getLevel();
        if (!level)
            return -1;
        auto* player = level->getPlayer(playerName);
        if (player == nullptr)
            return -1;
        return static_cast<int64_t>(player->getRuntimeID().rawID);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorRuntimeIdOfEntity", [](int64_t libId) -> int64_t {
        std::uint64_t runtimeId = 0;
        std::uint64_t uniqueId  = 0;
        if (!CustomEntityManager::getInstance().getIdPair(libId, uniqueId, runtimeId))
            return -1;
        return static_cast<int64_t>(runtimeId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerRespawn", [](std::string const& playerName) -> bool {
        return PlayerRespawnManager::getInstance().respawn(playerName, 2);
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerRespawnState", [](std::string const& playerName, int state) -> bool {
        return PlayerRespawnManager::getInstance().respawn(playerName, state);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerRespawnAt",
        [](std::string const& playerName, float x, float y, float z) -> bool {
            return PlayerRespawnManager::getInstance().respawnAt(playerName, x, y, z, 2);
        });

    hologramlib::lse::exportAs(NAMESPACE, "actorBlockEvent", [](int eventId) {
        effect::ActorEventFilter::getInstance().block(eventId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorUnblockEvent", [](int eventId) {
        effect::ActorEventFilter::getInstance().unblock(eventId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorBlockEventFor", [](std::string const& playerName, int eventId) {
        effect::ActorEventFilter::getInstance().blockForPlayer(playerName, eventId);
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "actorUnblockEventFor", [](std::string const& playerName, int eventId) {
            effect::ActorEventFilter::getInstance().unblockForPlayer(playerName, eventId);
        });

    hologramlib::lse::exportAs(NAMESPACE, "actorBlockEventForEntity", [](int64_t runtimeId, int eventId) {
        effect::ActorEventFilter::getInstance().blockForEntity(runtimeId, eventId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorUnblockEventForEntity", [](int64_t runtimeId, int eventId) {
        effect::ActorEventFilter::getInstance().unblockForEntity(runtimeId, eventId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorBlockClear", []() {
        effect::ActorEventFilter::getInstance().clear();
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorBlockStats", []() -> std::string {
        return effect::ActorEventFilter::getInstance().stats();
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayEventFor",
        [&mgr](std::string const& playerName, int64_t runtimeId, int eventId, int data) -> bool {
            return mgr.playFor(playerName, runtimeId, eventId, data);
        });

    hologramlib::lse::exportAs(NAMESPACE, "actorFreeze", [&mgr](int64_t runtimeId, float strength) -> bool {
        return mgr.setFreeze(runtimeId, strength);
    });

    hologramlib::lse::exportAs(NAMESPACE, "easterEggCreeper", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playCreeperSwelling(runtimeId);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorPlayTotem", [&mgr](int64_t runtimeId, int holdTicks) -> bool {
        return mgr.playTotem(runtimeId, holdTicks);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerShowEffect",
        [&mgr](std::string const& playerName, int effectId, int durationTicks, int amplifier) -> bool {
            return mgr.showEffect(playerName, effectId, durationTicks, amplifier);
        });

    hologramlib::lse::exportAs(NAMESPACE, "playerShowVillageHero", [&mgr](std::string const& playerName, int durationTicks) -> bool {
        (void)durationTicks;
        return mgr.playCelebrate(playerName, 3.0f);
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerShowBadOmen", [&mgr](std::string const& playerName, int durationTicks) -> bool {
        (void)durationTicks;
        return mgr.playOminousDrink(playerName, 40);
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerPlayCelebrate", [&mgr](std::string const& playerName, float seconds) -> bool {
        return mgr.playCelebrate(playerName, seconds);
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerPlayOminousDrink", [&mgr](std::string const& playerName, int holdTicks) -> bool {
        return mgr.playOminousDrink(playerName, holdTicks);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorSetPose", [&mgr](int64_t runtimeId, int poseIndex) -> bool {
        return mgr.setPose(runtimeId, poseIndex);
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorUnfreeze", [&mgr](int64_t runtimeId) -> bool {
        return mgr.setFreeze(runtimeId, 0.0f);
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayAnimation",
        [&mgr](int64_t runtimeId, std::string const& animation, std::string const& stopExpression, float blendOutTime) -> bool {
            return mgr.playAnimation(runtimeId, animation, "none", stopExpression, blendOutTime, "", 13);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayAnimationFull",
        [&mgr](
            int64_t            runtimeId,
            std::string const& animation,
            std::string const& nextState,
            std::string const& stopExpression,
            float              blendOutTime,
            std::string const& controller,
            int                molangVersion
        ) -> bool {
            return mgr.playAnimation(
                runtimeId,
                animation,
                nextState,
                stopExpression,
                blendOutTime,
                controller,
                molangVersion
            );
        });
}

} // namespace debugshape_export
