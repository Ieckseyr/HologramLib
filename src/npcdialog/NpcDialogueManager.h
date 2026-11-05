#pragma once

#include "core/Registry.h"

#include <cstdint>
#include <deque>
#include <format>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "hologramlib/HologramLib.h"

class Player;

namespace debugshape_export {

// NPC 对话框
class NpcDialogueManager {
public:
    static NpcDialogueManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::NpcDialogSpec const& spec);
    bool    close(int64_t dialogId);

    bool update(int64_t dialogId, hologramlib::NpcDialogSpec const& spec);
    void closeAll();
    bool isOpen(int64_t dialogId) const;

    [[nodiscard]] bool getSpec(int64_t dialogId, hologramlib::NpcDialogSpec& out) const;

    bool                 setAvatar(int64_t dialogId, int skinVariant, std::string const& avatarViewSpec);
    std::vector<int64_t> getAllIds() const;

    uint64_t addClickListener(std::function<void(hologramlib::NpcDialogClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    std::vector<hologramlib::NpcDialogClickEvent> pollClicks();
    void                                          clearClicks();

    [[nodiscard]] static std::string formatClick(hologramlib::NpcDialogClickEvent const& event);

    bool handleNpcRequest(
        std::string const& playerName,
        std::uint64_t      npcId,
        int                requestType,
        int                actionIndex,
        std::string const& sceneName);

    void                        onPlayerLeave(std::string const& playerName);
    void                        shutdown();
    [[nodiscard]] std::uint64_t droppedClicks() const;

private:
    NpcDialogueManager() = default;

    struct Dialog {
        int64_t                    dialogId{0};
        std::string                playerName;
        std::uint64_t              carrierUniqueId{0};
        std::uint64_t              carrierRuntimeId{0};
        hologramlib::NpcDialogSpec spec;

        bool touched{false};
    };

    struct Carrier {
        std::uint64_t uniqueId{0};
        std::uint64_t runtimeId{0};
    };

    std::uint64_t ensureCarrier(
        ::Player&                         player,
        std::string const&                playerName,
        hologramlib::NpcDialogSpec const& spec,
        std::string const&                actionJson);

    void                               removeCarrierEntity(::Player& player, std::uint64_t uniqueId);
    [[nodiscard]] static std::uint64_t nextCarrierUniqueId();
    [[nodiscard]] static std::uint64_t nextCarrierRuntimeId();
    void                               sendDialog(::Player& player, Dialog const& dialog, bool open);
    void                               dispatch(hologramlib::NpcDialogClickEvent const& event);

    mutable std::mutex                                                                         mMutex;
    core::Registry<Dialog, core::NoLock>                                                       mDialogs;
    std::unordered_map<std::string, int64_t>                                                   mByPlayer;
    std::unordered_map<std::string, Carrier>                                                   mCarriers;
    std::unordered_map<uint64_t, std::function<void(hologramlib::NpcDialogClickEvent const&)>> mListeners;
    static constexpr std::size_t                                                               kClickQueueCap = 1024;
    std::atomic<std::uint64_t>                                                                 mDroppedClicks{0};
    std::deque<hologramlib::NpcDialogClickEvent>                                               mClickQueue;
    uint64_t                                                                                   mNextListenerToken{1};
};

hologramlib::INpcDialogue& npcDialogueAdapter();

} // namespace debugshape_export
