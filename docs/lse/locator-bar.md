# 方位条（locatorBar*）

在客户端快捷栏下方的方位条上标出航点。库按玩家维护航点列表，`key` 由调用方命名，同一 `key` 再次设置即为更新位置，无需保存 UUID。

适用于追踪目标、队友位置、任务点引导。

```js
const { LocatorBar } = require("plugins/HologramLib/lib/HologramLib.js");

// 每 tick 刷新一次，让航点跟随目标
mc.listen("onTick", () => {
  LocatorBar.set("Steve", "boss", bossX, bossY, bossZ, 0, 0xFF0000, true);
});

LocatorBar.clear("Steve");
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `locatorBarSet(playerName, key, x, y, z, dim, color, visible)` | `Boolean` | 新增或更新航点；`color` 为 ARGB，`-1` 表示客户端默认色 |
| `locatorBarRemove(playerName, key)` | `Boolean` | 删除一个航点 |
| `locatorBarClear(playerName)` | `Integer` | 清空该玩家全部航点，返回清除数量 |
| `locatorBarCount(playerName)` | `Integer` | 当前航点数量 |
| `locatorBarHideFromOthers(playerName, hidden)` | `Boolean` | 将玩家从其他客户端隐藏，见[隐身](visibility.md) |
| `locatorBarIsHiddenFromOthers(playerName)` | `Boolean` | 查询隐藏状态 |

## 1.33.0 扩展：图标 / 大小 / 绑定实体 / 分组

```js
// 自定义图标贴图（资源包里的路径；空字符串回到默认图标）
LocatorBar.setIcon("Steve", "boss", "textures/ui/icon_boss");

// 图标显示大小（像素；<=0 回到默认）
LocatorBar.setIconSize("Steve", "boss", 24, 24);

// 绑定实体：航点跟随该实体，不用每 tick 改坐标
LocatorBar.attachEntity("Steve", "boss", entityUniqueId);
LocatorBar.clearAttach("Steve", "boss");

// 分组：同一 groupKey 的航点合进一条包发送
LocatorBar.setGroup("Steve", "boss", "raid");
LocatorBar.setGroup("Steve", "healer", "raid");
const n = LocatorBar.syncGroup("Steve", "raid");   // 一条包发出该组全部航点
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `locatorBarSetIcon(playerName, key, texturePath)` | `Boolean` | 自定义图标；空串=默认 |
| `locatorBarSetIconSize(playerName, key, width, height)` | `Boolean` | 图标像素尺寸；`<=0`=默认 |
| `locatorBarAttachEntity(playerName, key, entityUniqueId)` | `Boolean` | 绑定实体（按 uniqueId） |
| `locatorBarClearAttach(playerName, key)` | `Boolean` | 解除绑定 |
| `locatorBarSetGroup(playerName, key, groupKey)` | `Boolean` | 归入分组 |
| `locatorBarSyncGroup(playerName, groupKey)` | `Integer` | 把该组全部航点合成一条包发出 |

协议里没有独立的"分组"字段（一条 `LocatorBarPacket` 的 `mWaypoints` 是数组），所以分组落在**合并发送**这一层；
绑定实体也没有专属的标志位，实体 id 随位置一起带上。

## 当前限制：客户端不渲染服务端下发的航点

接口本身能发包（服务端侧的增减、图标、尺寸、绑定、分组都记着，`encodeChecked` 回读校验也过），
但**客户端不显示**这些航点。原因是这条 HUD 由客户端的"玩家方位"系统驱动：

- 正常进服的玩家会被服务端注册进 `ServerWaypointGroup`，客户端才有东西可画；
- 本库的假玩家（协议层实体）同样不会出现在方位条上；
- 服务端注册航点要经过 `ServerWaypoint` / `ServerWaypointGroup`，而这两个类的构造函数没有导出，
  `Waypoint::mWorldPos` 的 `WorldPosition` 也禁止构造，插件侧没法走原生路径。

所以这一组接口目前只在两方面有用：把所有字段一次发全（`locatorBarSetEx`）、以及做协议层实验。
**做玩家方位、标记点这类功能，请改用 Boss 条（锚点方案）或侧边栏。**

## 说明

- `key` 由调用方命名，同一玩家下唯一，不同玩家之间互不影响。
- `visible=false` 仅隐藏显示，记录保留，重新设置为 `true` 即恢复。
- 玩家下线时库清除其航点记录，客户端随下线同步消失。
- 航点的显示效果取决于客户端版本与资源包。
