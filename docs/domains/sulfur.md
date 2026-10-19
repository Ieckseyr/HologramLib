# 硫磺立方体

硫磺立方体展示：主手拿方块 + 默认隐身 + 外观档位；底层复用自定义实体。


## 硫磺立方体展示

第二种「摆一个方块」的方式（与 FMBE 的「狐狸 + 物品」并列）：生成一只 `minecraft:sulfur_cube`，把方块**装进它的主手**。

```cpp
hologramlib::SulfurDisplaySpec spec;
spec.block = "minecraft:bookshelf";   // 它「吞」下去的东西
spec.invisible = true;                // 默认就是 true
auto id = lib.sulfurDisplays().create(spec);
```

**这不是我们发明的花招**：行为包里 `entities/sulfur_cube.json` 就是靠 `slot.weapon.mainhand` + `behavior.equip_item` 拿「吞下去」的方块，客户端按装备渲染。所以看起来就是方块被吞在立方体上。放**方块类物品**观感最正。

### 隐身是一个参数，而且默认就是开

**实测（26.40 本机客户端）：立方体隐身时，主手里「吞下去的方块」照常渲染。**
所以 `invisible = true`（默认）的观感就是**一个方块浮在那里**，立方体本体看不见。要连立方体一起看就设 `false`。

> 对比：**NPC 载体的头像是会被隐身一起抹掉的** —— 两个实体行为不同，别套用。

### 外观档位

走**手写的 `ChangeMobProperty`(182)** 下发 `minecraft:sulfur_cube_archetype`（行为包里 `client_sync: true` 的 enum，13 档）：
`none` / `regular` / `bouncy` / `slow_bouncy` / `slow_flat` / `fast_flat` / `light` / `fast_sliding` / `slow_sliding` / `sticky` / `high_resistance` / `explosive` / `hot`。

因为它是 **enum**，装不进出生包的 `PropertySyncData`，所以必须在实体已被客户端认识之后再发 —— 库里会在 spawn 之后自动补发、respawn 后重放。

### 本域完全委托给 `ICustomEntity`

立方体就是一只自定义实体，所以**缩放 / 视距 / 可见玩家白名单 / 逐客户端朝向全都直接继承**。本域新增的协议内容只有 `ChangeMobProperty`。

> **「吞生物」没有做，也不打算做**：协议层实体没有 AI，真正的吞并 / 消化做不到。试过「让另一个实体骑在立方体上」做近似，骑乘位置与碰撞都调不出「被吞进去」的观感（实测不成立），已整体移除。本域只做**方块 + 隐身**。

---
