// HudExporter.cpp - Boss 条 / HUD / 方位条扩展 / 实体表现事件
#include "HudExporter.h"

#include "../actor/ActorEffectManager.h"
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

// ── 适配器 ─────────────────────────────────────────────────────────────

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

// ── LSE 导出 ───────────────────────────────────────────────────────────

void BossBarExporter::exportAll() {
    auto& mgr = BossBarManager::getInstance();

    // bossBarAdd(playerName, title, healthPercent, color, overlay) -> id
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

    // bossBarSetTitle(id, title) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetTitle", [&mgr](int64_t id, std::string const& title) -> bool {
        return mgr.setTitle(id, title);
    });

    // bossBarSetHealth(id, healthPercent) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetHealth", [&mgr](int64_t id, float percent) -> bool {
        return mgr.setHealth(id, percent);
    });

    // bossBarSetStyle(id, color, overlay) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "bossBarSetStyle", [&mgr](int64_t id, int color, int overlay) -> bool {
        return mgr.setStyle(
            id,
            static_cast<hologramlib::BossBarColor>(color),
            static_cast<hologramlib::BossBarOverlay>(overlay));
    });

    // bossBarRemove(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "bossBarRemove", [&mgr](int64_t id) -> bool {
        return mgr.remove(id);
    });

    // bossBarRemoveAll(playerName) -> null
    hologramlib::lse::exportAs(NAMESPACE, "bossBarRemoveAll", [&mgr](std::string const& playerName) {
        mgr.removeAll(playerName);
    });

    // bossBarCount(playerName) -> i
    hologramlib::lse::exportAs(NAMESPACE, "bossBarCount", [&mgr](std::string const& playerName) -> int {
        return mgr.count(playerName);
    });
}

void HudExporter::exportAll() {
    auto& mgr = HudManager::getInstance();

    // hudSetElement(playerName, element, visible) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "hudSetElement",
        [&mgr](std::string const& playerName, int element, bool visible) -> bool {
            return mgr.setElement(playerName, static_cast<hologramlib::HudElement>(element), visible);
        });

    // hudSetElements(playerName, mask, visible) -> i（bit0=纸娃娃 … bit12=物品名）
    hologramlib::lse::exportAs(NAMESPACE, "hudSetElements", [&mgr](std::string const& playerName, int mask, bool visible) -> int {
        return mgr.setElements(playerName, mask, visible);
    });

    // hudResetAll(playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "hudResetAll", [&mgr](std::string const& playerName) -> bool {
        return mgr.resetAll(playerName);
    });
}

void LocatorBarExtExporter::exportAll() {
    auto& mgr = LocatorBarManager::getInstance();

    // locatorBarSetIcon(playerName, key, texturePath) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetIcon",
        [&mgr](std::string const& playerName, std::string const& key, std::string const& texturePath) -> bool {
            return mgr.setIcon(playerName, key, texturePath);
        });

    // locatorBarSetIconSize(playerName, key, width, height) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetIconSize",
        [&mgr](std::string const& playerName, std::string const& key, float width, float height) -> bool {
            return mgr.setIconSize(playerName, key, width, height);
        });

    // locatorBarAttachEntity(playerName, key, entityUniqueId) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarAttachEntity",
        [&mgr](std::string const& playerName, std::string const& key, int64_t entityUniqueId) -> bool {
            return mgr.attachEntity(playerName, key, entityUniqueId);
        });

    // locatorBarClearAttach(playerName, key) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarClearAttach",
        [&mgr](std::string const& playerName, std::string const& key) -> bool {
            return mgr.clearAttach(playerName, key);
        });

    // locatorBarSetGroup(playerName, key, groupKey) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSetGroup",
        [&mgr](std::string const& playerName, std::string const& key, std::string const& groupKey) -> bool {
            return mgr.setGroup(playerName, key, groupKey);
        });

    // locatorBarSyncGroup(playerName, groupKey) -> i（同组航点合成一条包发出去）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSyncGroup",
        [&mgr](std::string const& playerName, std::string const& groupKey) -> int {
            return mgr.syncGroup(playerName, groupKey);
        });
}

