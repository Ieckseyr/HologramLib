# 1.7 GhostInteractEvent · 交互事件

完整签名（`#include "HologramLib/HologramLib.h"`）：

客户端点击/攻击"协议上存在"的实体时，事件经 InventoryTransactionPacket 内嵌的 ItemUseOnActor 事务到达（协议 944 起 InteractPacket 不再承载实体交互）；库 hook 收包后将目标 runtimeId 反查回库内 id 并派发，实现**可点击 NPC / 全息菜单**。

```cpp
struct GhostInteractEvent {
    std::string playerName;   // 点击者（realName）
    int         action{0};    // 1=右键交互 2=左键攻击（见下方说明）
    std::string domain;       // "entity" / "itemDisplay" / "npc"
    int64_t     id{-1};       // 对应域的库内 id
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};
```

- `action`：1=右键交互 2=左键攻击（ItemUseOnActor 的 Interact/ItemInteract 映射为 1，Attack 映射为 2；与旧 InteractPacket 语义对齐，消费端无需区分来源）
- 消费方式二选一（可并存）：C++ 推送 `setGhostInteractListener`；LSE 轮询 `ghostPollInteractions`（取走并清空队列，队列上限 256 条、满时丢最旧）
- 恒调 origin，不改变 BDS 对未知 runtimeId 交互包的原版行为；无监听且无人轮询时仅做 runtimeId 段判别，近零开销

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
