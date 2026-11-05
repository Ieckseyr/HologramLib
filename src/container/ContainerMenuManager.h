#pragma once

#include "core/Registry.h"

#include "hologramlib/HologramLib.h"

#include <ll/api/io/Logger.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

// 虚拟容器菜单
class ContainerMenuManager {
public:
    static ContainerMenuManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::ContainerMenuSpec const& spec);

    bool update(int64_t menuId, hologramlib::ContainerMenuSpec const& spec);

    // 取当前生效的 spec（含逐格 setItem 的累积结果），供 LSE 的「整菜单重发」用
    [[nodiscard]] bool getSpec(int64_t menuId, hologramlib::ContainerMenuSpec& out) const;

    bool setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item);

    bool setItemEnchanted(
        int64_t                                 menuId,
        int                                     slot,
        hologramlib::ContainerMenuItem const&   item,
        std::vector<std::pair<int, int>> const& enchants);

    bool setInteractive(int64_t menuId, bool on);

    bool setTitle(int64_t menuId, std::string const& title);
    bool close(int64_t menuId);
    void closeAll();

    bool blockEvent(int64_t menuId, int b0, int b1);

    [[nodiscard]] bool                 isOpen(int64_t menuId) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;

    uint64_t addClickListener(std::function<void(hologramlib::ContainerClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    std::vector<hologramlib::ContainerClickEvent> pollClicks();
    void                                          clearClicks();

    [[nodiscard]] static std::string formatClick(hologramlib::ContainerClickEvent const& event);

    bool handleSlotAction(std::string const& playerName, int containerEnum, int containerId, int slot);

    struct RequestSlot {
        int container{0};
        int containerId{-1};
        int slot{-1};
    };

    bool handleInteractiveRequest(std::string const& playerName, std::vector<RequestSlot> const& slots, int amount);

    bool handleContainerClose(std::string const& playerName, int containerId);

    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;

    [[nodiscard]] int64_t menuIdFor(std::string const& playerName) const;

    void                        onPlayerLeave(std::string const& playerName);
    void                        shutdown();
    [[nodiscard]] std::uint64_t droppedClicks() const;

private:
    ContainerMenuManager() = default;

    struct Menu {
        int64_t     menuId{0};
        std::string playerName;
        int         containerId{0};

        int posX{0};
        int posY{0};
        int posZ{0};

        int64_t                        carrierLibId{-1};
        std::uint64_t                  carrierUniqueId{0};
        std::uint64_t                  carrierRuntimeId{0};
        hologramlib::ContainerMenuSpec spec;

        bool uiOpen{false};

        std::uint64_t                  ownerEpoch{0};
        hologramlib::ContainerMenuItem cursor;

        std::unordered_map<int, std::vector<std::pair<int, int>>> enchantsBySlot;

        [[nodiscard]] bool isBig() const { return spec.rows >= 6; }
        [[nodiscard]] int  slotCount() const { return isBig() ? 54 : 27; }
    };

    int allocateContainerId();

    void dispatch(hologramlib::ContainerClickEvent const& event);

    void applyInteractiveSlots(Menu& menu, std::vector<RequestSlot> const& slots, int amount);

    void sendChestBlocks(::Player& player, Menu const& menu);
    void sendChestBlockActor(::Player& player, Menu const& menu);

    void resendBlockActor(::Player& player, Menu const& menu);
    void sendContainerOpen(::Player& player, Menu const& menu);

    void restoreRealBlocks(::Player& player, Menu const& menu);

    void sendMinecartCarrier(::Player& player, Menu const& menu);

    void               laterSendBlocks(std::string playerName, int64_t menuId);
    void               laterSendOpen(std::string playerName, int64_t menuId);
    void               laterMinecartOpen(std::string playerName, int64_t menuId);
    [[nodiscard]] bool snapshotMenu(int64_t menuId, Menu& out) const;

    [[nodiscard]] bool isCurrentEpoch(Menu const& menu) const;

    mutable std::mutex                                                                         mMutex;
    core::Registry<Menu, core::NoLock>                                                         mMenus;
    std::unordered_map<std::string, int64_t>                                                   mByPlayer;
    std::unordered_map<uint64_t, std::function<void(hologramlib::ContainerClickEvent const&)>> mListeners;
    static constexpr std::size_t                                                               kClickQueueCap = 1024;
    std::atomic<std::uint64_t>                                                                 mDroppedClicks{0};
    std::deque<hologramlib::ContainerClickEvent>                                               mClickQueue;
    int                                                                                        mNextContainerId{101};
    std::unordered_map<std::string, std::uint64_t>                                             mPlayerEpoch;
    std::unordered_map<int, std::uint64_t>                                                     mIdFreedTick;
    uint64_t                                                                                   mNextListenerToken{1};
};

hologramlib::IContainerMenu& containerMenuAdapter();

} // namespace debugshape_export
