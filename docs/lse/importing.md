# 脚本导入（lrca 直连 / 统一导入）

两种方式都能用。**推荐统一导入**：少写、少踩坑、升级不用改。

- 库里现在有 **339 个导出**
- 脚本侧唯一的前置是 `lrca`（LegacyRemoteCall）—— 它提供全局 `ll`
- 两种方式可以混用：主逻辑用统一导入，个别函数用 `ll.import` 直接取

---

## 方式一：统一导入（推荐）

发布包里 `plugins/HologramLib/HologramLib.js` 已经和 DLL 放在一起，直接引：

```js
const { ItemActor, Visibility, LocatorBar } = require("plugins/HologramLib/HologramLib.js");

ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
ItemActor.setMotion(id, -0.3, 0.2, 0);
Visibility.setHidden("Steve", true);
LocatorBar.set("Steve", "boss", x, y, z, 0, 0xFF0000, true);
```

也可以按导出名平铺（跟各页的函数表一一对应）：

```js
const HL = require("plugins/HologramLib/HologramLib.js");

HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
HL.playerHideSetHidden("Steve", true);
```

平铺写法可以先解构，之后完全不经过代理：`const { formClose, locatorBarSet } = HL;`

### 它替你做了什么

| 行为 | 说明 |
|---|---|
| **懒绑定** | 第一次取某个函数时才 `ll.import`，之后走缓存。用 8 个函数就绑 8 个，不是 339 个 |
| **版本核对** | 第一次绑定时读一次 `holoInfo()`（一次调用），版本或 ABI 戳对不上就打一条日志 |
| **缺 lrca / 缺函数 → 空操作** | 拿到的是 `function noop(){}`，脚本照常加载，只记一条 `logger.warn` |
| **拼错名字 → `undefined`** | 取不存在的属性得到 `undefined` 并警告一次，写错能看见 |
| **元信息** | `HL.apiVersion`、`HL.abiStamp`、`HL.list()`、`HL.has("名字")`、`HL.bind("名字")` |

### 分组

方法名 = 导出名去掉前缀（`itemActorSpawn` → `ItemActor.spawn`）。分组按源码前缀自动生成，
不会和实现漂移；分组对象是冻结的，方法同样是懒绑定。

| 分组 | 方法数 | 前缀 | 例子 |
|---|---|---|---|
| `Shape` | 39 | `shape*` | `Shape.create(...)` |
| `Hologram` | 28 | `holo*` | `Hologram.setLineText(id, 0, "…")` |
| `GradientLine` | 11 | `gradient*` | `GradientLine.create(...)` |
| `ItemDetail` | 2 | `itemDetail*` | `ItemDetail.show(...)` |
| `ItemDisplay` | 44 | `itemDisplay*` | `ItemDisplay.create(...)` |
| `Entity` | 47 | `entity*` | `Entity.create(...)` |
| `Ghost` | 2 | `ghost*` | `Ghost.poll(...)` |
| `Particle` | 22 | `particle*` | `Particle.create(...)` |
| `PlayerNpc` | 52 | `playerNpc*` | `PlayerNpc.spawn(...)` |
| `Trade` | 10 | `trade*` | `Trade.open(...)` |
| `Container` | 11 | `container*` | `Container.open(...)` |
| `NpcDialog` | 9 | `npcDialog*` | `NpcDialog.open(...)` |
| `Sensing` | 3 | `sensing*` | `Sensing.inputMode(...)` |
| `FakeInventory` | 9 | `fakeInv*` | `FakeInventory.apply(...)` |
| `Sulfur` | 10 | `sulfur*` | `Sulfur.create(...)` |
| `View` | 19 | `view*` | `View.entity(...)` |
| `ItemActor` | 8 | `itemActor*` | `ItemActor.spawn(...)` |
| `LocatorBar` | 6 | `locatorBar*` | `LocatorBar.set(...)` |
| `Form` | 2 | `form*` | `Form.close(...)` |
| `Visibility` | 5 | `playerHide*` | `Visibility.setHidden(...)` |

`HL.groups` 里有同一批对象（`HL.groups.ItemActor.spawn`）。

