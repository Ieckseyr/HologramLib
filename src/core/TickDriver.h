#pragma once

#include <atomic>
#include <cstddef>
#include <string>

namespace debugshape_export {
namespace tick {

void installDriver();

using Callback = void (*)();

enum class Order : int {
    PlayerNpc     = 10,
    CustomEntity  = 20,
    ItemDisplay   = 30,
    ParticleShape = 40,
    ViewOverride  = 50,
};

// 每 tick 调度
class TickDriver {
public:
    static TickDriver& getInstance() noexcept;

    bool add(char const* name, Callback cb, Order order);

    void run();

    std::size_t size() const noexcept;

    std::string describe() const;

    static constexpr std::size_t kMax = 16;

private:
    TickDriver() = default;

    struct Entry {
        char const* name{};
        Callback    cb{};
        int         order{};
    };

    Entry                    mEntries[kMax]{};
    std::size_t              mCount{};
    std::atomic<std::size_t> mPublished{0};
};

struct AutoRegister {
    AutoRegister(char const* name, Callback cb, Order order) {
        (void)TickDriver::getInstance().add(name, cb, order);
    }
};

} // namespace tick
} // namespace debugshape_export
