# 1.11 INpcDialogue · NPC 对话

下发 `NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON），并合成一只**只为该玩家可见**的 NPC 载体来承载界面；玩家点按钮或关闭时由 `NpcRequestPacket` 回传。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().npcDialogs()`
- 载体：纯协议合成的 `minecraft:npc`，摆在 `y = -66`（世界下方），不进 BDS 实体系统；客户端看不到实体，但界面里的头像照常渲染
- 按钮表：完全由服务端逐玩家生成 —— 按权限 / tag / 计分板 / 任务状态决定按钮，是在调用前自己算好 `buttons`
- 点击回传：`dialogId` + `sceneName` + `buttonIndex` + `actionId`（+ 按钮上挂的 `commands`）
- 生命周期：每个玩家同一时刻只有一个对话；点按钮 / 关闭 / 离线时载体一起删
- 版本：对话域 1.21.0 起；头部自定义字段（`avatarSkinVariant` / `avatarViewSpec`）1.26.0（26.40.8）加入

!!! info "点过任意按钮后，客户端会自己收起界面"

    多层级对话的做法是：点击回传里带 `sceneName` + `buttonIndex` / `actionId`，调用方据此**重新 `open`** 下一层。
    `update` 只用于「界面还开着」时的内容替换，唤不回已经收起的界面。

!!! note "按钮上的命令不会自己执行"

    `NpcDialogButton.commands` 只是显示在按钮上、并在点击回传里原样带回。
    合成 NPC 在服务端没有实体代跑命令，要执行请消费方自己来（如 `player.runCommand(...)`）。

!!! tip "头像可以自定义（26.40.8）"

    `avatarSkinVariant` 换 NPC 内置皮肤变体（`0 ~ 59`），`avatarViewSpec` 在载体上叠一层视图覆盖
    （`type=` 换载体类型、`name=` 换界面标题）。具体边界见「数据结构」里的字段说明。

## 数据结构

```cpp
struct NpcDialogButton {
    std::string              label;             // 按钮文字（支持 §）
    std::vector<std::string> commands;          // 展示在按钮上；点击时原样回传，不由 BDS 执行
    std::string              actionId;          // 服务器侧标识，点击时原样带回
    int                      mode{0};           // 0=普通按钮 1=关闭 2=打开
};

struct NpcDialogSpec {
    std::string carrierIdentifier{"minecraft:npc"}; // 载体实体类型
    std::string npcName{"NPC"};                     // 界面里的 NPC 名
    std::string sceneName{"main"};                  // 场景名
    std::string dialogue;                           // 正文

    std::int64_t                 npcUniqueIdOverride{0};
    std::vector<NpcDialogButton> buttons;
    std::string                  rawActionJson;

    int         avatarSkinVariant{-1};
    std::string avatarViewSpec;
};

struct NpcDialogClickEvent {
    std::string playerName;
    int64_t     dialogId{-1};
    std::string sceneName;
    int         buttonIndex{-1};  // -1 = 非按钮事件
    std::string actionId;
    bool        closed{false};
    std::vector<std::string> commands; // 被点击按钮上挂的命令（按 buttonIndex 回填）
};
```

