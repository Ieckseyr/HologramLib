# 脚本里怎么导入（少写一半，快一截）

库里现在有 339 个导出。如果每个脚本一开始就循环 `ll.import` 一遍，那么：

- 加载期要付**几百次跨语言绑定**（用不到的也付了）
- 每个脚本都要复制一份 `lrca` 探测 + 名字清单，改一个函数名要改所有脚本
- 最容易踩的坑：把 `ll.import(...)` 写在 `setInterval` / `onTick` 里 —— **每次调用都走一遍完整绑定**

库自带一个生成好的导入层，**随发布包一起发**，把这三件事一次解决。

## 发布包里就带着

解压 `HologramLib.zip` 之后，`plugins/HologramLib/`（和 DLL 平级） 里是三个文件：

| 文件 | 用途 |
|---|---|
| `HologramLib.js` | 脚本 `require` 的模块 |
| `HologramLib.d.ts` | 编辑器补全（函数名 / 参数名 / 类型 / 返回） |
| `exports.json` | 机器可读清单，给工具链用 |

不用自己生成，也不用从仓库里拷。

## 两种引法

```js
// A. 直接引库目录里那份 —— 升级库时自动跟着换
const HL = require("plugins/HologramLib/HologramLib.js");
```

```js
// B. 拷到脚本目录旁边 —— 想锁版本时用这个
const HL = require("./HologramLib.js");
```

## 两种写法

拿到模块之后，可以按**类**用（推荐），也可以按**导出名**平铺用 —— 两种都行，同一套实现：

```js
// 写法一：类.方法（同 GMLIB-LegacyRemoteCallApi 的风格）
const { ItemActor, Visibility, LocatorBar } = require("plugins/HologramLib/HologramLib.js");

ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
ItemActor.setMotion(id, -0.3, 0.2, 0);
Visibility.setHidden("Steve", true);
LocatorBar.set("Steve", "boss", x, y, z, 0, 0xFF0000, true);
```

```js
// 写法二：平铺（名字就是导出名，跟函数表一一对应）
const HL = require("plugins/HologramLib/HologramLib.js");

HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
HL.playerHideSetHidden("Steve", true);
```

平铺写法也可以先解构一次，之后完全不经过 Proxy：`const { formClose, locatorBarSet } = HL;`

## 分组表

方法名 = 导出名去掉前缀（`itemActorSpawn` → `ItemActor.spawn`）：

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

`HL.groups` 里有同一份对象（`HL.groups.ItemActor.spawn`），分组对象是冻结的。

对比一下原来必须写的：

```js
// 以前：每个脚本都要这几行，还要自己维护名字清单
if (typeof ll === "undefined" || typeof ll.import !== "function") return;
const itemActorSpawn = ll.import("HologramLib", "itemActorSpawn");
const formClose      = ll.import("HologramLib", "formClose");
// …用几个写几个
```

## 它做了什么

| 行为 | 说明 |
|---|---|
| **懒绑定** | `HL.xxx` 第一次被取用时才 `ll.import`，之后走缓存。用 8 个函数就绑 8 个，不是 339 个 |
| **版本核对** | 第一次绑定时顺便读一次 `holoInfo()`（一次 RPC），对不上就打一条日志，不抛异常 |
| **缺 lrca / 缺函数 → 空操作** | 拿到的是 `function noop(){}`，脚本照常加载，只记一条 `logger.warn` |
| **拼错名字 → `undefined`** | 取不存在的属性得到 `undefined` 并警告一次（不会静默变成空操作，写错能看见） |
| **元信息** | `HL.apiVersion`、`HL.abiStamp`、`HL.list()`、`HL.has("名字")`、`HL.bind("名字")` |

## 性能账

| 场景 | 直接 `ll.import` | 导入层 |
|---|---|---|
| 脚本加载（用了 8 个函数） | 339 次绑定（循环写法），或手写 8 次 | **8 次** + 1 次版本探测 |
| 每次调用 | 一次跨语言调用 | 同上（缓存的函数，参数原样转发） |
| 取属性开销 | — | 首次之后是缓存命中；解构过的变量完全不经过 Proxy |
| `ll.import` 写在热循环里 | 每 tick 一次完整绑定 | 结构上不会发生（取属性即缓存） |

导入层**不会**减少单次调用的成本 —— 真正贵的是"每次调用一次跨语言跳转"，那部分靠少调用解决：
能合并成一次的（比如整批航点）尽量别在 `onTick` 里逐个调。

## 编辑器补全

`HologramLib.d.ts` 和 `.js` 放在一起即可生效；拷到你脚本目录旁边（B 方案）也能用：

```ts
// 自动生成，参数名与类型从 C++ 导出签名解析
locatorBarSet(playerName: string, key: string, x: number, y: number, z: number,
              dim: number, color: number, visible: boolean): boolean;
```

## 版本与刷新

导入层跟着源码里的 `exportAs` 走，由 `tools/gen_lse_shim.py` 生成：

```
python tools/gen_lse_shim.py          # 重新生成（改过导出就重跑）
python tools/gen_lse_shim.py --check  # CI 用：和源码不一致就失败
python tools/pack_release.py          # 打包（三个文件一起进 bin/HologramLib.zip）
node tools/shim_test.js               # 离线自测（不需要服务器）
```

构建时 `xmake` 的 `after_build` 会把这三个文件拷进 `bin/HologramLib/`，所以发布包里一定有它。

## 常见坑

- **`require` 的路径**是相对**当前脚本文件**的。A 方案用的是相对仓库根的路径
  `plugins/HologramLib/HologramLib.js`，在 LSE 里可用；B 方案则是 `./HologramLib.js`，
  文件得真的在脚本旁边。别写 `require("HologramLib")`（没装成 node_modules 包）。
- **`ll` 是全局的**，由 `lrca` 提供。导入层会探测它；没有 `lrca` 时所有调用都是空操作，
  不是抛错 —— 所以"什么都没反应"时先看日志里有没有 `ll.import 不可用`。
- 升级库之后 A 方案不用改；B 方案要重新拷一次。库比导入层新通常没问题（旧清单里的函数都还在），
  反过来（导入层比库新）才会遇到"缺函数 → 空操作"，而且首次绑定时的版本核对会提示。
