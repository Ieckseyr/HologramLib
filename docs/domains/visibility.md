# 隐身（IPlayerVisibility）

控制其他玩家的客户端是否接收某个玩家的数据包。与 `IViewOverride` 的区别：`IViewOverride` 按收件人配置显示内容，本接口按被隐藏的玩家配置发送内容。

脚本用法见[隐身](../lse/visibility.md)。

## 接口

```cpp
struct PlayerPacketMask {
    bool spawn{false};  // AddPlayer / PlayerList(Add)
    bool tab{false};    // PlayerList
    bool motion{false}; // MovePlayer / MoveActorAbsolute / MoveActorDelta / SetActorMotion
    bool state{false};  // SetActorData / UpdateAttributes / MobEffect / Animate / ActorEvent
    bool skin{false};   // PlayerSkin
};

IPlayerVisibility& playerVisibility();

bool setMask(std::string const& playerName, PlayerPacketMask const& mask, bool alsoSelf = false);
bool setHidden(std::string const& playerName, bool hidden);
bool restore(std::string const& playerName);
PlayerPacketMask maskOf(std::string const& playerName) const;
bool isHidden(std::string const& playerName) const;
```

## 处理流程

钩子位于 `NetworkSystem::send`，在视图覆盖判定之前执行。

1. 没有任何记录时直接放行
2. 识别数据包所属玩家：按运行时 id 或 uuid 反查在线玩家
3. 目标玩家无记录，或收件人即目标玩家（且未开启 `alsoSelf`）时放行
4. 按数据包类型查询掩码，命中则丢弃

识别的数据包：`AddPlayer`、`MovePlayer`、`MoveActorAbsolute`、`SetActorMotion`、`SetActorData`、`Animate`、`ActorEvent`、`MobEffect` 按运行时 id；`PlayerList`、`PlayerSkin` 按 uuid。

## 两种方式

| 方式 | 对已出生的玩家 |
|---|---|
| `setMask` | 不消失，仅停止更新 |
| `setHidden` | 立即从其他客户端消失 |

## restore 的行为

清除掩码与隐身记录后，通过视图覆盖的撤销流程重新发送该玩家的出生包，因此重新发送的数据包含当前皮肤、装备与姿态。

## 说明

- 无法识别类型的数据包不会被拦截。
- 一条 `PlayerList` 中包含多个玩家时，整条数据包放行。
- 玩家下线时清除记录，客户端随下线同步消失。
- `alsoSelf=true` 时该玩家自己也收不到这些数据包。