void SidebarExporter::exportAll() {
    auto& mgr = SidebarManager::getInstance();

    // sidebarSet(playerName, title, [lines]) -> bool（空数组 = 移除）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "sidebarSet",
        [&mgr](std::string const& playerName, std::string const& title, std::vector<std::string> lines) -> bool {
            return mgr.set(playerName, title, lines);
        });

    // sidebarSetTitle(playerName, title) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "sidebarSetTitle", [&mgr](std::string const& playerName, std::string const& title) -> bool {
        return mgr.setTitle(playerName, title);
    });

    // sidebarSetLine(playerName, index, text) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "sidebarSetLine",
        [&mgr](std::string const& playerName, int index, std::string const& text) -> bool {
            return mgr.setLine(playerName, index, text);
        });

    // sidebarRemove(playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "sidebarRemove", [&mgr](std::string const& playerName) -> bool {
        return mgr.remove(playerName);
    });

    // sidebarRemoveAll() -> null
    hologramlib::lse::exportAs(NAMESPACE, "sidebarRemoveAll", [&mgr]() {
        mgr.removeAll();
    });

    // sidebarIsShown(playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "sidebarIsShown", [&mgr](std::string const& playerName) -> bool {
        return mgr.isShown(playerName);
    });

    // sidebarLineCount(playerName) -> i
    hologramlib::lse::exportAs(NAMESPACE, "sidebarLineCount", [&mgr](std::string const& playerName) -> int {
        return mgr.lineCount(playerName);
    });
}

void ActorEffectExporter::exportAll() {
    auto& mgr = ActorEffectManager::getInstance();

    // actorPlayHurt(runtimeId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "actorPlayHurt", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playHurt(runtimeId);
    });

    // actorPlayDeath(runtimeId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "actorPlayDeath", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playDeath(runtimeId);
    });

    // actorPlayInstantDeath(runtimeId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "actorPlayInstantDeath", [&mgr](int64_t runtimeId) -> bool {
        return mgr.playInstantDeath(runtimeId);
    });

    // actorPlayEvent(runtimeId, eventId, data) -> bool（eventId 见协议 ActorEvent）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayEvent",
        [&mgr](int64_t runtimeId, int eventId, int data) -> bool {
            return mgr.play(runtimeId, eventId, data);
        });

    // actorRuntimeIdOfPlayer(playerName) -> int64（玩家实体的 runtimeId；不在线 -1）
    hologramlib::lse::exportAs(NAMESPACE, "actorRuntimeIdOfPlayer", [](std::string const& playerName) -> int64_t {
        auto level = ll::service::getLevel();
        if (!level)
            return -1;
        auto* player = level->getPlayer(playerName);
        if (player == nullptr)
            return -1;
        return static_cast<int64_t>(player->getRuntimeID().rawID);
    });

    // actorRuntimeIdOfEntity(libId) -> int64（协议层实体的 runtimeId；没有 -1）
    hologramlib::lse::exportAs(NAMESPACE, "actorRuntimeIdOfEntity", [](int64_t libId) -> int64_t {
        std::uint64_t runtimeId = 0;
        std::uint64_t uniqueId  = 0;
        if (!CustomEntityManager::getInstance().getIdPair(libId, uniqueId, runtimeId))
            return -1;
        return static_cast<int64_t>(runtimeId);
    });

    // actorPlayEventFor(playerName, runtimeId, eventId, data) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "actorPlayEventFor",
        [&mgr](std::string const& playerName, int64_t runtimeId, int eventId, int data) -> bool {
            return mgr.playFor(playerName, runtimeId, eventId, data);
        });
}

} // namespace debugshape_export
