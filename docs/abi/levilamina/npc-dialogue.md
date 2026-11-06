# 1.11 INpcDialogue · NPC 对话

完整签名（`#include "HologramLib/HologramLib.h"`）：

`IHologramLib::npcDialogs()`。`NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON）下发，`NpcRequestPacket` 回传（点了第几个按钮 / 玩家关闭）。

- 按钮表完全由服务端逐玩家生成：按权限 / tag / 计分板 / 任务状态决定按钮是否出现，就是在调用本接口前自行算好 `buttons`，不需要客户端配合。
- 多层级对话 = 点击回传带回 `sceneName` + `buttonIndex` / `actionId`，调用方据此发送下一层。
- 载体实体是**纯协议合成**的 `minecraft:npc`（不进 BDS 实体系统），位置在世界下方 `y=-66`——客户端看不到实体，但界面里的头像照常渲染（改用隐形标志位反而会让头像一起消失）。只发给该玩家；点按钮 / 关闭 / 离线时发 `RemoveActor` 删掉。
- `NpcDialogSpec::rawActionJson` 非空时原样作为 `mActionJSON` 下发（原版按钮结构无公开文档，用于在游戏内实测字段格式）；`carrierIdentifier` 必须是 NPC 家族，换成村民等原版实体不行（实测要求）。
- **聊天框内显示的头像/模型可自定义（26.40.8）**：`NpcDialogSpec::avatarSkinVariant`（NPC **内置皮肤变体** 0..59，即 `NpcData.skin_list` 的 variant 值）—— 写进载体 ActorData 的 `SkinId(104)`，界面里的头像/模型换内置皮肤；`-1`（默认）不加该项（旧行为逐字节一致）。`NpcDialogSpec::avatarViewSpec` —— **在载体上叠一层 IViewOverride 语义**（spec 语法同 `viewEntity`）：`type=` 换载体类型（**实测: 僵尸/鸡这类非 NPC 家族照常弹界面**）、`name=` 换界面标题/交互文字。**`skin=` 分支已移除**（未发布批次内的收缩, 明记）: 它会把载体换成"玩家模型"（PlayerList+AddPlayer）, 实测客户端在该载体上点进菜单会关界面, 当前不可用；"让客户端认它是玩家"的路线由消费方自建（`playerNpcs()` + `npcUniqueIdOverride`）。要"整只模型完全自定义"也可用 `npcUniqueIdOverride` 路线：视图域 / 自定义实体生成展示实体、对话挂在它 id 上（1.27.0 的 `EntityView::skinId` 同-id 换模型也可用于那只展示实体）。LSE 侧就地修改用 `npcDialogSetAvatar`。

```cpp
struct NpcDialogButton {
    std::string label;
    std::vector<std::string> commands; // 展示在按钮上; 合成 NPC 不由 BDS 代跑, 命令回传给调用方
    std::string actionId;              // 服务器侧标识, 点击回传原样带回（对接对话树/任务）
    int         mode{0};               // 0=普通按钮 1=关闭 2=打开
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

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
