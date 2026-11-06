#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

// 假实体交互路由
class GhostInteractRouter {
public:
    static GhostInteractRouter& getInstance();

    void setListener(std::function<void(hologramlib::GhostInteractEvent const&)> listener);
    void clearListener();

    std::uint64_t addListener(std::function<void(hologramlib::GhostInteractEvent const&)> listener);
    bool          removeListener(std::uint64_t token);

    std::uint64_t addActorListener(std::function<void(hologramlib::ActorInteractEvent const&)> listener);
    bool          removeActorListener(std::uint64_t token);

    std::vector<hologramlib::GhostInteractEvent> poll();
    void                                         clearQueue();

    std::vector<hologramlib::ActorInteractEvent> pollActor();
    void                                         clearActorQueue();

    void dispatch(hologramlib::GhostInteractEvent const& ev);

    void dispatchActor(hologramlib::ActorInteractEvent const& ev);

    [[nodiscard]] static std::string formatEvent(hologramlib::GhostInteractEvent const& ev);
    [[nodiscard]] static std::string formatActorEvent(hologramlib::ActorInteractEvent const& ev);

    static void exportLse();

private:
    GhostInteractRouter()  = default;
    ~GhostInteractRouter() = default;

    struct TaggedListener {
        std::uint64_t                                               token;
        std::function<void(hologramlib::GhostInteractEvent const&)> fn;
    };

    struct TaggedActorListener {
        std::uint64_t                                               token;
        std::function<void(hologramlib::ActorInteractEvent const&)> fn;
    };

    std::mutex                                                  mMutex;
    std::uint64_t                                               mNextToken = 1;
    std::function<void(hologramlib::GhostInteractEvent const&)> mListener;
    std::vector<TaggedListener>                                 mListeners;
    std::vector<TaggedActorListener>                            mActorListeners;
    std::deque<hologramlib::GhostInteractEvent>                 mQueue;
    std::deque<hologramlib::ActorInteractEvent>                 mActorQueue;
};

} // namespace debugshape_export