- `NpcDialogButton.label` : `std::string`<br>按钮文字，支持 `§` 颜色码
- `NpcDialogButton.commands` : `std::vector<std::string>`<br>挂在按钮上的命令文本；会显示在按钮上，并随点击事件回传
- `NpcDialogButton.actionId` : `std::string`<br>服务器侧标识（对接对话树 / 任务），点击时原样带回
- `NpcDialogButton.mode` : `int`<br>`0` = 普通按钮，`1` = 关闭，`2` = 打开
- `NpcDialogSpec.carrierIdentifier` : `std::string`<br>载体实体类型；必须是 NPC 家族（客户端界面与该类型绑定，换成村民等原版实体弹不出来）
- `NpcDialogSpec.npcName` : `std::string`<br>界面里显示的 NPC 名，支持 `§`；空串 = 默认名
- `NpcDialogSpec.sceneName` : `std::string`<br>场景名；多层级对话靠它路由，点击回传会带回来
- `NpcDialogSpec.dialogue` : `std::string`<br>正文
- `NpcDialogSpec.npcUniqueIdOverride` : `std::int64_t`<br>非 `0` = 用这只**已存在实体**的 uniqueId 当界面绑定的 `npc_id`（把对话挂到真实 NPC 上）；`0` = 用库合成的载体
- `NpcDialogSpec.buttons` : `std::vector<NpcDialogButton>`<br>按钮表；空表 = 只有正文和关闭
- `NpcDialogSpec.rawActionJson` : `std::string`<br>非空时**原样**作为 `mActionJSON` 下发（覆盖 `buttons`），用于在游戏内实测原版按钮字段格式
- `NpcDialogSpec.avatarSkinVariant` : `int`<br>NPC 内置皮肤变体 `0 ~ 59`（写进载体的 `SkinId(104)`）；`-1`（默认）= 不加这一项，行为与旧版逐字节一致
- `NpcDialogSpec.avatarViewSpec` : `std::string`<br>在载体上叠加视图覆盖，语法同 `viewEntity`：`type=` 换载体类型（实测僵尸 / 鸡这类非 NPC 家族照常弹界面）、`name=` 换界面标题；空串 = 不叠

!!! note "`avatarViewSpec` 的 `skin=` 分支已移除"

    它会把载体换成「玩家模型」（`PlayerList` + `AddPlayer`），实测客户端在该载体上点进菜单会关界面，当前不可用。
    「让客户端认它是玩家」的路线由消费方自建（`playerNpcs()` + `npcUniqueIdOverride`）。

## 打开与关闭

### 打开一个对话

`int64_t open(std::string const& playerName, NpcDialogSpec const& spec)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名，必须在线
  - `spec` : `NpcDialogSpec const&`<br>场景 / 正文 / NPC 名 / 按钮表 / 头部外观，字段见「数据结构」
- 返回值：对话 id（从 `1` 自增）；失败返回 `-1`（玩家不在线、载体生成失败）。同一玩家重复调用会先拆掉上一层（含载体）再开新的
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& lib = hologramlib::IHologramLib::getInstance();

    hologramlib::NpcDialogSpec spec;
    spec.npcName   = "§e村长";
    spec.sceneName = "main";
    spec.dialogue  = "要来点任务吗？";

    spec.buttons.push_back({"§a接受", {"say 接受"}, "accept", 0});
    spec.buttons.push_back({"§c离开", {}, "leave", 1});

    int64_t dialogId = lib.npcDialogs().open("Steve", spec);
    if (dialogId < 0) {
        return; // 玩家不在线
    }
    ```

### 就地换内容

`bool update(int64_t dialogId, NpcDialogSpec const& spec)`

- 参数：
  - `dialogId` : `int64_t`<br>对话 id
  - `spec` : `NpcDialogSpec const&`<br>**整套新配置**（整个替换，不是增量合并）
- 返回值：已重发 `true`；对话不存在或玩家离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    spec.dialogue = "任务进度：3/10";
    lib.npcDialogs().update(dialogId, spec);
    ```

!!! note "`update` 不重建载体，也唤不回已收起的界面"

    它沿用同一个载体，只把 `NpcDialogue(Open)` 再发一遍 —— 适合界面**还开着**时改正文 / 按钮。
    玩家点过任意按钮后客户端会自己收起界面，此时要再显示必须重新 `open`（会生成新载体）。

### 关闭一个对话

`bool close(int64_t dialogId)`

- 参数：
  - `dialogId` : `int64_t`<br>对话 id
- 返回值：成功 `true`（发 `NpcDialogue(Close)` + `RemoveActor` 删掉载体）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.npcDialogs().close(dialogId);
    ```

