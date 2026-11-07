# 1.15 ISulfurDisplay · 硫磺方块

生成一只隐身的 `minecraft:sulfur_cube`，把要展示的方块「吞」在它主手里 —— 客户端按装备渲染，看起来就是方块浮在那里。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().sulfurDisplays()`
- 坐标：世界坐标，单位方块；维度 `0` 主世界 / `1` 下界 / `2` 末地
- 载体：一只协议层自定义实体 `minecraft:sulfur_cube`，内容装在主手（装备槽 `0`）
- 隐身：`SulfurDisplaySpec::invisible` 默认 `true` —— 默认观感就是「一个方块浮在那里」

!!! info "外观档位走手写的 ChangeMobProperty"

    档位 `minecraft:sulfur_cube_archetype` 是行为包里 `client_sync: true` 的 enum 属性，
    走手写的 `ChangeMobProperty`(182) 下发（26.40 线格式：Actor Id(int64 压缩) → Property Name(string)
    → Bool → String → Int32(压缩) → Float）。`AddActor` 的 `PropertySyncData` 只装得下按索引的
    int / float，装不下 enum，所以属性必须在实体已被客户端认识之后发 —— 库里在 spawn 后自动补发，
    respawn 后重放。

!!! note "本域完全委托给 ICustomEntity"

    立方体就是一只自定义实体，所以缩放 / 视距 / 可见玩家白名单 / 逐客户端朝向等能力直接继承；
    新增的协议内容只有 `ChangeMobProperty`。凡是 `client_sync` 的实体属性都能用
    `ICustomEntity::setMobProperty(id, name, value)` / `clearMobProperties(id)` 按名下发（档位就是这么实现的）。

## 数据结构

`SulfurDisplaySpec` —— 一只展示：

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `x` / `y` / `z` | `float` | `0` / `64` / `0` | 世界坐标，单位方块 |
| `dim` | `int` | `0` | 维度 id：`0` 主世界 / `1` 下界 / `2` 末地 |
| `block` | `ContainerMenuItem` | 空 | **吞下去的方块 / 物品**，走主手装备；`type` 空 = 空手 |
| `archetype` | `std::string` | `"regular"` | 外观档位；空串 = 不下发（保持客户端默认） |
| `variant` | `int` | `2` | 变种：`1` = 小、`2` = 中（含方块那一档） |
| `invisible` | `bool` | `true` | 本体是否隐身；`true` 时只剩被吞的方块可见 |
| `scale` | `float` | `1.0f` | 缩放，有效 `0.0625 ~ 10` |
| `viewDistance` | `double` | `0.0` | 视距（格）；`0` = 不限（同维度全员可见） |
| `visiblePlayers` | `std::vector<std::string>` | 空 | 白名单；空 = 全员可见，非空 = 只对这些玩家可见 |

`ContainerMenuItem` 的字段（`type` / `count` / `damage` / `name` / `lore`）与虚拟容器共用，
详见 [虚拟容器](container-menu.md#数据结构)：`name` / `lore` 会写进主手物品的 `display` NBT。

## 生命周期

### 创建一只硫磺方块展示

`int64_t create(SulfurDisplaySpec const& spec)`

- 参数：
  - `spec` : `SulfurDisplaySpec const&`<br>位置、维度、吞下去的方块与外观参数
- 返回值：展示 id（`>= 0`，后续调用都用它）；`-1` = 失败（背后的自定义实体没创建出来）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& sulfur = hologramlib::IHologramLib::getInstance().sulfurDisplays();

    hologramlib::SulfurDisplaySpec spec;
    spec.x         = 100.5f;
    spec.y         = 65.0f;
    spec.z         = -200.5f;
    spec.dim       = 0;
    spec.block.type  = "minecraft:bookshelf";
    spec.block.name  = "§6书架";
    spec.archetype = "regular";
    spec.invisible = true;                 // 默认就是 true

    int64_t displayId = sulfur.create(spec);
    if (displayId < 0) {
        return;
    }
    ```

!!! note "白名单会重建一次实体"

    `visiblePlayers` 非空时，库会先创建实体再套白名单（内部会重建一次），
    重建后主手装备不一定跟着重发，所以库里会补一次主手同步 —— 消费方不用自己补。

### 销毁一只展示

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>展示 id
- 返回值：已销毁 `true`；`false` = 该 id 不存在
- 返回值类型：`bool`
- 示例：

    ```cpp
    sulfur.destroy(displayId);
    ```

### 销毁全部展示

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    sulfur.destroyAll();
    ```

### 查询展示是否还在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>展示 id
- 返回值：存在 `true`；已销毁或不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (sulfur.exists(displayId)) {
        sulfur.setInvisible(displayId, true);
    }
    ```

