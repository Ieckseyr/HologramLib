# 1.7 GhostInteractEvent · 交互事件

玩家点/打一个「协议上存在、服务端没有真实实体」的库内目标（自定义实体、物品悬浮展示、假玩家 NPC）时派发的事件，用来做可点击 NPC、全息按钮和菜单入口。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：事件挂在总入口上，不属于某个域 —— `hologramlib::IHologramLib::getInstance()`
- 触发目标：`entity`（`ICustomEntity`）/ `itemDisplay`（`IItemDisplay`）/ `npc`（`IPlayerNpc`）
- `action`：`1` 右键交互 / `2` 左键攻击
- 坐标：`hasPos` 为 `true` 时，`x` / `y` / `z` 是点击**命中点**的世界坐标
- 线程：在收包路径上**同步**回调，不是下一 tick 的批量回调

!!! info "1.12.0 起，1.19.1 加多播"

    `setGhostInteractListener` / `clearGhostInteractListener` / `pollGhostInteractions` 是 1.12.0 的单播与轮询入口；
    `addGhostInteractListener` / `removeGhostInteractListener`（返回 token）是 1.19.1 追加的多播入口。
    两套可以同时用：先回调所有多播监听器，再回调单播监听器。

## 何时触发

客户端点击或攻击一个目标时，BDS 收包 → 库先执行 `origin`（原版处理照旧）→ 用目标 runtimeId 反查库内三张表 → 命中就构造事件派发。
主路径是 `InventoryTransactionPacket` 内嵌的 `ItemUseOnActor` 事务（协议 944 起实体交互不再走 `InteractPacket`）；
旧的 `InteractPacket` 钩子为兼容保留，两条路径都能派发。

!!! note "只在目标属于库内三张表时派发 Ghost 事件"

    判定的三张表是：自定义实体（`ICustomEntity`）、物品悬浮展示（`IItemDisplay`）、假玩家 NPC（`IPlayerNpc`）。
    目标 runtimeId 不在表里，**不会**产生 `GhostInteractEvent`；1.27.0 起这类点击改走真实实体通道（见下节 `ActorInteractEvent`），
    真机里两者互补，不是二选一。

!!! tip "先判 `action == 1 || action == 2`"

    主路径（`ItemUseOnActor` 事务）把 `Interact` / `ItemInteract` 映射成 `1`、`Attack` 映射成 `2`；
    兼容保留的旧 `InteractPacket` 路径会原样带出 `3..6`（下马 / 交互更新 / 打开 NPC / 打开背包）。
    不要假设只有 `1` 和 `2` 两种取值。

## 事件结构

```cpp
struct GhostInteractEvent {
    std::string playerName;   // 点击者（realName）
    int         action{0};    // 1=右键交互 2=左键攻击
    std::string domain;       // "entity" / "itemDisplay" / "npc"
    int64_t     id{-1};       // 对应域的库内 id（create 返回值，不是 runtimeId）
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};
```

- `playerName` : `std::string`<br>点击者玩家名（`realName`，不是显示名、不是 UUID）
- `action` : `int`<br>`1` = 右键交互，`2` = 左键攻击；旧 `InteractPacket` 路径可能是 `3 ~ 6`
- `domain` : `std::string`<br>`"entity"` 自定义实体 / `"itemDisplay"` 物品悬浮展示 / `"npc"` 假玩家 NPC
- `id` : `int64_t`<br>该域里的库内 id（`create` 返回的那个）；`domain` 与 `id` 一定成对出现，不会只填一个
- `hasPos` : `bool`<br>`true` = 带命中点坐标；旧 `InteractPacket` 路径没带坐标时为 `false`
- `x` / `y` / `z` : `float`<br>命中点世界坐标（方块单位）；`hasPos == false` 时恒为 `0`

## 订阅

### 注册多播监听器

`uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(GhostInteractEvent const&)>`<br>回调；可以注册多个，全部按注册顺序调用
- 返回值：监听 token（从 `1` 开始递增），移除时用它；传空函数返回 `0`（注册失败）
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    auto& lib = hologramlib::IHologramLib::getInstance();

    uint64_t token = lib.addGhostInteractListener([](hologramlib::GhostInteractEvent const& e) {
        if (e.action != 1) {
            return; // 只关心右键
        }
        // domain + id 就是库里的那只目标
    });
    ```

### 移除多播监听器

`bool removeGhostInteractListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addGhostInteractListener` 返回的监听 token
- 返回值：移除成功 `true`；token 不存在（已移除或从未注册）`false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.removeGhostInteractListener(token);
    token = 0;
    ```

### 设置单播监听器

`void setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(GhostInteractEvent const&)>`<br>回调；再次调用会**替换**上一个单播监听器（不是追加）
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.setGhostInteractListener([](hologramlib::GhostInteractEvent const& e) {
        // ...
    });
    ```

