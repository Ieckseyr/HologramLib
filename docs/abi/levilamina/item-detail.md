# 1.4 IItemDetail · 物品详情

把物品堆翻译成一行可读文本（例如 `Diamond x64`），按世界坐标画进指定维度。
它本质就是**一行悬浮字**，适合做提示文本、掉落物说明、NPC 头顶的物品名。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().itemDetails()`
- 坐标：世界坐标，单位方块
- 维度：`0` 主世界 / `1` 下界 / `2` 末地
- 返回的 id：**悬浮字 id**（属于 `IHologramText` 的 id 空间），可以继续用 `holograms()` 精修

下面示例里的 `tip` / `id` 都指 `show` 返回的那个悬浮字 id。

!!! info "这个域只有两个方法"

    `show` 造一行字并画到某个维度，`hide` 把它删掉。
    换位置、改颜色、加背景、跟随玩家都不在这里 —— 拿到 id 后交给
    `lib.holograms().setLocation(...)` / `setColor(...)` / `setFollowPlayer(...)`。

## 显示

### 显示一行物品名

`int64_t show(int dimId, float x, float y, float z, std::string const& itemId, int aux, int count, std::string const& customText = "")`

- 参数：
  - `dimId` : `int`<br>画到哪个维度：`0` 主世界 / `1` 下界 / `2` 末地
  - `x` / `y` / `z` : `float`<br>这行字的世界坐标，单位方块；`.5` 通常落在方块中心
  - `itemId` : `std::string const&`<br>物品标识符，例如 `minecraft:diamond`；写错也不会拒绝，只是名字退化成原始 id
  - `aux` : `int`<br>物品附加值（data 值），参与物品名解析；没有就填 `0`
  - `count` : `int`<br>数量；只有 `> 1` 才在名字后追加 ` xN`，`0` / `1` / 负数都不加
  - `customText` : `std::string const&`<br>自定义文本；空串 = 用自动物品名，非空则**完全替代**（连 ` xN` 也不再拼），支持 `§` 颜色码与 `{变量}`
- 返回值：悬浮字 id（`>= 1`）；创建失败或该维度画不出来返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& lib = hologramlib::IHologramLib::getInstance();

    // 自动物品名：Diamond x64
    int64_t tip = lib.itemDetails().show(0, 100.5f, 65.0f, 100.5f, "minecraft:diamond", 0, 64, "");
    if (tip < 0) {
        return; // 没画出来
    }

    // 自定义文案：不再拼物品名与数量
    int64_t ad = lib.itemDetails().show(0, 100.5f, 66.0f, 100.5f, "minecraft:diamond", 0, 1, "§b今日特价");
    ```

!!! note "物品名是怎么解析的"

    解析链：`ItemRegistry` 取物品 → `ItemStack::getDescriptionId()` → `I18n`（库内固定用 `en_US`
    语言资源）→ `getName` / `getDescriptionName` → 全部失败则原样显示 `itemId`。
    所以自动名字通常是英文物品名，**不随客户端语言变化**；要中文或自定义文案就自己写 `customText`。
    这条链失败不阻断调用，最差情况是屏幕上出现原始 id。

## 隐藏

### 删除这行物品名

`bool hide(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>`show` 返回的悬浮字 id
- 返回值：确实销毁了 `true`；id 不存在或已销毁 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& lib = hologramlib::IHologramLib::getInstance();
    bool ok   = lib.itemDetails().hide(tip);
    ```

!!! tip "hide 与 holograms 的等价关系"

    `hide(id)` 等价于 `lib.holograms().destroy(id)`：数据一起删掉。
    只想临时藏起来、之后还能显示回来，用 `lib.holograms().remove(id)` + `draw(id)`。

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

// 在一个钻石堆上方挂一行 "Diamond x64"，并让它始终可见、放大一点
void spawn_diamond_tip() {
    auto& lib = hologramlib::IHologramLib::getInstance();

    int64_t id = lib.itemDetails().show(0, 100.5f, 65.0f, 100.5f, "minecraft:diamond", 0, 64, "");
    if (id < 0) {
        return;
    }

    // show 返回的是悬浮字 id，整块样式继续交给 holograms()
    lib.holograms().setColor(id, 1.0f, 1.0f, 1.0f, 1.0f);
    lib.holograms().setScale(id, 1.2f);
    lib.holograms().setDepthTest(id, false); // false = 穿墙可见（默认）
    lib.holograms().refresh(id);

    // 不需要时整行删掉：
    // lib.itemDetails().hide(id);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