### 列出所有展示

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：展示 id 数组；一个都没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t id : sulfur.getAllIds()) {
        sulfur.destroy(id);
    }
    ```

### 取展示背后的实体 id

`int64_t entityIdOf(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>展示 id
- 返回值：背后自定义实体的库内 id（诊断 / 传给 `ICustomEntity` 用）；`-1` = 该展示不存在
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    int64_t entityId = sulfur.entityIdOf(displayId);
    if (entityId >= 0) {
        // 可以直接拿这个 id 去调 ICustomEntity 的方法
    }
    ```

## 内容与外观

### 换掉吞下去的方块 / 物品

`bool setBlock(int64_t id, ContainerMenuItem const& item)`

- 参数：
  - `id` : `int64_t`<br>展示 id
  - `item` : `ContainerMenuItem const&`<br>新内容；`type` 空 = 空手。装备只认 `type` / `damage` / `name` / `lore`，`count` 不参与渲染
- 返回值：已换 `true`；`false` = 该展示 id 不存在，或背后的实体已经不在了
- 返回值类型：`bool`
- 示例：

    ```cpp
    hologramlib::ContainerMenuItem block;
    block.type = "minecraft:gold_block";
    block.name = "§6金块";
    sulfur.setBlock(displayId, block);
    ```

改的是主手装备（`slot 0`），会重建一次实体 —— 客户端看到一次重生，但不闪退。
需要高频换内容（比如每次点击都换）时请谨慎，重建有代价。

### 换外观档位

`bool setArchetype(int64_t id, std::string const& archetype)`

- 参数：
  - `id` : `int64_t`<br>展示 id
  - `archetype` : `std::string const&`<br>档位名，取值见下表；空串 = 清掉属性，回到客户端默认外观
- 返回值：已换 `true`；`false` = 该展示 id 不存在，或属性没发出去
- 返回值类型：`bool`
- 示例：

    ```cpp
    sulfur.setArchetype(displayId, "sticky");   // 换成黏糊糊那档
    sulfur.setArchetype(displayId, "");         // 清掉属性，回到默认
    ```

| 档位 | 观感 |
|---|---|
| `none` | 不动、无特殊表现 |
| `regular` | 默认档（`SulfurDisplaySpec` 的默认值） |
| `bouncy` / `slow_bouncy` | 弹跳（快 / 慢） |
| `slow_flat` / `fast_flat` | 压扁再回弹（慢 / 快） |
| `light` | 轻 |
| `fast_sliding` / `slow_sliding` | 滑行（快 / 慢） |
| `sticky` | 黏 |
| `high_resistance` | 高抗性 |
| `explosive` | 易爆 |
| `hot` | 灼热 |

换档位**不重建实体**，当场生效。

### 设置本体是否隐身

`bool setInvisible(int64_t id, bool on)`

- 参数：
  - `id` : `int64_t`<br>展示 id
  - `on` : `bool`<br>`true` = 隐身（只剩被吞的方块可见）；`false` = 连立方体本体一起看到
- 返回值：已设置 `true`；`false` = 该展示 id 不存在
- 返回值类型：`bool`
- 示例：

    ```cpp
    sulfur.setInvisible(displayId, false);   // 想看立方体本体
    sulfur.setInvisible(displayId, true);    // 只看方块
    ```

### 设置缩放

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>展示 id
  - `scale` : `float`<br>缩放值，有效 `0.0625 ~ 10`；超出范围下发时钳到边界，`<= 0` 直接失败
- 返回值：已设置 `true`；`false` = 该展示 id 不存在或 `scale <= 0`
- 返回值类型：`bool`
- 示例：

    ```cpp
    sulfur.setScale(displayId, 1.5f);
    ```

## 实测结论与边界

- **隐身默认开**：26.40 本机客户端上确认 —— 立方体隐身时，主手里吞下去的方块照常渲染，
  所以默认观感就是「一个方块浮在那里」。要看立方体本体就设 `false`。
  （NPC 载体的头像是会被隐身一起抹掉的，两个实体行为不同，别套用。）
- **「吞生物」没有做**：协议层实体没有 AI，真正吞并 / 消化做不到；试过让另一个实体骑在立方体上做近似，
  骑乘位置与碰撞都调不出「被吞进去」的观感（实测不成立），已整体移除 —— 本域只做方块与隐身。
- 展示只存在于客户端，服务端没有这只实体：不参与碰撞、不进存档，玩家也打不到它。

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.23.0 · 26.40.5 | `ISulfurDisplay` 首次发布：主手吞方块 + 隐身默认开 + 外观档位属性；同批开放 `ICustomEntity::setMobProperty` / `clearMobProperties` |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <cstdint>

void spawn_bookshelf_display() {
    auto& sulfur = hologramlib::IHologramLib::getInstance().sulfurDisplays();

    hologramlib::SulfurDisplaySpec spec;
    spec.x           = 100.5f;
    spec.y           = 65.0f;
    spec.z           = -200.5f;
    spec.dim         = 0;
    spec.block.type  = "minecraft:bookshelf";
    spec.block.count = 1;
    spec.block.name  = "§6书架";
    spec.block.lore  = {"§7点一下试试"};
    spec.archetype   = "regular";
    spec.variant     = 2;          // 2 = 中（含方块那一档）
    spec.invisible   = true;       // 只看到书架
    spec.scale       = 1.0f;
    spec.viewDistance = 0.0;       // 不限视距

    int64_t displayId = sulfur.create(spec);
    if (displayId < 0) {
        return;
    }

    // 过一会儿换成金块
    hologramlib::ContainerMenuItem gold;
    gold.type = "minecraft:gold_block";
    gold.name = "§6金块";
    sulfur.setBlock(displayId, gold);

    // 想看到立方体本体
    sulfur.setInvisible(displayId, false);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
