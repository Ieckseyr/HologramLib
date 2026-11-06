# 1.17 IItemActor / IFormControl / ILocatorBar

完整签名（`#include "HologramLib/HologramLib.h"`）：

四个接口都很小，各自的说明见 [掉落物](../../domains/item-actor.md) / [表单控制](../../domains/form.md) / [方位条](../../domains/locator-bar.md) / [隐身](../../domains/visibility.md)，这里只列签名。

**IItemActor** —— 协议层掉落物（`AddItemActor` / `SetActorMotion` / `TakeItemActor` / `RemoveActor`）：

```cpp
struct ItemActorSpec {
    std::string itemId; int aux{0}; int count{1};
    float x{0}, y{0}, z{0}; int dimension{0};
    float vx{0}, vy{0}, vz{0};
};

IItemActor& itemActors();
int64_t spawn(ItemActorSpec const& spec);   // 只发给同维度; 该维度没人在线 = -1
bool    setMotion(int64_t id, float vx, float vy, float vz); // 飞行途中改速度/方向
bool    moveTo(int64_t id, float x, float y, float z);
bool    take(int64_t id, std::string const& playerName);     // 播拾取动画后删除, 不给物品
bool    destroy(int64_t id);
void    destroyAll();
bool    exists(int64_t id) const;
std::vector<int64_t> getAllIds() const;
```

物品走数字 id + count + aux，**不带 UserData**（原因见 [掉落物](../../domains/item-actor.md)）。

**IFormControl** —— 强制关掉客户端表单（`ClientboundCloseForm`）：

```cpp
IFormControl& formControl();
bool closeForm(std::string const& playerName);
int  closeAllForms();
```

**ILocatorBar** —— 方位条航点（`LocatorBar`）：

```cpp
ILocatorBar& locatorBar();
bool setWaypoint(playerName, key, x, y, z, dimension, color = -1, visible = true);
bool removeWaypoint(playerName, key);
int  clearWaypoints(playerName);
int  waypointCount(playerName) const;
bool hideFromOthers(playerName, hidden);   // 转给视图覆盖域的 hidden 通道
bool isHiddenFromOthers(playerName) const;
```

**IPlayerVisibility** —— 隐身（`AddPlayer` / `PlayerList` / `PlayerSkin` / `MovePlayer` /
`MoveActorAbsolute` / `SetActorMotion` / `SetActorData` / `Animate` / `ActorEvent` / `MobEffect`）：

```cpp
struct PlayerPacketMask {
    bool spawn{false}; bool tab{false}; bool motion{false}; bool state{false}; bool skin{false};
};

IPlayerVisibility& playerVisibility();
bool setMask(playerName, PlayerPacketMask const& mask, bool alsoSelf = false);
bool setHidden(playerName, hidden);   // 整体隐身（复用视图覆盖域 hidden 通道）
bool restore(playerName);             // 清掩码 + 清隐身 + 立刻补发出生包
PlayerPacketMask maskOf(playerName) const;
bool isHidden(playerName) const;
```

### 1.33.0 追加：IBossBar / IHud / ILocatorBarExt / IActorEffect

四个接口都追加在头文件尾部（`HologramLib.h` 里 `IHologramLib` 之前），访问器追加在 `IHologramLib` 尾部。

| 接口 | 访问器 | 方法 |
|------|--------|------|
| `IBossBar` | `bossBars()` | `add(playerName, title, healthPercent, color, overlays)` / `setTitle` / `setHealth` / `setStyle` / `remove` / `removeAll` / `count` |
| `IHud` | `hud()` | `setElement(playerName, HudElement, visible)` / `setElements(playerName, mask, visible)` / `resetAll(playerName)` |
| `ILocatorBarExt` | `locatorBarExt()` | `setIcon` / `setIconSize` / `attachEntity` / `clearAttach` / `setGroup` |
| `IActorEffect` | `actorEffects()` | `playHurt(runtimeId)` / `playDeath` / `playInstantDeath` / `play(runtimeId, eventId, data)` |
| `ISidebar` | `sidebar()` | `set(playerName, title, lines)` / `setTitle` / `setLine` / `remove` / `removeAll` / `isShown` / `lineCount` |

枚举：`BossBarColor`（0~7）、`BossBarOverlay`（0~4）、`HudElement`（0~12）。

协议：`BossEventPacket`(74) / `SetHudPacket`(308) / `LocatorBarPacket`(341) / `ActorEventPacket`(27) / `SetDisplayObjectivePacket`(107) + `SetScorePacket`(108) + `RemoveObjectivePacket`(106)。
`IBossBar` 的 target 必须是客户端认识的真实实体，所以建条时库自动在玩家所在维度的虚空里（`y = -120`）生成一只 `minecraft:structure_void` 无模型掉落物作锚点，删除时回收。

`HOLOGLIB_ABI_STAMP` 仍为 1（尾部追加），但新增了接口，消费方需要重新编译。基线 `abi/v1.33.0.json`。

**既有接口的尾部追加**：`IContainerMenu::blockEvent(menuId, b0, b1)`（`BlockEvent`，假箱子的方块事件）、
`IPlayerNpc::playEmote(id, pieceId, lengthTicks)`（`Emote`，原版表情动作）。

这一版虚表尾部变长（3 个新接口 + 两个方法），`HOLOGLIB_ABI_STAMP` 仍为 1，
但用旧头文件编译的消费方要重新编译；基线是 `abi/v1.32.0.json`。

### 1.34.0 追加：IPlayerRespawn（死亡动画后的复活，只发包）

`IPlayerRespawn` 追加在头文件尾部（`IHologramLib` 之前），访问器 `playerRespawn()` 追加在
`IHologramLib` 尾部；`HOLOGLIB_ABI_STAMP` 仍为 `1`，但用旧头文件编译的消费方需要重新编译。
基线 `abi/v1.34.0.json`。

```cpp
class IPlayerRespawn {
public:
    virtual ~IPlayerRespawn() = default;
    // 原地复活（玩家当前坐标）; state 默认 2 = ClientReadyToSpawn
    virtual bool respawn(std::string const& playerName, int state = 2) = 0;
    // 指定坐标复活
    virtual bool respawnAt(std::string const& playerName, float x, float y, float z, int state = 2) = 0;
};
```

**为什么需要它**：`IActorEffect::playDeath()` 只是给客户端发 `ActorEvent::DEATH`，
服务端实体并未死亡；客户端进死亡界面后发 `PlayerAction(Respawn)`，服务端判定"无需复活"不会回包，
客户端于是卡住。`IPlayerRespawn` 直接补一个 `RespawnPacket`(45) 给客户端：

```cpp
auto& api = hologramlib::IHologramLib::getInstance();
api.actorEffects().playDeath(runtimeId);      // 播死亡动画
// ……稍后
api.playerRespawn().respawn("Steve");        // 把客户端从死亡界面拉回来
```

协议：`RespawnPacket`(45) = `Position`(Vec3) + `State`(uint8: 0/1/2) + `PlayerRuntimeId`(uvarint64)。
服务端实体的血量 / 位置 / 维度 / 背包都不变，也不会触发死亡或重生事件。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
