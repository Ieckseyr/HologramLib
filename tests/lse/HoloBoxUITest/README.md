# HoloBoxUITest —— HologramLib 箱子 UI（虚拟容器）测试 · 指令版

一个 LSE 脚本，把 HologramLib 的 `container*` 十个导出全跑一遍：
**先自动跑 20+ 条断言**，再把按钮摆进格子里让你手点，验证「点击回传 / 就地刷新 / 换标题 / 可交互 / 关闭」。

## 装法

整个目录丢到服务器 `plugins/` 下即可（和 `ItemPhys` 等脚本插件一样）：

```
plugins/HoloBoxUITest/manifest.json
plugins/HoloBoxUITest/HoloBoxUITest.js
```

前置：`HologramLib`（本库）+ `legacy-script-engine-quickjs`。启动服务器后生效。

## 三条指令

进服后聊天栏直接敲（带不带斜杠都行）：

| 指令 | 作用 | 输出 |
|---|---|---|
| `/boxui` | 跑自检断言，然后打开 54 格测试界面 | 界面 + 聊天条 `menuId` |
| `/boxtest` | 只跑自检断言，不开界面 | 聊天条 `PASS x / FAIL y` |
| `/boxstop` | 关掉所有容器界面 + 清空点击队列 | 聊天条确认 |

实现上是 `mc.newCommand(name, desc, PermType.Any)` + `overload([])`（无参指令，不依赖 `ParamType` / `setEnum`，
所以不受脚本引擎版本差异影响）。如果 `mc.newCommand` 不可用，会自动退回聊天前缀 `.boxui / .boxtest / .boxstop`
并在控制台打一条 ERROR 说明原因。

## 断言覆盖（看控制台的 `[PASS]` / `[FAIL]`）

| 编号 | 验什么 |
|---|---|
| T0 | 10 个 `container*` 导出都存在 |
| T1 | `containerOpen` 返回菜单 id（玩家不在线时返回 -1 并中止） |
| T2a–d | `containerSetItem`：带 lore / 带数量 / 方块 / 带 damage |
| T3a–c | 越界槽位 54、负数槽位 → false；空 `type` = 清空该格 |
| T4a–b | `containerSetTitle` 就地换标题 |
| T5a–b | `containerSetInteractive(true/false)` |
| T6a–b | `containerIsOpen` / `containerGetIds` 包含本菜单 |
| T7a–e | 不存在的 id：`isOpen` / `close` / `setItem` / `setTitle` / `setInteractive` 全部 false |
| T8a–b | `containerPollClicks` 返回数组；`containerClearClicks` 后队列为空 |

手点阶段（`[CLICK]` 行之后跟一条 `[PASS]`）：

| 格子 | 行为 | 验证点 |
|---|---|---|
| 4 | 换成附魔金苹果 + 时间戳 | 同格就地刷新、无闪烁 |
| 8 | 标题在第 1/2 页之间翻 | 换标题不重开界面 |
| 13 | 切换可交互 | 拖动/交换被接住 vs 只回传 |
| 22 | 真发一颗钻石 | 点击回调里能跑业务（`giveItem`） |
| 26 | 翻页（重排按钮） | 重排 + 换标题 |
| 45 | `containerClose` | `closed=1` 回传 + `isOpen=false` |
| 53 | `containerCloseAll` | 全关 |

## 期望的控制台输出

```
[HoloBoxUITest] v1.1.0 已启动: 前置就绪
[HoloBoxUITest] 指令已注册: /boxui /boxtest /boxstop
[HoloBoxUITest] =========== 自检开始 (player=Steve) ===========
[HoloBoxUITest]   [PASS] T0 前置：10 个 container* 导出全部存在
[HoloBoxUITest]   [PASS] T1 containerOpen 返回菜单 id  → 3
...
[HoloBoxUITest] =========== 自检结束: PASS 21 / FAIL 0 ===========
[HoloBoxUITest]  [CLICK] player=Steve menuId=3 slot=4 closed=0
[HoloBoxUITest]   [PASS] C1 点击后就地刷新该格
```

失败时会打 `[FAIL] ... → 期望 X，实际 Y`，直接对着看哪个导出行为变了。

## 排错：点击回传的 menuId 对不上

如果日志长这样：

```
[HoloBoxUITest]  /boxui: player=X menuId=1
[HoloBoxUITest]  [CLICK] player=X menuId=0 slot=4 closed=0
```

说明**库回传的 menuId 没填**（`Menu::menuId` 恒为 0）。这曾经是 HologramLib 26.51.6 的一个真 bug
（container 域并表时漏了回填，已在 `fix(container): 回填 Menu::menuId` 修掉）。
脚本对此的处理：

* 回传 **0** → 按玩家认领这次点击（一个玩家同时只有一个菜单），并记一条 `[FAIL] C9 点击回传的 menuId 与打开时一致`
  —— 测试继续跑，你也能立刻看到库有问题；
* 回传**别的非 0 id** → 认定是别人家的菜单，忽略（`[WARN] 不是本脚本的菜单`）。

修好后第一次点击会打一条 `[PASS] C9 点击回传的 menuId 与打开时一致 → menuId=1`（每个菜单只验一次），
之后所有点击事件的 `menuId` 都是对的。

## 两个容易踩的坑（脚本已处理）

1. **打开有固有延迟**（默认 7 tick ≈ 350ms）：客户端要先应用箱子方块与方块实体，界面才绑得住位置。
   延迟期间填格不会丢（走的是逐格刷新那条包），只是界面稍晚出现 —— 所以打开后立刻填格是对的。
2. **`closed=1` 事件的顺序**：库在派发关闭事件**之前**就把菜单摘掉了，所以回调查 `containerIsOpen`
   必须是 `false`。如果哪天这里变成 `true`，说明关闭路径的顺序变了。

## 开发用：离线跑一遍

`mock_run.js` 用假的 `ll` / `mc` / `logger` 把脚本跑起来（不连服务器），**连指令注册一起模拟**：

```bash
node mock_run.js
```

覆盖 9 个场景：指令注册（3 条、都 setup、都是无参重载）、`/boxtest`、`/boxui` + 点 5 个按钮、关闭回传、
布局外格子、未知菜单、`/boxstop`、控制台执行（应报"只能由玩家执行"）、缺前置时的降级提示。

当前结果：**PASS 72 / FAIL 0**（WARN 两条都是刻意构造的负向用例）。
