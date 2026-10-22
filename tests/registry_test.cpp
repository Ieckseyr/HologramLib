// registry_test.cpp - core/Registry<T> 的离线单测（不依赖 BDS / LeviLamina）
//
// 为什么独立: Registry 只依赖标准库, 所以它可以在 CI 里用 g++ 单独编译运行,
// 不需要 levilamina 包、不需要起服。内核层能这样测的只有它和 ABI 工具。
//
// 跑法:  g++ -std=c++20 -I src tests/registry_test.cpp -o registry_test && ./registry_test
//        (Windows 本地: clang-cl /std:c++latest /EHsc /I src tests/registry_test.cpp)
#include "core/Registry.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace core = ::debugshape_export::core;

int gFailures = 0;

void check(bool cond, char const* what) {
    if (cond) {
        std::printf("  PASS  %s\n", what);
    } else {
        std::printf("  FAIL  %s\n", what);
        ++gFailures;
    }
}

struct Item {
    int         dim{0};
    std::string name;
};

void testIdAllocation() {
    std::printf("[1] id 分配: 递增 / 不复用\n");
    core::Registry<Item> reg;
    auto const           a = reg.insert(std::make_unique<Item>(Item{0, "a"}));
    auto const           b = reg.insert(std::make_unique<Item>(Item{0, "b"}));
    check(b == a + 1, "连续 insert 的 id 递增 1");
    check(reg.erase(a), "erase 存在的 id 返回 true");
    auto const c = reg.insert(std::make_unique<Item>(Item{0, "c"}));
    check(c > b, "erase 之后新 id 不回填（迟到的包不会打到新对象上）");
    check(!reg.erase(a), "重复 erase 返回 false");
    check(reg.size() == 2, "erase 后 size 正确");
}

void testRuntimeIdIndex() {
    std::printf("[2] runtimeId 索引与主表同生共死\n");
    core::Registry<Item> reg;
    auto const           id    = reg.insert(std::make_unique<Item>(Item{0, "x"}), 0x6D000001ULL);
    auto const           found = reg.findByRuntimeId(0x6D000001ULL);
    check(found.has_value() && *found == id, "按 runtimeId 能查到 id");
    check(!reg.findByRuntimeId(0x6D000002ULL).has_value(), "未登记的 runtimeId 返回 nullopt");
    reg.erase(id);
    check(!reg.findByRuntimeId(0x6D000001ULL).has_value(), "erase 之后 runtimeId 索引一并清除（手写最常漏的一条）");
}

void testDimensionIndex() {
    std::printf("[3] 维度索引: 分桶 / 迁移 / 删除\n");
    core::Registry<Item> reg;
    auto const           a = reg.insert(std::make_unique<Item>(Item{1, "a"}), 0, 1);
    auto const           b = reg.insert(std::make_unique<Item>(Item{2, "b"}), 0, 2);
    check(reg.snapshotByDimension(1).size() == 1, "维度 1 只有 1 个");
    check(reg.snapshotByDimension(1)[0] == a, "维度 1 里是 a");

    reg.setDimension(a, 2);
    check(reg.snapshotByDimension(1).empty(), "迁移后原维度清空");
    check(reg.snapshotByDimension(2).size() == 2, "迁移后新维度有 2 个");

    reg.erase(b);
    auto const inDim2 = reg.snapshotByDimension(2);
    check(inDim2.size() == 1 && inDim2[0] == a, "erase 后新维度索引也清除（不留悬空 id）");
}

void testDirtySet() {
    std::printf("[4] 脏集: 插入策略 / 取走语义 / 过滤已销毁\n");
    core::Registry<Item> plain; // 默认: 插入不标脏（只在增删时动作的域不该让脏集只增不取）
    plain.insert(std::make_unique<Item>(Item{0, "a"}));
    check(plain.dirtyCount() == 0, "默认配置下 insert 不标脏");

    core::Registry<Item> ticking{true}; // 逐 tick 刷新的渲染域
    auto const           a = ticking.insert(std::make_unique<Item>(Item{0, "a"}));
    auto const           b = ticking.insert(std::make_unique<Item>(Item{0, "b"}));
    check(ticking.dirtyCount() == 2, "dirtyOnInsert 配置下 insert 自动标脏");

    auto first = ticking.takeDirty();
    check(first.size() == 2, "takeDirty 取到 2 个");
    check(ticking.dirtyCount() == 0, "takeDirty 之后脏集清空（不是读后清）");

    ticking.markDirty(a);
    ticking.erase(a);
    auto second = ticking.takeDirty();
    check(second.empty(), "已销毁的 id 从脏集里被过滤掉");

    ticking.markAllDirty();
    check(ticking.takeDirty().size() == 1, "markAllDirty 只标记活着的对象");
    (void)b;
}

void testSnapshotAndForEach() {
    std::printf("[5] 快照遍历: 锁外使用\n");
    core::Registry<Item> reg;
    for (int i = 0; i < 5; ++i) {
        reg.insert(std::make_unique<Item>(Item{i, "item"}));
    }
    auto ids = reg.snapshotAll();
    check(ids.size() == 5, "snapshotAll 返回全部 id");

    int sum = 0;
    for (auto const id : ids) {
        reg.withObject(id, [&](Item const& item) {
            sum += item.dim;
        });
    }
    check(sum == 0 + 1 + 2 + 3 + 4, "按快照逐个 withObject 汇总正确");

    reg.erase(ids[0]);
    int visited = 0;
    reg.forEachLocked([&](core::Registry<Item>::Id, Item const&) {
        ++visited;
    });
    check(visited == 4, "forEachLocked 只遍历活着的对象");
}

void testConcurrentAccess() {
    std::printf("[6] 并发: 多线程 insert/erase 不崩且计数自洽\n");
    core::Registry<Item> reg;
    constexpr int        kThreads   = 4;
    constexpr int        kPerThread = 500;

    std::vector<std::thread> threads;
    std::atomic<int>         inserted{0};
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&reg, &inserted] {
            for (int i = 0; i < kPerThread; ++i) {
                auto const id = reg.insert(std::make_unique<Item>(Item{i % 3, "t"}), 0, (i % 3) + 1);
                inserted.fetch_add(1);
                if (i % 4 == 0)
                    reg.erase(id);
            }
        });
    }
    for (auto& th : threads)
        th.join();

    auto const total  = reg.size();
    auto const dimSum = reg.snapshotByDimension(1).size() + reg.snapshotByDimension(2).size() +
                        reg.snapshotByDimension(3).size();
    check(total == static_cast<std::size_t>(inserted.load() - (kThreads * (kPerThread / 4))),
          "size 等于 插入数-删除数");
    check(total == dimSum, "维度索引总数与主表一致（并发下没有漏索引）");
}

} // namespace

int main() {
    testIdAllocation();
    testRuntimeIdIndex();
    testDimensionIndex();
    testDirtySet();
    testSnapshotAndForEach();
    testConcurrentAccess();

    if (gFailures != 0) {
        std::printf("\nREGISTRY-TEST: %d 项失败\n", gFailures);
        return 1;
    }
    std::printf("\nREGISTRY-TEST: 全部通过\n");
    return 0;
}
