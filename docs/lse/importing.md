# 脚本里怎么导入（少写一半，快一截）

库里现在有 339 个导出。如果每个脚本一开始就循环 `ll.import` 一遍，那么：

- 加载期要付**几百次跨语言绑定**（用不到的也付了）
- 每个脚本都要复制一份 `lrca` 探测 + 名字清单，改一个函数名要改所有脚本
- 最容易踩的坑：把 `ll.import(...)` 写在 `setInterval` / `onTick` 里 —— **每次调用都走一遍完整绑定**

库自带一个生成好的导入层，把这三件事一次解决。

## 用法

把 `plugins/HologramLib/lse/HologramLib.js` 拷到脚本目录旁边，然后：

```js
const HL = require("./HologramLib.js");

// 直接用
HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
HL.playerHideSetHidden("Steve", true);

// 或者在启动时解构一次（之后完全不走 Proxy）
const { formClose, locatorBarSet, locatorBarClear } = HL;
```

也可以不拷，用绝对路径（注意 LSE 脚本目录是 `plugins/<你的插件>/`）：

```js
const HL = require("plugins/HologramLib/lse/HologramLib.js");
```

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

同目录还有 `HologramLib.d.ts`，放在脚本旁边，`HL.` 就能补全函数名与参数名/类型
（参数名和类型是从 C++ 导出签名里解析出来的，返回类型也一致）。

```ts
// HologramLib.d.ts（自动生成）
locatorBarSet(playerName: string, key: string, x: number, y: number, z: number,
              dim: number, color: number, visible: boolean): boolean;
```

## 版本与刷新

导入层是**生成物**，跟着 `exportAs` 走：

```
python tools/gen_lse_shim.py          # 重新生成（改过导出就重跑）
python tools/gen_lse_shim.py --check  # CI 用：和源码不一致就失败
node tools/shim_test.js               # 离线自测（不需要服务器）
```

生成物三个：`lse/HologramLib.js`、`lse/HologramLib.d.ts`、`lse/exports.json`（机器可读清单）。

## 常见坑

- **`require` 的路径**是相对**当前脚本文件**的。放在 `plugins/MyPlugin/` 下就用 `require("./HologramLib.js")`，
  文件得真的在那儿；别再写 `require("HologramLib")`（没装成 node_modules 包）。
- **`ll` 是全局的**，由 `lrca` 提供。导入层会探测它；没有 `lrca` 时所有调用都是空操作，
  不是抛错 —— 所以"什么都没反应"时先看日志里有没有 `ll.import 不可用`。
- 升级库之后重新生成一次导入层。库比导入层新时通常没问题（旧清单里的函数都还在），
  反过来（导入层比库新）才会遇到"缺函数 → 空操作"。
