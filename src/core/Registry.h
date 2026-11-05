#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace debugshape_export {
namespace core {

struct NoLock {
    constexpr void lock() noexcept {}
    constexpr void unlock() noexcept {}
};

template <class T, class Mutex = std::mutex>
// 带锁的对象表
class Registry {
public:
    using Id     = std::int64_t;
    using IdList = std::vector<Id>;

    explicit Registry(bool dirtyOnInsert = false)
        : mDirtyOnInsert(dirtyOnInsert) {}

    Registry(Registry const&)            = delete;
    Registry& operator=(Registry const&) = delete;

    // 未指定维度传 nullopt
    Id insert(std::unique_ptr<T> object, std::uint64_t runtimeId = 0, std::optional<int> dimId = std::nullopt) {
        std::lock_guard lock(mMutex);
        Id const        id = mNextId++;
        mById[id]          = std::move(object);
        if (runtimeId != 0) {
            mByRuntimeId[runtimeId] = id;
        }
        if (dimId.has_value()) {
            indexDimensionLocked(id, *dimId);
        }
        if (mDirtyOnInsert)
            mDirty.push_back(id);
        return id;
    }

    bool insertWithId(Id id, std::unique_ptr<T> object, std::uint64_t runtimeId = 0, std::optional<int> dimId = std::nullopt) {
        std::lock_guard lock(mMutex);
        if (mById.contains(id))
            return false;
        mById[id] = std::move(object);
        if (id >= mNextId)
            mNextId = id + 1;
        if (runtimeId != 0)
            mByRuntimeId[runtimeId] = id;
        if (dimId.has_value())
            indexDimensionLocked(id, *dimId);
        if (mDirtyOnInsert)
            mDirty.push_back(id);
        return true;
    }

    bool erase(Id id) {
        std::lock_guard lock(mMutex);
        auto            it = mById.find(id);
        if (it == mById.end())
            return false;
        eraseIndexesLocked(id);
        mById.erase(it);
        return true;
    }

    bool exists(Id id) const {
        std::lock_guard lock(mMutex);
        return mById.contains(id);
    }

    void clear() {
        std::lock_guard lock(mMutex);
        mById.clear();
        mByRuntimeId.clear();
        mByDimension.clear();
        mDirty.clear();
    }

    bool empty() const {
        std::lock_guard lock(mMutex);
        return mById.empty();
    }

    std::size_t size() const {
        std::lock_guard lock(mMutex);
        return mById.size();
    }

    template <class Fn>
    bool withObject(Id id, Fn&& fn) {
        std::lock_guard lock(mMutex);
        auto            it = mById.find(id);
        if (it == mById.end())
            return false;
        if constexpr (std::is_void_v<std::invoke_result_t<Fn&, T&>>) {
            fn(*it->second);
            return true;
        } else {
            return static_cast<bool>(fn(*it->second));
        }
    }

    template <class Fn>
    bool withObject(Id id, Fn&& fn) const {
        std::lock_guard lock(mMutex);
        auto            it = mById.find(id);
        if (it == mById.end())
            return false;
        if constexpr (std::is_void_v<std::invoke_result_t<Fn&, T const&>>) {
            fn(*it->second);
            return true;
        } else {
            return static_cast<bool>(fn(*it->second));
        }
    }

    template <class Fn>
    void forEachLocked(Fn&& fn) const {
        std::lock_guard lock(mMutex);
        for (auto const& [id, object] : mById) {
            fn(id, *object);
        }
    }

    T* peek(Id id) noexcept {
        static_assert(std::is_same_v<Mutex, NoLock>, "peek() 只适用于 NoLock 表：加锁由调用方负责");
        auto it = mById.find(id);
        return it == mById.end() ? nullptr : it->second.get();
    }

    T const* peek(Id id) const noexcept {
        static_assert(std::is_same_v<Mutex, NoLock>, "peek() 只适用于 NoLock 表：加锁由调用方负责");
        auto it = mById.find(id);
        return it == mById.end() ? nullptr : it->second.get();
    }

    std::optional<Id> findByRuntimeId(std::uint64_t runtimeId) const {
        std::lock_guard lock(mMutex);
        auto            it = mByRuntimeId.find(runtimeId);
        if (it == mByRuntimeId.end())
            return std::nullopt;
        return it->second;
    }

    void setDimension(Id id, int newDim) {
        std::lock_guard lock(mMutex);
        auto            it = mById.find(id);
        if (it == mById.end())
            return;
        unindexDimensionLocked(id);
        indexDimensionLocked(id, newDim);
    }

    IdList snapshotByDimension(int dimId) const {
        std::lock_guard lock(mMutex);
        auto            it = mByDimension.find(dimId);
        if (it == mByDimension.end())
            return {};
        return it->second;
    }

    IdList snapshotAll() const {
        std::lock_guard lock(mMutex);
        IdList          out;
        out.reserve(mById.size());
        for (auto const& [id, _] : mById) {
            out.push_back(id);
        }
        return out;
    }

    void markDirty(Id id) {
        std::lock_guard lock(mMutex);
        mDirty.push_back(id);
    }

    void markAllDirty() {
        std::lock_guard lock(mMutex);
        for (auto const& [id, _] : mById) {
            mDirty.push_back(id);
        }
    }

    IdList takeDirty() {
        std::lock_guard lock(mMutex);
        IdList          out;
        out.reserve(mDirty.size());
        for (Id id : mDirty) {
            if (mById.contains(id))
                out.push_back(id);
        }
        mDirty.clear();
        return out;
    }

    std::size_t dirtyCount() const {
        std::lock_guard lock(mMutex);
        return mDirty.size();
    }

private:
    void indexDimensionLocked(Id id, int dim) {
        auto& ids = mByDimension[dim];
        if (std::find(ids.begin(), ids.end(), id) == ids.end())
            ids.push_back(id);
    }

    void unindexDimensionLocked(Id id) {
        for (auto it = mByDimension.begin(); it != mByDimension.end();) {
            auto& ids = it->second;
            ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
            it = ids.empty() ? mByDimension.erase(it) : std::next(it);
        }
    }

    void eraseIndexesLocked(Id id) {
        for (auto it = mByRuntimeId.begin(); it != mByRuntimeId.end();) {
            if (it->second == id) {
                it = mByRuntimeId.erase(it);
            } else {
                ++it;
            }
        }
        unindexDimensionLocked(id);
    }

    mutable Mutex                              mMutex;
    Id                                         mNextId{1};
    bool                                       mDirtyOnInsert{false};
    std::unordered_map<Id, std::unique_ptr<T>> mById;
    std::unordered_map<std::uint64_t, Id>      mByRuntimeId;
    std::unordered_map<int, IdList>            mByDimension;
    IdList                                     mDirty;
};

} // namespace core
} // namespace debugshape_export