!!! tip "点完按钮不用手动 close"

    任意按钮点击（以及玩家关闭界面）之后，只要回调里没有 `open` / `update` 接手，库会自动收尾：
    删除对话记录并移除载体。手动 `close` 主要用于「还没点就撤掉」的场景。

### 关闭全部对话

`void closeAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.npcDialogs().closeAll();
    ```

### 查询对话是否还开着

`bool isOpen(int64_t dialogId) const`

- 参数：
  - `dialogId` : `int64_t`<br>对话 id
- 返回值：存在 `true`；已关闭或从来不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (lib.npcDialogs().isOpen(dialogId)) {
        lib.npcDialogs().close(dialogId);
    }
    ```

### 列出全部对话 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有打开的对话 id；没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t id : lib.npcDialogs().getAllIds()) {
        lib.npcDialogs().close(id);
    }
    ```

## 点击回传

### 注册点击监听器

`uint64_t addClickListener(std::function<void(NpcDialogClickEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(NpcDialogClickEvent const&)>`<br>按钮点击 / 关闭时回调；可注册多个，按注册顺序调用
- 返回值：监听 token（从 `1` 开始）；传空函数返回 `0`
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    uint64_t token = lib.npcDialogs().addClickListener(
        [&lib](hologramlib::NpcDialogClickEvent const& e) {
            if (e.closed || e.buttonIndex < 0) {
                return; // 玩家直接关了界面
            }
            if (e.actionId == "accept") {
                hologramlib::NpcDialogSpec next;
                next.npcName   = "§e村长";
                next.sceneName = "quest";
                next.dialogue  = "去帮我打 10 只僵尸。";
                next.buttons.push_back({"§a知道了", {}, "ok", 1});
                lib.npcDialogs().open(e.playerName, next); // 换页 = 重新 open
            }
        });
    ```

!!! note "事件的字段怎么来"

    - 点了按钮：`buttonIndex` = 按钮下标（`0` 起），`actionId` / `commands` 从该按钮回填，`closed = false`
    - 玩家关闭界面：`closed = true`，`buttonIndex = -1`
    - `sceneName` 优先用客户端回传值，回传为空时用 `spec.sceneName`
    - 不是按钮点击、也不是关闭的请求不会派发事件

### 移除点击监听器

`bool removeClickListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addClickListener` 返回的 token
- 返回值：移除成功 `true`；token 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.npcDialogs().removeClickListener(token);
    ```

!!! note "C++ 侧只有监听器，没有轮询"

    与 ghost / trade 不同，`INpcDialogue` 不提供 `poll*` 接口 —— 点击必须用监听器接。
    （脚本侧走 `npcDialogPollClicks` 轮询，那是 LSE 导出，不是 C++ 接口。）

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

// 两层对话：第一层「接受 / 离开」，接受后换到第二层
void open_quest_dialog(std::string const& playerName, uint64_t& outToken) {
    auto& lib = hologramlib::IHologramLib::getInstance();

    hologramlib::NpcDialogSpec first;
    first.npcName   = "§e村长";
    first.sceneName = "main";
    first.dialogue  = "要来点任务吗？";
    first.buttons.push_back({"§a接受", {"say accept"}, "accept", 0});
    first.buttons.push_back({"§c离开", {}, "leave", 1});

    if (lib.npcDialogs().open(playerName, first) < 0) {
        return; // 玩家不在线
    }

    outToken = lib.npcDialogs().addClickListener(
        [&lib](hologramlib::NpcDialogClickEvent const& e) {
            if (e.closed) {
                return;
            }
            if (e.actionId == "accept") {
                hologramlib::NpcDialogSpec second;
                second.npcName   = "§e村长";
                second.sceneName = "quest";
                second.dialogue  = "任务进度：0/10";
                second.buttons.push_back({"§a知道了", {}, "ok", 1});
                lib.npcDialogs().open(e.playerName, second); // 点完按钮必须重新 open
            }
        });
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
