# 1.6 ICustomEntity · 自定义实体

完整签名（`#include "HologramLib/HologramLib.h"`）：

AddActorPacket 直发客户端生成**纯视觉实体**，不占服务端实体系统。适合 NPC 壳 / 装饰生物 / 盔甲架布景等（无碰撞、无服务端逻辑；交互经 ghost 事件路由，见 1.7）。

```cpp
struct CustomEntityEquipment {
    std::string name;    // item name（空 = 空槽位）
    int         aux{0};
    std::string nbt{};   // SNBT 形式, 空 = 无附加 NBT
};

struct CustomEntityConfig {
    std::string identifier{"minecraft:armor_stand"}; // 实体类型标识符（短名自动补 minecraft:）
    float       x{0}, y{64}, z{0};                   // 世界坐标
    int         dimension{0};
    float       yaw{0}, pitch{0};                    // 朝向（度）
    std::string nametag{};                           // 头顶名字（支持 § 颜色码; 空 = 无）
    bool        nametagAlwaysShow{false};
    float       scale{1.0f};                         // 客户端有效域 0.0625~10, 自动钳制
    int         variant{0};                          // 变种（皮肤/亚种）
    int         markVariant{0};                      // 二级变种
    int         colorIndex{0};                       // 颜色索引（羊/项圈等染色实体）
    std::int64_t flags{0};                           // 原始 flags 位掩码（0x01=着火 0x20=隐身 等）
    bool        invisible{false};                    // 隐身便捷开关（发包时合成 0x20）
    double      viewDistance{64.0};                  // <=0 无限制
    bool        enabled{true};
    int         pose{0};                             // PoseIndex 0..13（0=Standing 3..13 坐姿/睡姿/跳舞等）
    CustomEntityEquipment equipment[6];              // 槽位: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet
    std::string ridePlayerName{};                    // 骑到指定玩家头上（SetActorLinkPacket; 须在线）
    int64_t     rideEntityId{0};                     // 骑到另一自定义实体上（对方库内 id 为载具）
};
```

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create / createRandom / createWithId | `(CustomEntityConfig const&[, int64_t desiredId]) -> int64_t` | 同 itemDisplay 语义（随机段 `[0x10000000, 0x7FFFFFFF)`; desiredId<=0 或占用返回 -2） |
| | destroy / destroyAll / exists / isIdUsed / getAllIds / get | — | 同 itemDisplay 语义 |
| 属性 | setIdentifier | `(int64_t, std::string const&) -> bool` | 换实体类型 |
| | setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 仅改坐标 |
| | setRotation | `(int64_t, float yaw, float pitch) -> bool` | 朝向（度） |
| | setNametag | `(int64_t, std::string const&) -> bool` | 空串清除 |
| | setScale | `(int64_t, float) -> bool` | <=0 拒绝 |
| | setVariant / setMarkVariant / setColorIndex | `(int64_t, int) -> bool` | 变种/染色 |
| | setFlags / setInvisible / setEnabled / setViewDistance | — | 行为 |
| | setPose | `(int64_t, int pose) -> bool` | PoseIndex 0..13 |
| | setEquipmentSlot | `(int64_t, int slot, std::string const& name, int aux, std::string const& nbt) -> bool` | 六槽装备; name 空清空槽位; nbt 为 SNBT |
| | findNearest | `(float x, float y, float z, int dim, double maxDist) -> int64_t` | 最近查找; 无匹配 -1 |
| 缩放 | scaleBy | `(int64_t, double factor) -> bool` | 相对缩放：现有 scale × factor，结果自动钳制 0.0625~10; factor<=0 返回 false |
| 可见性 | setVisiblePlayers | `(int64_t, std::vector<std::string> const&) -> bool` | 白名单（按 realName 匹配）; 空列表 = 清除限制 |
| | setVisiblePlayer / clearVisiblePlayers | `(int64_t, std::string const&) / (int64_t) -> bool` | 标量版 / 清除（恢复全员可见） |
| 逐客户端朝向（1.20.0; LSE: `entitySetPlayerRotation`，26.51.6 起） | setPlayerRotation | `(int64_t, std::string const& playerName, float yaw, float pitch) -> bool` | **1.20.0**: 覆盖指定玩家收到的该实体朝向（出生包 AddActor 与增量包都按覆盖值下发）; 未覆盖的玩家仍用 config 朝向; 玩家离线/未见过该实体返回 false; 变更走轻脏增量（无闪烁）, 下一 tick 生效 |
| | clearPlayerRotation | `(int64_t, std::string const& playerName) -> bool` | 清除单个玩家的朝向覆盖 |
| | clearPlayerRotations | `(int64_t) -> bool` | 清除该实体全部玩家的朝向覆盖 |
| 诊断 | getDebugInfo | `(int64_t) -> std::string` | 运行态摘要; 未找到返回 `not_found` |
| 骑乘 | setRidePlayer | `(int64_t, std::string const& playerName) -> bool` | 骑到玩家头上（空名清除; 须在线） |
| | setRideEntity | `(int64_t, int64_t vehicleEntityId) -> bool` | 骑到另一自定义实体上; 0 清除 |
| | clearRide | `(int64_t) -> bool` | 解除骑乘链接 |
| 动画 | playAnimation | `(int64_t, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 播放原版动画（AnimateEntityPacket, 如 `animation.humanoid.base_pose`）; stopExpression 空串 = 常驻; durationTicks>0 到期自动停止 |

属性变更经 tick 脏刷新合并为单次 respawn（无闪烁串台）；可见性由库内 Level tick hook 自动同步（同 itemDisplay 模式）。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
