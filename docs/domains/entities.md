# 实体与交互

自定义实体（纯协议层）与真实实体交互事件；实体本身没有 AI，交互由 ghost 域转发。


## 自定义实体

协议层生成一只原版实体，支持姿态、装备槽、动画、ActorLink 骑乘。

### 逐客户端朝向（`1.20.0`）

`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations`（实体与 NPC 通用）让**不同玩家看到不同的朝向** —— 即「千人千面」的朝向版。

`setRotationLight` 是一条**轻脏通道**：只发朝向、不重建实体，同一 tick 内多次调用会合并成一条包。

### 千人千面：逐观看者外观覆盖（`1.21.0`）

| 方法 | 作用 |
|---|---|
| `setPlayerNametag` | 该观看者看到的名字牌 |
| `setPlayerScale` | 该观看者看到的缩放 |
| `setPlayerEquipmentSlot` | 该观看者看到的装备 |
| `clearPlayerAppearance` | 清掉该观看者的全部覆盖 |

出生包与增量包都按观看者覆盖值下发，**装备变更即时单发、无闪烁**。
LSE 侧是 `entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`（`entity*` 尾部 4 个）。

### 属性同步

`setMobProperty`（`26.40.5` 起对外开放）用于下发 `client_sync` 类属性 —— 硫磺立方体的外观档位就走这条路。

> **为什么属性必须「出生之后」再发**：`AddActor` 的 `PropertySyncData` 只装得下按索引的 int / float，**装不下 enum**。所以 enum 属性（如 `sulfur_cube_archetype`）必须在客户端已经认识这只实体之后才能发。库里会在 spawn 之后自动补发、并在 respawn 后重放 —— 这属于「送达保障」，不是替消费方做业务。

---

## Ghost 交互

Ghost = **非真实实体**的交互路由。玩家的 `InteractPacket` 打在一个不是真实体的目标上（比如自定义实体、假玩家 NPC），库把它路由成事件给消费方。

```cpp
auto token = lib.addGhostInteractListener([](hologramlib::GhostInteractEvent const& e) {
    // e.action 是 InteractPacket 的原始 Action 值
});
lib.removeGhostInteractListener(token);
```

- C++ 侧支持**多播监听**（`addGhostInteractListener` / `removeGhostInteractListener` 返回 token，`1.19.1` 起）；旧的单监听 `setGhostInteractListener` 仍在。
- `GhostInteractEvent::action` 是 **`InteractPacket` 的原始 Action 值**：

  | 值 | 含义 |
  |---|---|
  | 1 | Interact（右键） |
  | 2 | Attack（左键攻击） |
  | 3 | StopRiding |
  | 4 | InteractUpdate |
  | 5 | NpcOpen |
  | 6 | OpenInventory |

  处理前先判 `action == 1 || action == 2`，不要假设只有这两种取值。

> **脚本侧拿事件**：`ghostPollInteractions()` 取走并清空待处理队列（返回字符串数组，格式见 [实体、粒子与交互](../lse/entities.md)），`ghostClearInteractions()` 直接丢弃队列。C++ 侧另有 `addGhostInteractListener` 多播监听器。

---
## 新 API / 交互（1.31.0）

- 真实实体交互事件：`addActorInteractListener` / `removeActorInteractListener`（C++，类尾追加）。
- `ICustomEntity::setMobProperty` 可同步 `client_sync` 属性。
- 实体动画：`playAnimation` / `playAnimationTo` / `stopAnimation` / `stopAnimationTo`；`stopExpression` 是唯一的自由 Molang 字段。
- 假玩家交互见 [假玩家 NPC](player-npc.md)。