### 想锁版本就拷一份

把 `HologramLib.js` 拷到脚本目录旁边，改 `require("./HologramLib.js")`。
代价是升级库之后要重新拷；`HologramLib.d.ts` 放同一目录，编辑器就能补全参数名与类型。

---

## 方式二：lrca 直连（`ll.import`）

只用一两个函数、或者不想依赖那个 `.js` 文件时，直接走 lrca：

```js
// `ll` 是全局，由 lrca 提供。先探测，避免没装 lrca 时整个脚本加载失败。
if (typeof ll === "undefined" || typeof ll.import !== "function") {
  logger.warn("缺 lrca，HologramLib 的调用会被跳过");
  return;
}

const itemActorSpawn = ll.import("HologramLib", "itemActorSpawn");
if (typeof itemActorSpawn !== "function") {
  logger.warn("这版库没有 itemActorSpawn（版本不匹配？）");
  return;
}

itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
```

要点：

- **在模块加载时绑定一次，存到变量里**。不要在 `onTick` / `setInterval` / 事件回调里反复
  `ll.import` —— 那等于每次调用都走一遍完整绑定。
- `ll.import` 返回的不是函数，说明库没加载、lrca 没加载，或者名字写错。**逐个判断**再保存。
- 名字必须与导出表一字不差（大小写敏感）。查法见下面「函数名从哪查」。

### 建议的封装写法

要在一个脚本里用十几个函数时，用一张表批量绑：

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

// 必需的有哪些、可选的有哪些，分开写
bindAll(["itemActorSpawn", "itemActorSetMotion", "itemActorTake"]);
```

这其实就是统一导入在做的事 —— 只是它把清单和降级都替你写好了。

---

## 两种方式怎么选

| 场景 | 用哪个 |
|---|---|
| 新写脚本、要用多个域的多个函数 | **统一导入**（类.方法） |
| 只用 1~3 个函数，且不想多带一个文件 | lrca 直连 |
| 想连库版本一起锁死 | 统一导入 + 把 `.js` 拷进自己插件目录 |
| 老脚本里已经有一套自己的 `HL` 绑定表 | 保留，不用改；新代码可以混用统一导入 |
| 需要判断"这版库有没有某个函数" | `HL.has("名字")`；直连时判断返回值 |

---

## 函数名从哪查

1. 各域页面 —— [LSE 脚本接口](index.md) 的「7. 按域查函数」
2. `HL.list()` —— 运行时拿到的完整清单
3. `exports.json`（和 `HologramLib.js` 放一起）—— 每条形如：

```json
{ "name": "itemActorSpawn", "group": "ItemActor", "method": "spawn",
  "params": [["itemId", "string"], ["aux", "number"]], "returns": "number" }
```

---

## 发布包里有什么

```
plugins/HologramLib/
  HologramLib.dll
  HologramLib.pdb
  manifest.json
  HologramLib.js      ← 统一导入 require 这个
  HologramLib.d.ts    ← 编辑器补全（放同一目录即可）
  exports.json        ← 机器可读清单
```

构建时 `xmake` 的 `after_build` 会把这三个文件拷进 `bin/HologramLib/`，打包脚本再一起塞进
`bin/HologramLib.zip`，所以发布包里一定有它们。

---

## 常见坑

- **`require` 的路径是相对当前脚本文件**的。`plugins/HologramLib/HologramLib.js` 在 LSE 里可用；
  拷到自己目录则是 `./HologramLib.js`。不要写 `require("HologramLib")` —— 它没装成 node_modules 包。
- **`ll` 由 lrca 提供**，不是库提供的。没有 lrca 时 `ll` 可能整个不存在，
  所以直连写法一定要先 `typeof ll.import === "function"`。
- **加载顺序**：库在 `enable()` 里探测 lrca，没探测到会在服务器启动完成后再试一次。
  但脚本如果太早跑（模块顶层就调用），函数可能还没注册 —— 这类初始化放进
  `mc.listen("onServerStarted", …)` 更稳。
- **升级库之后**：统一导入（直接引库目录那份）不用改；拷出来的那份要重新拷。
  直连写法不用改，除非函数名变了（不会）。
