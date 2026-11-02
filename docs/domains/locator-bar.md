# 方位条（ILocatorBar）

在客户端方位条上标出航点。库按玩家维护航点列表，`key` 由调用方命名，同一 `key` 再次设置即为更新位置。

协议：`LocatorBar`。

> **当前限制：客户端不渲染服务端下发的航点。**
>
> 这条 HUD 只显示服务端"玩家方位"系统里注册过的玩家：正常进服的玩家会出现，本库的假玩家不会，
> 服务端凭空发的航点也不会。原生注册要经过 `ServerWaypoint` / `ServerWaypointGroup`，这两个类
> 没有导出构造函数，`Waypoint::mWorldPos` 的 `WorldPosition` 也禁止构造，插件侧走不通。
>
> 需要标记点、玩家方位这类功能，请用 Boss 条（`IBossBar`）或侧边栏（`ISidebar`）。

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
