# 1.9 IPlayerNpc · 假玩家 NPC

完整签名（`#include "HologramLib/HologramLib.h"`）：

> **NPC 皮肤已可正常渲染（26.40.2 修复）**。本节全部接口可用且数据链路完整：皮肤注册（PNG / 在线采集 / 目录导入）、`getSkinBlob` / `registerSkinFromBlob` 导出恢复、NPC 创建/移动/朝向/缩放/视距/显隐、点击交互均正常。此前的症状（客户端不渲染皮肤、外观回退默认模型）根因在 PlayerList 皮肤条目的 `Id` / `FullId` 为空或残留原玩家缓存键，已修复。

纯协议假玩家：`PlayerListPacket(Add, 携带皮肤) → AddPlayerPacket → [20 tick] PlayerListPacket(Remove)`（假玩家短暂出现在 Tab 后移除，实体因皮肤已缓存持续渲染）。不占服务端实体系统；点击交互经 ghost 管线 `domain="npc"` 派发（§1.7）。

皮肤注册表全局共享：PNG 文件注册（GDI+ 解码 64×64/128×128，自定义 geometry/armSize）、目录批量导入（一个子文件夹 = PNG + 可选 `.json` 几何模型，缺省 = 标准玩家模型）或从在线玩家采集（`Player::mSkin → SerializedSkinImpl` 全字段拷贝：贴图/披风/动画贴图/几何/Persona 部件/染色 → 以 skinId 注册运行时快照，玩家之后换肤不影响）。解码/采集一次，多 NPC 复用零重复开销。**库不落盘**——持久化由消费方负责：注册/采集/导入成功后 `getSkinBlob` 导出全字段二进制快照存到自己的目录，重启时 `registerSkinFromBlob` 恢复，不依赖源 PNG/玩家在线。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 皮肤 | registerSkin | `(PlayerNpcSkin const& skin) -> bool` | PNG 注册（skinId 空 = 文件名; 重复覆盖） |
| | importSkins | `(std::string const& dirPath) -> int` | 目录批量导入（skinId = 子文件夹名; PNG 必需 + `.json` 几何可选; 返回导入数量, 目录无效 -1） |
| | captureSkin | `(std::string const& skinId, std::string const& playerName) -> bool` | 从在线玩家采集（不在线返回 false; 重复覆盖） |
| | getSkinBlob | `(std::string const& skinId, std::string& out) const -> bool` | 全字段序列化导出（消费方持久化用; 未注册 false） |
| | registerSkinFromBlob | `(std::string const& blob) -> bool` | blob 反序列化注册（与 getSkinBlob 配对; 格式非法 false） |
| | hasSkin / unregisterSkin / getSkinIds | — | unregister 有 NPC 引用时拒绝 |
| 生命周期 | create / createRandom / createWithId | `(PlayerNpcConfig const&) -> int64_t` | 失败: -1 常规 / -2 id 占用 / -3 皮肤未注册; 持久化由消费者负责 |
| | destroy / destroyAll / exists / get / isIdUsed / getAllIds | — | 同其他域 |
| 属性 | setPosition | `(int64_t id, float x, float y, float z, int dim) -> bool` | dim<0 仅改坐标 |
| | setRotation / setNametag / setSkin / setViewDistance / setEnabled | — | setSkin 未注册返回 false; 变更经 tick 脏刷新合并为单次 respawn |
| 朝向（1.20.0; LSE: `playerNpcSetRotLight`，26.51.6 起） | setRotationLight | `(int64_t id, float yaw) -> bool` | 轻量朝向: 只发 MoveActorAbsolute 增量（不重建实体/不重发皮肤, 无闪烁）, 适合每 tick 跟踪 |
| | setPlayerRotation | `(int64_t id, std::string const& playerName, float yaw) -> bool` | 覆盖指定玩家收到的朝向（出生包与增量包都按覆盖值）; 未覆盖玩家用 config 朝向 |
| | clearPlayerRotation / clearPlayerRotations | `(int64_t[, std::string const&]) -> bool` | 清除单个 / 全部玩家的朝向覆盖 |
| 可见性 | setVisiblePlayers / clearVisiblePlayers / setVisiblePlayer | — | 玩家名白名单（空 = 全员） |
| 轻量位置（26.40.8） | setPositionLight | `(int64_t id, float x, float y, float z, int dim) -> bool` | **只发 `MoveActorAbsolute` 增量**（不重建实体/不重发皮肤, 无闪烁）; 与 setRotationLight 同一条轻脏通道, 同一 tick 合并一条包; `dim` 与当前维度不同返回 false（跨维度仍走 setPosition 重建） |
| 皮肤注入（26.40.8; LSE: `playerNpcInjectSkin` / `playerNpcInjectSkinAll`） | injectSkin | `(std::string const& viewerName, std::string const& targetName, std::string const& skinId) -> bool` | 用 **target 自己的 UUID / uniqueId** 发 `PlayerList(Add)` 就地更新皮肤条目（真实 xuid + trust 三态 + 皮肤对象 `OverridesPlayerAppearance=true`——覆盖客户端已装备皮肤的前提）; `viewerName` 空串 = 所有在线玩家（**含 target 本人**）; 条目按注册表原样下发; **不自动重发**（丢帧由消费方再调一次） |
| | injectSkinAll | `(std::string const& targetName, std::string const& skinId) -> bool` | 全员版（等价 `injectSkin("", targetName, skinId)`） |
| 动画（26.40.8; LSE: `playerNpcPlayAnimation` / `playerNpcPlayAnimationTo`） | playAnimation | `(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 对"已见过该 NPC 的玩家"发 `AnimateEntityPacket`（controller 名按 NPC id 内自动唯一化, 资源包无需同名控制器）; `stopExpression` 空 = 常驻, `query.any_animation` = 立刻停, `durationTicks>0` 到期自动补停止包; 动画名需客户端可解析（原版或随资源包下发）; 无观察者返回 false |
| | playAnimationTo | `(int64_t id, std::string const& playerName, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | 同上, 只发给该玩家（补发用; 未见过该 NPC 返回 false） |
| 出生回调（26.40.8; 仅 C++） | setEntitySpawnCallback | `(std::function<void(int64_t id, std::string const& playerName)>) -> void` | NPC 对某玩家**出生完成**回调（供补发动画/状态; EntitySpawnCallback 语义）; 与 customentity 域同款队列（+2 tick 发出） |
| 诊断 | getDebugInfo | `(int64_t id) const -> std::string` | 运行态摘要 |

```cpp
struct PlayerNpcSkin {
    std::string pngPath;                                        // PNG 路径（64/128）
    std::string skinId;                                         // 空 = 用文件名
    std::string geometry{"geometry.humanoid.custom"};          // 模型（resourcePatch）
    std::string armSize{"wide"};                                // "wide" / "slim"
    std::string geometryData;                                   // 1.18.0 完整几何 JSON（可选, 提供时启用自定义模型）
};

struct PlayerNpcConfig {
    std::string name{"NPC"};        // 显示名（nametag 同步）
    std::string skinId{"default"};   // 必须已注册, 否则创建失败 -3
    float x{0}, y{64}, z{0};
    int   dimension{0};
    float yaw{0};
    double viewDistance{96.0};      // <=0 无限制; 滞回: 退出需超 viewDistance+4
    bool  enabled{true};
};
```

---

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
