#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

// 虚拟锚点实体池
class AnchorPool {
public:
    static AnchorPool& getInstance();

    std::int64_t acquire(int dimension);

    bool release(std::int64_t uniqueId);
    void releaseAll();

    [[nodiscard]] int  count() const;
    [[nodiscard]] bool exists(std::int64_t uniqueId) const;

    void shutdown();

private:
    AnchorPool() = default;

    mutable std::mutex                             mMutex;
    std::unordered_map<std::int64_t, std::int64_t> mAnchors;
};

} // namespace debugshape_export
