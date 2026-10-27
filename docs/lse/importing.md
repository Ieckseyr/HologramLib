# 脚本导入

库里共有 339 个函数，脚本侧有两种导入方式。两种都用 `lrca`（LegacyRemoteCall）提供的全局 `ll`。

## 方式一：统一导入

`plugins/HologramLib/lib/HologramLib.js` 随库一起发布，直接引用即可。

```js
const { ItemActor, Visibility, LocatorBar } = require("plugins/HologramLib/lib/HologramLib.js");

ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
ItemActor.setMotion(id, -0.3, 0.2, 0);
Visibility.setHidden("Steve", true);
LocatorBar.set("Steve", "boss", x, y, z, 0, 0xFF0000, true);
```

也可以按函数原名调用：

```js
const HL = require("plugins/HologramLib/lib/HologramLib.js");

HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
HL.playerHideSetHidden("Steve", true);
```

函数在第一次调用时才与库绑定，之后复用。库未加载、`lrca` 未加载或函数名不存在时，调用会被忽略并记录一条警告，脚本不会因此中断。

| 属性 | 说明 |
|---|---|
| `HL.apiVersion` | 库的 API 版本，如 `"1.32.0"` |
| `HL.abiStamp` | 虚表布局戳（C++ 侧兼容判断用） |
| `HL.list()` | 全部函数名 |
| `HL.has("函数名")` | 判断该函数是否存在 |
| `HL.groups` | 所有分组对象 |

### 分组

函数按前缀分组，方法名为函数名去掉前缀（`itemActorSpawn` → `ItemActor.spawn`）。

| 分组 | 函数数 | 前缀 |
|---|---|---|
| `Shape` | 39 | `shape*` |
| `Hologram` | 28 | `holo*` |
| `GradientLine` | 11 | `gradient*` |
| `ItemDetail` | 2 | `itemDetail*` |
| `ItemDisplay` | 44 | `itemDisplay*` |
| `Entity` | 47 | `entity*` |
| `Ghost` | 2 | `ghost*` |
| `Particle` | 22 | `particle*` |
| `PlayerNpc` | 52 | `playerNpc*` |
| `Trade` | 10 | `trade*` |
| `Container` | 11 | `container*` |
| `NpcDialog` | 9 | `npcDialog*` |
| `Sensing` | 3 | `sensing*` |
| `FakeInventory` | 9 | `fakeInv*` |
| `Sulfur` | 10 | `sulfur*` |
| `View` | 19 | `view*` |
| `ItemActor` | 8 | `itemActor*` |
| `LocatorBar` | 6 | `locatorBar*` |
| `Form` | 2 | `form*` |
| `Visibility` | 5 | `playerHide*` |

`lib/` 下的 `HologramLib.d.ts` 与 `.js` 放在一起时，编辑器可获得参数名、类型与返回值补全。

## 方式二：`ll.import` 直连

```js
if (typeof ll === "undefined" || typeof ll.import !== "function") {
  logger.warn("缺少 lrca，HologramLib 调用将被跳过");
  return;
}

const itemActorSpawn = ll.import("HologramLib", "itemActorSpawn");
if (typeof itemActorSpawn !== "function") {
  logger.warn("当前库版本没有 itemActorSpawn");
  return;
}

itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
```

注意事项：

- 在脚本加载时绑定一次并保存到变量，不要在 `onTick`、`setInterval` 或事件回调里反复调用 `ll.import`。
- 返回值不是函数时说明库未加载、`lrca` 未加载或函数名写错，需逐个判断。
- 函数名区分大小写，须与导出表完全一致。

需要绑定多个函数时，可以用一张表统一处理：

```js
const NS = "HologramLib";
const HL = {};

function bindAll(names) {
  if (typeof ll === "undefined" || typeof ll.import !== "function") return false;
  let any = false;
  for (const name of names) {
    const fn = ll.import(NS, name);
    if (typeof fn === "function") {
      HL[name] = fn;
      any = true;
    }
  }
  return any;
}

bindAll(["itemActorSpawn", "itemActorSetMotion", "itemActorTake"]);
```

## 选择建议

| 场景 | 建议 |
|---|---|
| 新脚本，使用多个函数 | 统一导入 |
| 只使用一两个函数 | `ll.import` 直连 |
| 需要固定库版本 | 统一导入，并把 `lib/HologramLib.js` 复制到自己的插件目录 |
| 已有自己的绑定表 | 保留即可，也可与新脚本的写法混用 |
| 需要判断某函数是否存在 | `HL.has("函数名")` |

## 函数名查询

- 各域页面：[LSE 脚本接口](index.md)
- 运行时：`HL.list()`
- `exports.json`（与 `HologramLib.js` 同目录）：

```json
{ "name": "itemActorSpawn", "group": "ItemActor", "method": "spawn",
  "params": [["itemId", "string"], ["aux", "number"]], "returns": "number" }
```

## 发布包内容

```
plugins/HologramLib/
  HologramLib.dll
  HologramLib.pdb
  manifest.json
  tooth.json
  lib/
    HologramLib.js
    HologramLib.d.ts
    exports.json
```

## 常见问题

- `require` 的路径相对于当前脚本文件。引用库目录用 `plugins/HologramLib/lib/HologramLib.js`，把文件复制到自己目录则用 `./HologramLib.js`。不要写 `require("HologramLib")`。
- `ll` 由 `lrca` 提供。未安装 `lrca` 时 `ll` 可能不存在，直连写法需先判断。
- 库在启用时探测 `lrca`，未探测到会在服务器启动完成后再试一次。脚本如果在模块顶层立即调用，建议把初始化放在 `mc.listen("onServerStarted", …)` 内。
- 更新库后，直接引用库目录的写法无需修改；复制到自身目录的文件需要重新复制。
