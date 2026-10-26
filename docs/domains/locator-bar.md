# 方位条（ILocatorBar）

在客户端方位条上标出航点。库按玩家维护航点列表，`key` 由调用方命名，同一 `key` 再次设置即为更新位置。

协议：`LocatorBar`。

脚本用法见[方位条](../lse/locator-bar.md)。

## 接口

```cpp
ILocatorBar& locatorBar();

bool setWaypoint(
    std::string const& playerName,
    std::string const& key,        // 调用方命名，同一玩家下唯一
    float              x,
    float              y,
    float              z,
    int                dimension,
    int                color    = -1,    // ARGB；-1 表示客户端默认
    bool               visible  = true
);
bool removeWaypoint(std::string const& playerName, std::string const& key);
int  clearWaypoints(std::string const& playerName);
int  waypointCount(std::string const& playerName) const;

// 隐身的封装，实现在 IPlayerVisibility
bool hideFromOthers(std::string const& playerName, bool hidden);
bool isHiddenFromOthers(std::string const& playerName) const;
```

## 说明

- 航点标识为 UUID，由库按 `key` 稳定生成，调用方无需保存。
- 每个玩家维护一张 `key → {msb, lsb}` 表，玩家下线时清除。
- `visible=false` 仅隐藏显示，记录保留。
- `dimension` 原样写入数据包，显示位置由客户端决定。
- 航点的图标、颜色与显示效果取决于客户端版本与资源包。
