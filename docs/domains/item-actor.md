# 掉落物（IItemActor）

在客户端显示掉落物实体。该实体只存在于客户端，服务端没有对应实体。

协议：`AddItemActor`、`SetActorMotion`、`TakeItemActor`、`RemoveActor`。

脚本用法见[掉落物生成](../lse/item-actor.md)。

## 接口

```cpp
struct ItemActorSpec {
    std::string itemId;   // "minecraft:diamond"
    int         aux{0};
    int         count{1};
    float       x{0}, y{0}, z{0};
    int         dimension{0};
    float       vx{0}, vy{0}, vz{0};
};

IItemActor& itemActors();

int64_t spawn(ItemActorSpec const& spec);              // <0 表示失败
bool    setMotion(int64_t id, float vx, float vy, float vz);
bool    moveTo(int64_t id, float x, float y, float z);
bool    take(int64_t id, std::string const& playerName);
bool    destroy(int64_t id);
void    destroyAll();
bool    exists(int64_t id) const;
std::vector<int64_t> getAllIds() const;
```

## 说明

- 只发送给同维度的玩家。生成时该维度没有玩家则返回 -1。
- `setMotion` 设置当前速度，可重复调用。
- 物品包含物品 id、数量与 aux，不携带 NBT。需要自定义名称的物品请使用[物品显示](item-display.md)。
- `take` 只播放动画并删除实体，不发放物品。
- 实体号段为 `0x70`（`core/VirtualActorIds.h` 的 `actorIds::itemActor`），不参与交互事件路由。
