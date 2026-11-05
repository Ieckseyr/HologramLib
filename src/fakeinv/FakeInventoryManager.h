#pragma once

#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

// 伪造背包
class FakeInventoryManager {
public:
    static FakeInventoryManager& getInstance();

    bool apply(std::string const& playerName, hologramlib::FakeInventorySpec const& spec);
    bool setSlot(std::string const& playerName, int slot, hologramlib::ContainerMenuItem const& item);
    bool refresh(std::string const& playerName);
    bool clear(std::string const& playerName);
    void clearAll();

    [[nodiscard]] bool                     isActive(std::string const& playerName) const;
    [[nodiscard]] std::vector<std::string> getActivePlayers() const;

    uint64_t addClickListener(std::function<void(hologramlib::FakeInventoryClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    std::vector<hologramlib::FakeInventoryClickEvent> pollClicks();
    void                                              clearClicks();

    [[nodiscard]] static std::string formatClick(hologramlib::FakeInventoryClickEvent const& event);

    bool handleInventoryAction(std::string const& playerName, int containerEnum, int slot);

    void                        onPlayerLeave(std::string const& playerName);
    void                        shutdown();
    [[nodiscard]] std::uint64_t droppedClicks() const;

    void tick();

private:
    FakeInventoryManager() = default;

    struct Session {
        std::string                    playerName;
        hologramlib::FakeInventorySpec spec;
        int                            ticksSinceRefresh{0};
    };

    [[nodiscard]] static bool slotIsFaked(Session const& session, int slot);

    Session* findLocked(std::string const& playerName);

    mutable std::mutex                                                                             mMutex;
    std::unordered_map<std::string, Session>                                                       mSessions;
    std::unordered_map<uint64_t, std::function<void(hologramlib::FakeInventoryClickEvent const&)>> mListeners;
    static constexpr std::size_t                                                                   kClickQueueCap = 1024;
    std::atomic<std::uint64_t>                                                                     mDroppedClicks{0};
    std::deque<hologramlib::FakeInventoryClickEvent>                                               mClickQueue;
    uint64_t                                                                                       mNextListenerToken{1};
};

hologramlib::IFakeInventory& fakeInventoryAdapter();

} // namespace debugshape_export
