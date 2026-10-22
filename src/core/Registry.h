// Registry.h - 一个域的"协议对象表"（内核层）
//
// 收敛前 8 个域各自手写同一套东西: unordered_map<int64_t, unique_ptr<T>> + mNextId + mMutex
// + 维度/玩家索引 + 脏标记 + 进服重发。这套板子有四个容易写错的点, 现在由本类一次性保证:
//
//   1. id 单调递增、**不复用**。复用会让迟到的包/事件打到新对象上（销毁后 200 tick 内
//      迟到的 UI 包是真实场景）。erase 之后 id 也不会回填。
//   2. 索引与主表同生共死。runtimeId / 维度索引在 erase 里一并清掉 —— 手写时最容易漏一个。
//   3. 脏集是"取走"语义（takeDirty）, 不是"读后清"。两个 tick 挤在一起时不会漏刷新。
//   4. **回调里不得再调本表**: 所有对外遍历都走快照或 forEachLocked, 而这两者都在锁内。
//      要发包就先把 id 列表拷出来, 解锁后再发 —— 这条是 674c7b3 那次死锁的根因
//      （持锁发包 → 发包钩子重入 → recursive_mutex 只是把死锁变成"能跑但状态可能半更新"）。
//
// 本文件**只依赖标准库**, 不 include 任何 mc/ll 头: 因此能在 CI 里脱离 BDS 单独跑
// tests/registry_test.cpp。域特定语义（跟随、逐观看者形状、动画队列）不进这里。
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace debugshape_export {
namespace core {

template <class T>
class Registry {
public:
    using Id = std::int64_t;
    using IdList = std::vector<Id>;

    /// dirtyOnInsert = true 时每次插入自动标脏（逐 tick 刷新的渲染域用）；
    /// false（默认）适合"只在增删时动作"的域，避免脏集只增不取。
    explicit Registry(bool dirtyOnInsert = false)
        : mDirtyOnInsert(dirtyOnInsert) {}

    Registry(Registry const&) = delete;
    Registry& operator=(Registry const&) = delete;

    // ── 生命周期 ──────────────────────────────────────────────────────────────
    /// 分配新 id 并插入。runtimeId/dimId 为 0 表示"不进索引"（多数域只在需要反查时才给）。
    Id insert(std::unique_ptr<T> object, std::uint64_t runtimeId = 0, int dimId = 0) {
        std::lock_guard lock(mMutex);
        Id const id = mNextId++;
        mById[id] = std::move(object);
        if (runtimeId != 0) {
            mByRuntimeId[runtimeId] = id;
        }
        if (dimId != 0) {
            mByDimension[dimId].push_back(id);
        }
        if (mDirtyOnInsert)
            mDirty.push_back(id);
        return id;
    }

    /// 指定 id 插入（消费方自带 id 的域用; 已存在则失败）
    bool insertWithId(Id id, std::unique_ptr<T> object, std::uint64_t runtimeId = 0, int dimId = 0) {
        std::lock_guard lock(mMutex);
        if (mById.contains(id))
            return false;
        mById[id] = std::move(object);
        if (id >= mNextId)
            mNextId = id + 1;
        if (runtimeId != 0)
            mByRuntimeId[runtimeId] = id;
        if (dimId != 0)
            mByDimension[dimId].push_back(id);
        if (mDirtyOnInsert)
            mDirty.push_back(id);
        return true;
    }

    bool erase(Id id) {
        std::lock_guard lock(mMutex);
        auto it = mById.find(id);
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

    std::size_t size() const {
        std::lock_guard lock(mMutex);
        return mById.size();
    }

    // ── 查询（指针只在锁内有效: 要跨调用用就拷值）────────────────────────────
    /// 在锁内访问单个对象; fn 返回 false 时表示"不需要再继续"（这里是单对象, 仅作语义统一）
    template <class Fn>
    bool withObject(Id id, Fn&& fn) {
        std::lock_guard lock(mMutex);
        auto it = mById.find(id);
        if (it == mById.end())
            return false;
        fn(*it->second);
        return true;
    }

    template <class Fn>
    bool withObject(Id id, Fn&& fn) const {
        std::lock_guard lock(mMutex);
        auto it = mById.find(id);
        if (it == mById.end())
            return false;
        fn(*it->second);
        return true;
    }

    /// 遍历全部（**在锁内**）; 只做纯计算/写快照, 不要在这里发包
    template <class Fn>
    void forEachLocked(Fn&& fn) const {
        std::lock_guard lock(mMutex);
        for (auto const& [id, object] : mById) {
            fn(id, *object);
        }
    }

    // ── 索引 ──────────────────────────────────────────────────────────────────
    std::optional<Id> findByRuntimeId(std::uint64_t runtimeId) const {
        std::lock_guard lock(mMutex);
        auto it = mByRuntimeId.find(runtimeId);
        if (it == mByRuntimeId.end())
            return std::nullopt;
        return it->second;
    }

    /// 改维度时同步索引（漏调 = 按维度重发会漏对象）
    void setDimension(Id id, int newDim) {
        std::lock_guard lock(mMutex);
        auto it = mById.find(id);
        if (it == mById.end())
            return;
        for (auto& [dim, ids] : mByDimension) {
            auto pos = std::find(ids.begin(), ids.end(), id);
            if (pos != ids.end()) {
                ids.erase(pos);
                break;
            }
        }
        if (newDim != 0)
            mByDimension[newDim].push_back(id);
    }

    IdList snapshotByDimension(int dimId) const {
        std::lock_guard lock(mMutex);
        auto it = mByDimension.find(dimId);
        if (it == mByDimension.end())
            return {};
        return it->second;
    }

    /// 全量 id 快照（进服重发/关停清理用）—— 拿到之后在锁外逐个 withObject + 发包
    IdList snapshotAll() const {
        std::lock_guard lock(mMutex);
        IdList out;
        out.reserve(mById.size());
        for (auto const& [id, _] : mById) {
            out.push_back(id);
        }
        return out;
    }

    // ── 脏标记 ────────────────────────────────────────────────────────────────
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

    /// 取走脏集（每 tick 一次）; 已销毁的 id 在这里被过滤掉
    IdList takeDirty() {
        std::lock_guard lock(mMutex);
        IdList out;
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
    void eraseIndexesLocked(Id id) {
        for (auto it = mByRuntimeId.begin(); it != mByRuntimeId.end();) {
            if (it->second == id) {
                it = mByRuntimeId.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = mByDimension.begin(); it != mByDimension.end();) {
            auto& ids = it->second;
            ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
            it = ids.empty() ? mByDimension.erase(it) : std::next(it);
        }
    }

    mutable std::mutex mMutex;
    Id mNextId{1};
    bool mDirtyOnInsert{false};
    std::unordered_map<Id, std::unique_ptr<T>> mById;
    std::unordered_map<std::uint64_t, Id> mByRuntimeId;
    std::unordered_map<int, IdList> mByDimension;
    IdList mDirty;
};

} // namespace core
} // namespace debugshape_export
