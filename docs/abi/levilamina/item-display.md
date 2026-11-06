# 1.5 IItemDisplay · 物品悬浮展示

完整签名（`#include "HologramLib/HologramLib.h"`）：

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
    std::string itemNbt{};                 // 物品附加数据（SNBT; 自定义名称等）
    bool   itemGlint{false};               // 附魔光效（BDS 原生路径注入 1 级锋利）
    float  hitboxWidth{0.0f};              // AABB 判定体积宽（R53; 0 = 无判定体积, 1.17.0）
    float  hitboxHeight{0.0f};             // AABB 判定体积高（R54; 0 = 无判定体积, 1.17.0）
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
| createRandom | `(ItemDisplayConfig const&) -> int64_t` | 随机段 `[0x10000000, 0x7FFFFFFF)` 自动生成不重复 ID; 成功返回生成的 ID, <0 失败 |
| createWithId | `(ItemDisplayConfig const&, int64_t desiredId) -> int64_t` | 指定 ID 创建（持久化恢复用）; desiredId<=0 或已被占用返回 -2 |
| isIdUsed | `(int64_t) -> bool` | 查询 ID 是否在用 |
| scaleBy | `(int64_t, double factor) -> bool` | 相对缩放：现有 scale × factor; 常量直接相乘, 表达式包裹 `(expr)*factor`; factor<=0 返回 false |
| setItemWithNbt | `(int64_t, std::string const& item, int aux, std::string const& nbt) -> bool` | 换物品（带附加数据）：nbt 为 SNBT; 空串 = 清除; 解析失败按无 NBT 处理并告警 |
| setGlint | `(int64_t, bool on) -> bool` | 附魔光效开关：开 = BDS 原生 `saveEnchantsToUserData` 注入 1 级锋利; 幂等 |
| follow | `(int64_t, std::string const& playerName, float offX, float offY, float offZ) -> bool` | 1.17.0 展示跟随玩家：每 tick 同步目标实时坐标（含跨维度自动 respawn），对已见玩家发 MoveActorAbsolute（非 teleport，客户端插值）平滑位移，全程无 respawn；玩家下线自动解除（方块原地保留）；`setPosition` 手动设位解除跟随 |
| unfollow | `(int64_t) -> bool` | 1.17.0 解除跟随；无跟随关系返回 true |
| setHitbox | `(int64_t, float width, float height) -> bool` | 1.17.0 AABB 判定体积：对已见玩家广播 SetActorData(R53/R54)，即时无 respawn；数值持久入配置（respawn 自动按当前值发包）；0/0 恢复不可命中 |

可见性由库内 Level tick hook 自动同步（每 20 tick：同维度 + 可见距离内玩家自动生成/移除；玩家断线自动清理）；属性变更即时生效（对已见玩家原子 despawn→respawn）。`setMode` / `setViewDistance` 幂等（同值不标脏不 respawn）。

无感创建（`createSeamless`）、单次跳变缩放（`scaleTo`）与可见白名单目前仅 LSE 导出（§2.5），未上 C++ 接口。渐变动画由消费者基于 `scaleTo` 步进驱动（如 5 步 easeOutCubic）。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
