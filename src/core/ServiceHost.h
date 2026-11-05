#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "DiagLog.h"

namespace debugshape_export {
namespace core {

struct Domain {
    char const* name;
    bool (*start)();
    void (*stop)();
    void (*exportLse)();
};

// 域注册与启停
class ServiceHost {
public:
    static constexpr std::size_t kMaxDomains = 40;

    void add(Domain domain) noexcept {
        if (mCount < kMaxDomains) {
            mDomains[mCount++] = domain;
        }
    }

    // 返回 false 表示启动失败,回滚
    bool startAll() {
        for (std::size_t i = 0; i < mCount; ++i) {
            Domain const& d = mDomains[i];
            if (d.start != nullptr && !d.start()) {
                HLIB_LOG_ERROR("[host] domain '{}' failed to start, rolling back", d.name);
                stopFrom(i);
                return false;
            }
            HLIB_LOG_INFO("[host] started '{}'", d.name);
        }
        return true;
    }

    void stopAll() noexcept { stopFrom(mCount); }

    void exportAll() {
        for (std::size_t i = 0; i < mCount; ++i) {
            if (mDomains[i].exportLse != nullptr) {
                mDomains[i].exportLse();
            }
        }
    }

    void clear() noexcept { mCount = 0; }

    [[nodiscard]] std::size_t size() const noexcept { return mCount; }

    [[nodiscard]] std::string describe() const {
        std::string out;
        for (std::size_t i = 0; i < mCount; ++i) {
            if (!out.empty()) {
                out += ", ";
            }
            out += mDomains[i].name;
        }
        return out;
    }

private:
    void stopFrom(std::size_t exclusiveEnd) noexcept {
        while (exclusiveEnd > 0) {
            --exclusiveEnd;
            Domain const& d = mDomains[exclusiveEnd];
            if (d.stop != nullptr) {
                d.stop();
            }
            HLIB_LOG_INFO("[host] stopped '{}'", d.name);
        }
        mCount = 0;
    }

    std::array<Domain, kMaxDomains> mDomains{};
    std::size_t                     mCount{};
};

} // namespace core
} // namespace debugshape_export
