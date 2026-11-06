# 1.15 ISulfurDisplay · 硫磺方块

完整签名（`#include "HologramLib/HologramLib.h"`）：

`IHologramLib::sulfurDisplays()`。第二种摆放方式（与 `IItemDisplay` 的"狐狸 + 物品"并列）：
生成一只 `minecraft:sulfur_cube`，把要展示的方块/物品**装进它的主手** —— 行为包里立方体就是靠
`slot.weapon.mainhand` + `behavior.equip_item` 拿着"吞下去"的方块（客户端按装备渲染）。

外观档位 `minecraft:sulfur_cube_archetype` 是行为包里 `client_sync: true` 的 enum 属性，走**手写的
`ChangeMobProperty`(182)** 下发（26.40 线格式: Actor Id(int64 压缩) → Property Name(string) → Bool →
String → Int32(压缩) → Float）。`AddActor` 的 `PropertySyncData` 只装得下按索引的 int/float，装不下 enum，
所以属性必须在实体已被客户端认识之后发 —— 库里在 spawn 后自动补发，respawn 后重放。

本域**完全委托给 `ICustomEntity`**（立方体就是一只自定义实体），缩放 / 视距 / 可见玩家白名单 / 逐客户端
朝向等能力直接继承；新增的协议内容只有 `ChangeMobProperty`。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生成 | create | `(SulfurDisplaySpec const&) -> int64_t` | 返回展示 id（<0 = 失败） |
| **吞方块** | setBlock | `(int64_t id, ContainerMenuItem const&) -> bool` | 换"吞下去"的方块（改主手装备 → 重建一次实体） |
| 外观 | setArchetype | `(int64_t id, std::string const&) -> bool` | 换外观档位（`ChangeMobProperty`；空串 = 清掉属性；**不重建实体**） |
| **隐身** | setInvisible | `(int64_t id, bool) -> bool` | 隐身开关（spec 上的 `invisible` 参数同义） |
| | setScale | `(int64_t id, float) -> bool` | 缩放（0.0625~10） |
| 生命周期 | destroy / destroyAll / exists / getAllIds / entityIdOf | — | `entityIdOf` 返回背后的自定义实体 id（诊断用） |

```cpp
struct SulfurDisplaySpec {
    float       x{0}, y{64}, z{0};
    int         dim{0};
    ContainerMenuItem block;                    // **吞下去的方块**（核心参数; 走主手装备）
    std::string archetype{"regular"};           // none/regular/bouncy/sticky/hot/explosive/light/...
    int         variant{2};                     // 1=小 / 2=中（含方块那一档）
    bool        invisible{true};                // **隐身参数**（**默认开**: 实测隐身时方块照常渲染 → 只留内容）
    float       scale{1.0f};
    double      viewDistance{0.0};
    std::vector<std::string> visiblePlayers;    // 空 = 全员可见
};
```

**隐身默认开（实测）**：26.40 本机客户端上确认 —— 立方体隐身时，主手里"吞下去的方块"照常渲染，
所以 `invisible` 默认 `true`，默认观感就是"一个方块浮在那里"（立方体本体不可见）；要看立方体本体就设 `false`。
（NPC 载体的头像是会被隐身一起抹掉的，两个实体行为不同，不要套用。）

**"吞生物"没有做**：协议层实体没有 AI，真正吞并/消化做不到；试过让另一个实体骑在立方体上做近似，
骑乘位置与碰撞都调不出"被吞进去"的观感（实测不成立），已整体移除 —— 本域只做方块与隐身。

**另外**: `ICustomEntity::setMobProperty(id, name, value)` / `clearMobProperties(id)` 同批开放 ——
凡是 `client_sync` 的实体属性都能按名下发（硫磺立方体的档位就是这么实现的）。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
