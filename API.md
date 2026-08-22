# HologramLib API 参考

- 库版本：1.6.0（`HOLOGLIB_API_VERSION 0x010600`）
- 唯一公开头：`include/hologramlib/HologramLib.h`

## API 稳定性契约

| 层级 | 冻结内容 | 演进规则 |
|------|----------|----------|
| C++ 接口 | 全部纯虚方法签名与语义（ABI 稳定：实现对象在 DLL 内创建，消费者只持引用） | 只在接口**尾部追加**方法、只新增函数；永不修改/删除 |
| C++ 宏 | `HOLOGLIB_API_VERSION`、`HOLOGLIB_API`、`hologramlib` 命名空间、枚举值 | 只追加枚举值 |
| LSE 命名空间 | 单一命名空间 `HologramLib` 全部函数名（四域前缀）、参数顺序、返回值类型 | 只增不改不删 |
| 版本协商 | `IHologramLib::version()`（BCD：0x010500 = 1.5.0） | 随发布递增 |

破坏兼容仅允许发生在大版本（2.0.0），届时提供全新接口名并长期保留旧接口最终冻结实现。

`src/` 目录一切内容均为内部实现，**不属于 API**。

---

## 1. C++ 接口

### 1.1 入口

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // 唯一 dllexport 工厂
        IShapeDrawer&  shapes();                     // 形状渲染
        IHologramText& holograms();                  // 悬浮字全息
        IItemDetail&   itemDetails();                // 物品详情
        bool     isLseAvailable();                   // LSE 兼容层是否已挂载
        uint32_t version();                          // 0x010500
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）
- 坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；建议主线程调用（发包在调用线程执行）

### 1.2 IShapeDrawer（形状渲染）

| 方法 | 签名 | 说明 |
|------|------|------|
| createText | `(float x, float y, float z, std::string const& text) -> int64_t` | 文本形状 |
| createLine | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 线段 |
| createBox | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 盒体（两点对角） |
| createCircle | `(x, y, z, scale: float) -> int64_t` | 圆 |
| createSphere | `(x, y, z, scale: float) -> int64_t` | 球 |
| createArrow | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 箭头 |
| setColor | `(int64_t id, float r, float g, float b, float a) -> bool` | RGBA |
| setScale | `(int64_t id, float scale) -> bool` | 缩放 |
| setDuration | `(int64_t id, float seconds) -> bool` | 存留时长 |
| setDimension | `(int64_t id, int dimId) -> bool` | 维度 |
| setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| setText | `(int64_t id, std::string const& text) -> bool` | 文本内容 |
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | 旋转（弧度） |
| clearRotation | `(int64_t id) -> bool` | 清除旋转 |
| draw | `(int64_t id) -> bool` | 全维度可见者绘制 |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | 指定玩家 |
| drawToDimension | `(int64_t id, int dimId) -> bool` | 指定维度 |
| remove | `(int64_t id) -> bool` | 隐藏（保留数据） |
| update | `(int64_t id) -> bool` | 可见时原地重发（同 networkId 覆盖，无闪烁） |
| destroy | `(int64_t id) -> bool` | 销毁 |
| destroyAll | `() -> void` | 全部销毁 |
| exists | `(int64_t id) -> bool` | 存在性 |
| type | `(int64_t id) -> ShapeType` | 形状类型 |

`ShapeType` 枚举（与 LSE `getShapeType` 数值一致）：

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

> 注：旧版 `FilledQuad = 6` 已移除；数值 6 保留空洞，永不复用。

### 1.3 IHologramText（悬浮字全息）

整块多行文本单一背景框渲染（`\n` 合并所有行）；变更后调用 `refresh` 原地重绘（同 networkId 覆盖，无闪烁）。行索引 0 起。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create | `(float x, float y, float z) -> int64_t` | 创建 |
| | destroy | `(int64_t id) -> bool` | 销毁 |
| | destroyAll | `() -> void` | 全部销毁 |
| 行管理 | addLine | `(int64_t id, std::string const& text) -> bool` | 追加行 |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | 行文本 |
| | setLineScale | `(int64_t id, int line, float scale) -> bool` | 行缩放 |
| | removeLine | `(int64_t id, int line) -> bool` | 移除行 |
| | clearLines | `(int64_t id) -> bool` | 清空行 |
| | getLineCount | `(int64_t id) -> int` | 行数 |
| 颜色 | setColor | `(int64_t id, r, g, b, a: float) -> bool` | 整体纯色 |
| | setLineColor | `(int64_t id, int line, r, g, b, a: float) -> bool` | 行纯色 |
| | setLineGradient | `(int64_t id, int line, r1,g1,b1, r2,g2,b2: float) -> bool` | 行双色渐变 |
| | setLineRainbow | `(int64_t id, int line, float speed) -> bool` | 行彩虹 |
| 动画 | setLineScroll | `(int64_t id, int line, int direction, float speed) -> bool` | 滚动：0=无 1=左 2=右 |
| | setVerticalAnimation | `(int64_t id, int type, float speed, float range) -> bool` | 垂直：0=无 1=弹跳 2=滚动 |
| | setLineSpacing | `(int64_t id, float spacing) -> bool` | 行距 |
| 位置 | setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | 跟随玩家 |
| | clearFollowPlayer | `(int64_t id) -> bool` | 取消跟随 |
| 显示 | draw / drawToDimension / drawToPlayer / remove | 同形状语义 | |
| | refresh | `(int64_t id) -> bool` | 重解析变量并原地重发 |
| 驱动 | tick | `(float deltaTime) -> void` | 动画推进（滚动/跟随） |

