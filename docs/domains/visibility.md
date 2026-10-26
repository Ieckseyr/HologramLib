# 玩家出站包控制（IPlayerVisibility）

按"**被看的人**（主体）"屏蔽出站包。与 `IViewOverride` 的分工：

- `IViewOverride` 问的是"**看的人**（收件人）看到什么"
- 这边问的是"**被看的人**（主体）能不能被看到"

两者是同一套出站判定，但参数不同，所以没有塞进视图覆盖。

脚本侧见 [隐身与出站包控制](../lse/visibility.md)。

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

## 判定流程

钩子挂在 `NetworkSystem::send`（`ViewOutboundHook`），在视图覆盖判定**之前**执行：

1. 一条条目都没有 → 直接放行（快路径，零开销）
2. 认主体：按运行时 id 或 uuid 反查在线玩家
3. 主体没有条目 / 收件人就是主体本人（且没开 `alsoSelf`）→ 放行
4. 按包的分类查掩码，命中就丢

认主体的包：`AddPlayer` / `MovePlayer` / `MoveActorAbsolute` / `SetActorMotion` / `SetActorData` /
`Animate` / `ActorEvent` / `MobEffect` 按运行时 id；`PlayerList` / `PlayerSkin` 按 uuid。

## 两套机制不能混

| 机制 | 能不能让"已经站在别人屏幕上"的人消失 |
|---|---|
| `setMask` | **不能**，它只挡后续的包 |
| `setHidden` | **能**，走视图覆盖域的 hidden 通道（出生 + 更新 + 撤销） |

## restore 是怎么补发出生包的

不另写 `AddPlayer`（那要重建皮肤、属性、位置、姿态，容易漏），而是借视图覆盖域已经验证过的
"撤销即补发真身"流程：

```cpp
hologramlib::EntityView view;
view.hidden = true;
viewManager.overrideEntity("", uniqueId, view);   // 先挂一条隐藏
viewManager.clearEntity("", uniqueId);            // 撤销 → 库把真身重新发给所有观看者
```

好处是补发的出生包里皮肤、装备、姿态全是引擎的真实状态。
