# 方位条（ILocatorBar）

1.26 客户端快捷栏下方的方位指示。库按玩家维护航点，协议是 `LocatorBar`。

脚本侧见 [方位条](../lse/locator-bar.md)。

## 接口

```cpp
ILocatorBar& locatorBar();

bool setWaypoint(
    std::string const& playerName,
    std::string const& key,        // 自己起名，同一玩家下唯一
    float              x,
    float              y,
    float              z,
    int                dimension,
    int                color    = -1,    // ARGB；-1 = 客户端默认
    bool               visible  = true
);
bool removeWaypoint(std::string const& playerName, std::string const& key);
int  clearWaypoints(std::string const& playerName);
int  waypointCount(std::string const& playerName) const;

// 隐身的薄包装，实现已并入 IPlayerVisibility
bool hideFromOthers(std::string const& playerName, bool hidden);
bool isHiddenFromOthers(std::string const& playerName) const;
```

## 实现要点

- **航点身份是 UUID，库按 key 稳定派生**（FNV-1a 两个种子）—— 所以更新位置、删除航点
  都不需要调用方保管 UUID，同一个 key 再 `set` 就是更新。
- 每个玩家一张 `key → {msb, lsb}` 表，玩家下线时清掉。
- `visible=false` 发的是"更新可见性"，记录还在。
- `setWaypoint` 里的 `dimension` 原样进包，由客户端决定这个航点在哪个维度显示。

## 与隐身的关系

`hideFromOthers` / `isHiddenFromOthers` 是 [玩家出站包控制](visibility.md) 的薄包装 ——
同一套实现，只是从方位条这边也能调。完整的掩码 / 恢复能力在那个域里。
