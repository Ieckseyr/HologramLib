# 客户端附加 · C++ 接口

26.51.7 新增三个接口，都很小；另外两个既有接口在尾部各加了一个方法。
完整签名见 `include/hologramlib/HologramLib.h`。

---

## 1.17 IItemActor（协议层掉落物，26.51.7）

客户端凭空出现一只掉落物实体。**服务端没有它**：不参与拾取判定、不进存档、没有物理，
服务端那些"捡东西"的逻辑碰不到它。

```cpp
struct ItemActorSpec {
    std::string itemId;   // "minecraft:diamond"
    int         aux{0};
    int         count{1};
    float       x{0}, y{0}, z{0};
    int         dimension{0};
    float       vx{0}, vy{0}, vz{0}; // 初速度
};

IItemActor& itemActors();
int64_t spawn(ItemActorSpec const& spec);            // <0 = 失败
bool    setMotion(int64_t id, float vx, float vy, float vz);
bool    moveTo(int64_t id, float x, float y, float z);
bool    take(int64_t id, std::string const& playerName);
bool    destroy(int64_t id);
void    destroyAll();
bool    exists(int64_t id) const;
std::vector<int64_t> getAllIds() const;
```

协议：`AddItemActor`（生成）· `SetActorMotion`（改速度）· `TakeItemActor`（拾取动画）· `RemoveActor`（删除）。

**几个边界，先看再写：**

- **只发给同维度的玩家。** `spawn` 时该维度没人在线直接返回 -1、不占 id。
- **`setMotion` 是"设定当前速度"**，不是一次性冲量。掉头、悬停（`0,0,0`）、加速都用它，
  可以反复调；客户端按自己那套物理继续飞。
- **物品不带 NBT。** 走的是数字物品 id + 数量 + aux，没有 UserData —— 因为 BDS 那份
  `NetworkItemStackDescriptor` 的 payload 构造没导出，模组侧造不出来，而 sculk 那套 NBT 方言
  与物品描述符不一致（手写必错，客户端解析越界会卡死，`fakeinv` 里记过这个坑）。
  要"带自定义名字的物品"继续用 `IItemDisplay`（FMBE）。
- **`take` 不会真的给物品。** 它只给指定玩家播一条"飞过去"的动画，然后把实体对所有人删掉；
  要不要发物品是消费方的事。
- actor 号段是 `0x70` 段（`core/VirtualActorIds.h` 的 `actorIds::itemActor`），**不在 ghost 路由范围内** ——
  地面掉落物不该被当成"点了要回调"的对象。

---

## 1.18 IFormControl（客户端表单强制关闭，26.51.7）

库不生成表单，只发 `ClientboundCloseForm` 把客户端当前打开的表单收起来。

```cpp
bool closeForm(std::string const& playerName); // 玩家不在线 = false
int  closeAllForms();                          // 返回发给了几个在线玩家
```

用途：玩家 ESC 不掉的界面、被别的插件顶掉的界面、过场需要收界面。
表单本身仍归 LSE / LeviLamina Form API / MeowMenu。

---

## 1.19 ILocatorBar（方位条航点，26.51.7）

按玩家维护一组航点，协议是 `LocatorBar`。

```cpp
bool setWaypoint(playerName, key, x, y, z, dimension, color = -1, visible = true);
bool removeWaypoint(playerName, key);
int  clearWaypoints(playerName);
int  waypointCount(playerName) const;
```

- `key` 由调用方起名；同一个 key 再 `set` 就是更新位置。
- 航点身份是 UUID，**库按 key 稳定派生**（FNV-1a 两个种子），所以更新/删除不需要调用方保管 UUID。
- `color` 是 ARGB，`-1` = 客户端默认色；`visible=false` 只是暂时隐藏，记录还在。
- `Remove` 用同一套 key 派生，不需要提前保存 UUID。

**尚未实测**：图标 / 贴图要不要资源包配合、只给位置能不能显示、不同 `dimension` 值的效果。
第一次验按"位置 + visible"两项来。

---

## 1.20 既有接口的尾部追加（26.51.7）

| 接口 | 新增 | 协议 | 说明 |
|---|---|---|---|
| `IContainerMenu` | `blockEvent(menuId, b0, b1)` | `BlockEvent` | 在假箱子的坐标发方块事件。箱子盖 `b0=1` 开 / `0` 合（**未实测**）；参数原样透传，别的方块事件也能用 |
| `IPlayerNpc` | `playEmote(id, pieceId, lengthTicks)` | `Emote` | 原版表情动作，`pieceId` 是资源包里的表情标识；`lengthTicks=0` 用客户端时长；只发给已见过该 NPC 的玩家 |

两者都是**尾部追加**，`HOLOGLIB_ABI_STAMP` 不变；但虚表长度变了，
用旧头文件编译的消费方要重新编译（`abi/v1.32.0.json` 是这一版的基线）。