内置变量（PAPI 翻译后兜底解析）：`{time}` `{online}` `{tps}` `{player}`。
外部占位符 `%name%` / `{name}` 经 LseBridge 调 `MeowPAPI::translateString(WithPlayer)`（可选，缺席原样保留）。

### 1.4 IItemDetail（物品详情）

```cpp
// 在 (x,y,z) 显示 "本地化物品名 xN"（自动翻译; count<=1 无数量后缀）
// customText 非空则完全替代自动文本（支持 § 颜色码与 {变量}）
// 返回悬浮字 ID（可用 holograms() 继续精修）
int64_t show(int dimId, float x, float y, float z,
             std::string const& itemId, int aux, int count,
             std::string const& customText = "");
bool hide(int64_t id);
```

物品名解析链：`ItemRegistry（HashedString）→ ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → 兜底原样显示 ID（不阻断）。

### 1.5 IItemDisplay（FMBE 物品悬浮显示，1.6.0 追加）

配置驱动的物品/方块悬浮展示（FMBE 狐狸+发包）。全部变换字段为**常量数字或 Molang 表达式字符串**；持久化由消费者负责（库只管渲染）。

```cpp
struct ItemDisplayConfig {
    std::string item{"minecraft:diamond"}; // 物品标识符
    int         itemAux{0};                // 附加值
    float       x{0}, y{64}, z{0};         // 世界坐标
    int         dimension{0};
    std::string offsetX{"0"}, offsetY{"-4"}, offsetZ{"0"};           // v.xpos/ypos/zpos 模型单位平移
    std::string baseOffsetX/Y/Z{"0"};                                 // v.xbasepos... 渲染像素偏移
    std::string rotX{"180"}, rotY{"0"}, rotZ{"180"};                 // 三轴旋转（度）
    std::string scale{"0.375"};                                       // 缩放（方块模式建议 0.5）
    std::string extendScale{"1"}, extendRotX{"-90"}, extendRotY{"0"}; // 方块模式二段变换
    int    mode{0};                        // 0=auto 1=item 2=block
    double viewDistance{64.0};             // <=0 无限制
    bool   enabled{true};
};
```

| 方法 | 签名 | 说明 |
|------|------|------|
| create | `(ItemDisplayConfig const&) -> int64_t` | 创建（<0 失败） |
| destroy / destroyAll | `(int64_t) -> bool` / `() -> void` | 销毁 |
| exists | `(int64_t) -> bool` | 存在性 |
| get | `(int64_t, ItemDisplayConfig&) -> bool` | 拷贝输出当前配置 |
| setItem | `(int64_t, std::string const&, int) -> bool` | 换物品 |
| setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 仅改坐标 |
| setOffset / setBaseOffset | `(int64_t, ox, oy, oz: string) -> bool` | 平移（Molang） |
| setRotation | `(int64_t, rx, ry, rz: string) -> bool` | 三轴旋转（Molang） |
| setScale | `(int64_t, scale: string) -> bool` | 缩放（Molang） |
| setExtend | `(int64_t, scale, rx, ry: string) -> bool` | 方块二段变换 |
| setMode / setEnabled / setViewDistance | — | 行为 |
| rotateY | `(int64_t, float) -> bool` | 偏航叠加增量（常量/表达式自适应） |
| getAllIds | `() -> std::vector<int64_t>` | 全部 ID |

可见性由库内 Level tick hook 自动同步（每 20 tick：同维度 + 可见距离内玩家自动生成/移除；玩家断线自动清理）；属性变更即时生效（对已见玩家原子 despawn→respawn）。

---

## 2. LSE 接口（ll.import 统一命名空间）

LegacyRemoteCall（lrca）在场时自动导出。**单命名空间 `HologramLib`**，四域前缀区分能力域：

| 前缀域 | 能力 | 对应 C++ 接口 | 函数数 |
|--------|------|---------------|--------|
| `shape*` | 形状渲染 | IShapeDrawer | 36 |
| `holo*` | 悬浮字全息 | IHologramText | 25 |
| `gradient*` | 渐变线 | GradientLineManager | 11 |
| `itemDetail*` | 物品详情 | IItemDetail | 2 |

缺席时安全降级（`ll.import` 得 null）。

类型记法：`f`=浮点 `i`=整数 `s`=字符串 `b`=布尔 `[i]`=int64 数组。除注明外 id 均为 int64。

### 2.1 shape*（形状，36 函数）

**创建**

| 函数 | 签名 |
|------|------|
| shapeCreateText | `(x: f, y: f, z: f, text: s) -> i` |
| shapeCreateLine | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| shapeCreateBox | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| shapeCreateCircle | `(x: f, y: f, z: f, scale: f) -> i` |
| shapeCreateSphere | `(x: f, y: f, z: f, scale: f) -> i` |
| shapeCreateArrow | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |

**属性**

| 函数 | 签名 |
|------|------|
| shapeSetText | `(id, text: s) -> b` |
| shapeGetText | `(id) -> s` |
| shapeSetLocation | `(id, x: f, y: f, z: f) -> b` |
| shapeGetLocation | `(id) -> [f,f,f]` |
| shapeSetColor | `(id, r: f, g: f, b: f, a: f) -> b` |
| shapeGetColor | `(id) -> [f,f,f,f]` |
| shapeSetScale | `(id, scale: f) -> b` |
| shapeSetDuration | `(id, seconds: f) -> b` |
| shapeSetDimension | `(id, dimId: i) -> b` |
| shapeSetRotation | `(id, pitch: f, yaw: f, roll: f) -> b` |
| shapeClearRotation | `(id) -> b` |
| shapeGetRotation | `(id) -> [f,f,f]` |
| shapeGetShapeType | `(id) -> i`（0..5，见 ShapeType） |

**显示**

| 函数 | 签名 |
|------|------|
| shapeDraw | `(id) -> b` |
| shapeDrawToPlayer | `(id, playerName: s) -> b` |
| shapeDrawToDimension | `(id, dimId: i) -> b` |
| shapeDrawBatch | `(ids: [i]) -> b` |
| shapeRemove | `(id) -> b` |
| shapeRemoveToPlayer | `(id, playerName: s) -> b` |
| shapeRemoveToDimension | `(id, dimId: i) -> b` |
| shapeUpdate | `(id) -> b` |
| shapeUpdateToPlayer | `(id, playerName: s) -> b` |
| shapeUpdateToDimension | `(id, dimId: i) -> b` |

**生命周期与查询**

| 函数 | 签名 |
|------|------|
| shapeDestroy | `(id) -> b` |
| shapeDestroyAll | `() -> nil` |
| shapeDestroyBatch | `(ids: [i]) -> b` |
| shapeFindTextByLocation | `(x: f, y: f, z: f, radius: f) -> [i]` |
| shapeFindTextByLocationAndContent | `(x: f, y: f, z: f, radius: f, text: s) -> i` |
| shapeGetAllShapeIds | `() -> [i]` |
| shapeExists | `(id) -> b` |

### 2.2 holo*（悬浮字，25 函数）

| 函数 | 签名 |
|------|------|
| holoCreate | `(x: f, y: f, z: f) -> i` |
| holoDestroy | `(id) -> b` |
| holoDestroyAll | `() -> nil` |
| holoAddLine | `(id, text: s) -> b` |
| holoSetLineText | `(id, lineIndex: i, text: s) -> b` |
| holoSetLineScale | `(id, lineIndex: i, scale: f) -> b` |
| holoRemoveLine | `(id, lineIndex: i) -> b` |
| holoClearLines | `(id) -> b` |
| holoGetLineCount | `(id) -> i` |
| holoSetColor | `(id, r,g,b,a: f) -> b` |
| holoSetLineColor | `(id, lineIndex: i, r,g,b,a: f) -> b` |
| holoSetLineGradient | `(id, lineIndex: i, r1,g1,b1,r2,g2,b2: f) -> b` |
| holoSetLineRainbow | `(id, lineIndex: i, speed: f) -> b` |
| holoSetLineScroll | `(id, lineIndex: i, direction: i, speed: f) -> b`（方向 0=无 1=左 2=右） |
| holoSetVerticalAnimation | `(id, type: i, speed: f, range: f) -> b`（0=无 1=弹跳 2=滚动） |
| holoSetLineSpacing | `(id, spacing: f) -> b` |
| holoSetLocation | `(id, x: f, y: f, z: f) -> b` |
| holoSetFollowPlayer | `(id, playerName: s, offsetY: f) -> b` |
| holoClearFollowPlayer | `(id) -> b` |
| holoTick | `(deltaTime: f) -> nil`（动画驱动） |
| holoDraw | `(id) -> b` |
| holoDrawToDimension | `(id, dimId: i) -> b` |
| holoDrawToPlayer | `(id, playerName: s) -> b` |
| holoRemove | `(id) -> b` |
| holoRefresh | `(id) -> b`（重解析变量并原地重发） |

### 2.3 gradient*（渐变线，11 函数）

| 函数 | 签名 |
|------|------|
| gradientCreate | `(x1,y1,z1: f, x2,y2,z2: f, segments: i) -> i` |
| gradientSetGradient | `(id, r1,g1,b1,r2,g2,b2: f) -> b` |
| gradientSetRainbow | `(id, speed: f) -> b` |
| gradientSetColor | `(id, r,g,b,a: f) -> b` |
| gradientSetEndpoints | `(id, x1,y1,z1: f, x2,y2,z2: f) -> b` |
| gradientDraw | `(id) -> b` |
| gradientDrawToDimension | `(id, dimId: i) -> b` |
| gradientRemove | `(id) -> b` |
| gradientDestroy | `(id) -> b` |
| gradientDestroyAll | `() -> nil` |
| gradientTick | `(deltaTime: f) -> nil`（彩虹动画驱动） |

### 2.4 itemDetail*（物品详情，2 函数）

| 函数 | 签名 |
|------|------|
| itemDetailShow | `(dimId: i, x,y,z: f, itemId: s, aux: i, count: i, customText: s) -> i` |
| itemDetailHide | `(id) -> b` |

`itemDetailShow`：在 (x,y,z) 显示"本地化物品名 xN"（count<=1 无数量后缀）；`customText` 传 `""` 用自动文本，非空则完全替代（支持 § 颜色码与 `{变量}`）。返回悬浮字 ID，可继续用 `holo*` 精修。

### 2.5 itemDisplay*（FMBE 物品悬浮显示，1.6.0 追加，18 函数）

FMBE（狐狸+发包）技术：隐形狐狸手持物品渲染任意物品/方块的悬浮展示。三轴旋转/函数平移/缩放全部支持 **Molang 表达式**（如 `"math.sin(query.life_time*90)*360"`）。

| 函数 | 签名 |
|------|------|
| itemDisplayCreate | `(x: f, y: f, z: f, dim: i, itemId: s, aux: i) -> i` |
| itemDisplayCreateAdvanced | `(x,y,z: f, dim: i, itemId: s, aux: i, offX,offY,offZ: s, rotX,rotY,rotZ: s, scale: s) -> i` |
| itemDisplayDestroy | `(id) -> b` |
| itemDisplayDestroyAll | `() -> nil` |
| itemDisplayExists | `(id) -> b` |
| itemDisplayGetAllIds | `() -> [i]` |
| itemDisplaySetItem | `(id, itemId: s, aux: i) -> b` |
| itemDisplaySetPosition | `(id, x,y,z: f, dim: i) -> b`（dim<0 仅改坐标） |
| itemDisplaySetOffset | `(id, ox,oy,oz: s) -> b`（模型单位平移; Molang） |
| itemDisplaySetBaseOffset | `(id, ox,oy,oz: s) -> b`（渲染像素基础偏移; Molang） |
| itemDisplaySetRotation | `(id, rx,ry,rz: s) -> b`（三轴旋转/度; Molang） |
| itemDisplaySetScale | `(id, scale: s) -> b`（Molang） |
| itemDisplaySetExtend | `(id, scale,rx,ry: s) -> b`（方块模式二段变换） |
| itemDisplaySetMode | `(id, mode: i) -> b`（0=auto 1=item 2=block） |
| itemDisplaySetEnabled | `(id, enabled: b) -> b` |
| itemDisplaySetViewDistance | `(id, dist: f) -> b`（<=0 无限制） |
| itemDisplayRotateY | `(id, delta: f) -> b`（偏航叠加增量） |

渲染模式：`auto`（默认）按物品 3D/2D 自动选择；`item` 平面物品渲染（wiki.scale/wiki.posrot 路径，rotY 内部自动 +205 补偿狐狸头朝向）；`block` 3D 方块渲染（完整旋转矩阵路径 + 二段扩展变换）。可见性由库内 Level tick hook 自动同步（同维度 + 可见距离内玩家自动生成/移除，玩家加入/断线自动清理）。

---

## 3. 消费者版本协商示例

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x010500, "need HologramLib >= 1.5.0");

auto& lib = hologramlib::IHologramLib::getInstance();
if (lib.version() < 0x010500) { /* 运行时能力协商 */ }
```
