# 掉落物（IItemActor）

协议层掉落物：客户端凭空出现一只掉落物实体，**服务端没有它**。

- 不参与拾取判定，不进存档，没有服务端物理
- 运动由客户端算，所以 `setMotion` 随时可以改速度与方向
- 适合表演（奖励飞出、抽奖、剧情道具），不适合"玩家真能捡走"

脚本侧见 [掉落物生成](../lse/item-actor.md)。

## 接口

```cpp
struct ItemActorSpec {
    std::string itemId;   // "minecraft:diamond"
    int         aux{0};
    int         count{1};
    float       x{0}, y{0}, z{0};
    int         dimension{0};
    float       vx{0}, vy{0}, vz{0};   // 初速度
};

IItemActor& itemActors();

int64_t spawn(ItemActorSpec const& spec);              // <0 = 失败
bool    setMotion(int64_t id, float vx, float vy, float vz);
bool    moveTo(int64_t id, float x, float y, float z);
bool    take(int64_t id, std::string const& playerName);
bool    destroy(int64_t id);
void    destroyAll();
bool    exists(int64_t id) const;
std::vector<int64_t> getAllIds() const;
```

## 协议

`AddItemActor`（生成）· `SetActorMotion`（改速度）· `TakeItemActor`（拾取动画）· `RemoveActor`（删除）。

## 为什么物品不带 NBT

BDS 那份 `NetworkItemStackDescriptor` 的 payload 构造没有导出（默认构造与赋值都在 DLL 里，
只有 `(ItemActor&)` 那个构造是 MCAPI，而模组侧造不出真的 `ItemActor`），所以走不了 BDS 的包类。
sculk 那套包能用，但它的物品描述符 NBT 方言与 BDS 不一致 —— 手写必错，客户端解析越界会崩。

结论：这条路径只发**数字物品 id + 数量 + aux**，不带 UserData。地上一个普通钻石本来也不需要 NBT。
要"带自定义名字的物品"用 [物品显示](item-display.md) 那条路（FMBE）。

## 其他边界

- **只发给同维度的玩家。** `spawn` 时该维度没人在线返回 -1，不占 id。
- `take` 只给指定玩家播"飞过去"，然后把实体对所有人删除；**不给物品**。
- 实体号段是 `0x70` 段（`core/VirtualActorIds.h` 的 `actorIds::itemActor`），
  **不参与 ghost 交互路由** —— 地面掉落物不该被当成"点了要回调"的对象。

## 还没实测

外观、抛物线、`take` 的动画、多人同屏时的位置同步。