!!! info "单播与多播的差别"

    单播是 1.12.0 的旧入口，只有一个槽位、后设覆盖前设；多播是 1.19.1 的新入口，可以有任意多个，
    并且能用 token 精确移除。两者同时存在时**先多播后单播**，同一事件会被两边各回调一次。

### 清空单播监听器

`void clearGhostInteractListener()`

- 参数：无
- 返回值：无（只清单播槽位，不影响多播监听器）
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.clearGhostInteractListener();
    ```

## 轮询

### 轮询交互队列

`std::vector<std::string> pollGhostInteractions()`

- 参数：无
- 返回值：一行一个事件，格式 `player=<名> action=<n> domain=<域> id=<id>[ pos=(x,y,z)]`；队列空时是空数组
- 返回值类型：`std::vector<std::string>`
- 示例：

    ```cpp
    // 例如 "player=Steve action=1 domain=npc id=7"
    for (auto const& line : lib.pollGhostInteractions()) {
        // 按 key= 切分解析：消费端不要假设字段之间只有空格
    }
    ```

!!! note "取走并清空，队列上限 256"

    队列是「取走并清空」语义。有没有监听器，事件都会入队 —— 只挂监听器、从不轮询时队列会一直滚动，
    满 256 条后丢最旧的一条（不会无限增长，但也意味着那份轮询数据是残缺的）。
    只用监听器的话，可以偶尔调一次 `pollGhostInteractions()` 顺手倒掉。
    `pos=` 只在 `hasPos` 为 `true` 时出现，其余字段顺序固定。

## 线程与时序

- **同步派发**：回调在收包钩子里执行，处理完监听器才返回；不会推迟到下一 tick，也不会合并。
- **先放行原版**：两个钩子都先调 `origin`，库只旁听、不改写包，所以对未知 runtimeId 的交互包原版行为不受影响。
- **锁外回调**：监听器被调用时库内互斥已释放，回调里可以安全地 `add/removeGhostInteractListener`、`pollGhostInteractions`，
  也可以直接销毁触发事件的那只目标。
- **顺序**：同一玩家的连续点击按收包顺序回调；不同玩家之间不保证顺序。
- **线程**：回调运行在收包线程上，不保证是 tick 线程。需要按 tick 推进的逻辑，把事件记进自己的队列、到 tick 里消费。
- **异常**：Ghost 通道不吞异常（真实实体通道会捕获并记日志），回调里请自己 `try` / `catch`，否则会带着异常退出钩子。

## 真实实体交互（1.27.0）

同一条收包路径把「不是库内目标」的点击也转出来，字段换成 runtimeId / uniqueId 形式；这条通道与 Ghost 通道互补。

### 注册真实实体监听器

`uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(ActorInteractEvent const&)>`<br>回调；可注册多个
- 返回值：监听 token（从 `1` 开始，与 Ghost token 共用同一个计数器）；传空函数返回 `0`
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    uint64_t actorToken = lib.addActorInteractListener([](hologramlib::ActorInteractEvent const& e) {
        if (e.action == 2) {
            // 被攻击的真实实体：e.targetType / e.uniqueId
        }
    });
    ```

### 移除真实实体监听器

`bool removeActorInteractListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addActorInteractListener` 返回的 token
- 返回值：移除成功 `true`；token 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.removeActorInteractListener(actorToken);
    ```

`ActorInteractEvent` 的字段：

| 字段 | 类型 | 含义 |
|---|---|---|
| `playerName` | `std::string` | 点击者玩家名 |
| `targetType` | `std::string` | 目标实体类型名，如 `minecraft:zombie`；实体已消失时为空串 |
| `action` | `int` | `1` 右键 / `2` 左键（与 Ghost 通道同义） |
| `runtimeId` | `std::uint64_t` | 目标运行时 id |
| `uniqueId` | `std::int64_t` | 目标 uniqueId；取不到时为 `0` |
| `hasPos` / `x` / `y` / `z` | `bool` / `float` | 同 Ghost 事件 |

!!! note "这条通道没有轮询接口"

    只有监听器能拿到事件，记得自己存 token；Ghost 那边才提供 `pollGhostInteractions`。

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

void install_click_router() {
    auto& lib = hologramlib::IHologramLib::getInstance();

    // 一个菜单入口：右键任何假玩家 NPC 就打开它的对话框
    uint64_t token = lib.addGhostInteractListener([&lib](hologramlib::GhostInteractEvent const& e) {
        if (e.action != 1 || e.domain != "npc") {
            return;
        }

        hologramlib::NpcDialogSpec spec;
        spec.npcName = "§e店员";
        spec.dialogue = "要点什么？";
        spec.buttons.push_back({"§a看看货", {"say hi"}, "shop", 0});
        lib.npcDialogs().open(e.playerName, spec);
    });

    // 关服/卸载时收尾
    // lib.removeGhostInteractListener(token);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
