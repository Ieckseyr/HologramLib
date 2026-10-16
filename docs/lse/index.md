# LSE 脚本接口

HologramLib 在脚本侧只占**一个命名空间** `HologramLib`：装上前置插件后，用 `ll.import("HologramLib", "函数名")` 直接拿函数来调，不需要在脚本插件的 `manifest.json` 里声明依赖，也没有配置文件。

本页教你怎么把它跑起来。具体函数按域分页，见文末目录。

---

## 1. 前置

脚本侧只需要 **LegacyRemoteCall（`lrca`）** —— 库靠它把 C++ 接口暴露给脚本。加载顺序无所谓：库在 `enable()` 里探测 `lrca`，没探测到会在服务器启动完成后再试一次。

---

## 2. 三行上手

```js
const holoCreate = ll.import("HologramLib", "holoCreate");   // 拿函数
const id = holoCreate(0, 70, 0);                             // 在世界原点上方 70 格建一个悬浮字
ll.import("HologramLib", "holoAddLine")(id, "§e你好");        // 加一行字
ll.import("HologramLib", "holoDraw")(id);                    // 画给所有人看
```

`ll.import` 拿到的是**函数本身**，直接调用即可。建议像上面第一行那样先存进变量，避免每次调用都查一遍。

---

## 3. 拿不到函数 / 调用没反应

| 现象 | 原因 | 怎么办 |
|---|---|---|
| `ll.import(...)` 拿到 `null` | 库没加载、`lrca` 没加载，或者函数名写错 | 先确认日志里有 `正在加载 HologramLib`；函数名对照本目录里的页面逐个核对大小写 |
| 调用返回 `false` / `-1` | 参数不合法（坐标不是数、id 不存在、玩家不在线…） | 看该函数的「返回值」说明 |
| 调用了但屏幕上什么都没有 | 只创建没显示 | 大多数域都要**再调一次显示函数**（如 `holoDraw` / `shapeDraw` / `entitySpawn`），或把内容作为参数传给创建函数 |
| 玩家重连后东西没了 | 客户端不落盘 | 在 `PlayerJoin` 里重新 `draw` 一遍（库不替你决定重发时机） |

判断某个函数在你的库里存不存在，用 `ll.hasExported`：

```js
if (ll.hasExported("HologramLib", "viewItemSpawnHooked")) {
  // 这个版本支持掉落物换类型
}
```

---

## 4. 一个完整例子

放到 `plugins/我的脚本/manifest.json` 指向的 `.js` 里即可（`type` 用 `lse-quickjs` 或 `lse-nodejs`）：

```js
const holoCreate = ll.import("HologramLib", "holoCreate");
const holoAddLine = ll.import("HologramLib", "holoAddLine");
const holoDraw = ll.import("HologramLib", "holoDraw");
const holoDestroy = ll.import("HologramLib", "holoDestroy");
const shapeCreateBox = ll.import("HologramLib", "shapeCreateBox");
const shapeDraw = ll.import("HologramLib", "shapeDraw");

// 玩家进服：在他头顶挂一行字，脚下画一个方框
mc.listen("onJoin", (player) => {
  const pos = player.pos;                       // FloatPos

  const holo = holoCreate(pos.x, pos.y + 2.5, pos.z);
  holoAddLine(holo, `§e欢迎，${player.realName}`);
  holoDraw(holo);

  const box = shapeCreateBox(pos.x - 1, pos.y, pos.z - 1, pos.x + 1, pos.y + 1, pos.z + 1);
  shapeDraw(box);

  // 玩家退服：自己收拾，别留垃圾
  mc.listen("onLeft", (p) => {
    if (p.xuid === player.xuid) {
      holoDestroy(holo);
    }
  });
});
```

> 库**不做业务**：命令、权限、持久化都在你的脚本里。库也不落盘 —— 脚本重启后要自己重新创建。

---

## 5. 点击 / 交互事件怎么拿（轮询）

界面类（容器、NPC 对话、交易）和 ghost 交互的事件**不进回调**，而是进队列，需要你在定时器或 tick 里自己轮询。一次调用会把队列**取走并清空**，条目是一行 `键=值` 组成的字符串：

| 轮询函数 | 条目格式（示例） |
|---|---|
| `containerPollClicks` | `player=Steve menuId=3 slot=13 closed=0` |
| `npcDialogPollClicks` | `player=Steve dialogId=1 scene=main button=0 actionId=accept closed=0 commands=…` |
| `tradePollSettlements` | `player=Steve menuId=3 offer=1 ok=1 reason=` |
| `fakeInvPollClicks` | `player=Steve slot=5` |
| `ghostPollInteractions` | `player=Steve action=1 domain=npc id=42 pos=(100.50,65.00,100.50)` |

```js
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");

setInterval(() => {
  for (const line of containerPollClicks()) {       // 取走并清空
    const f = Object.fromEntries(                     // 简单解析：按空格切成 键=值
      line.split(" ").map(kv => kv.split("=", 2)),
    );
    logger.info(`玩家 ${f.player} 点了槽位 ${f.slot}`);
  }
}, 100);
```

> 字段之间用空格分隔；`npcDialogPollClicks` 的 `commands=…` 本身可能含空格，解析时要按 `键=` 切分而不是只按空格。每个函数的完整字段说明见对应页面。

---

## 6. 调试

- 服务端控制台输入 `jsdebug`（或 `luadebug`）进入交互调试模式，直接敲脚本立即执行；再敲一次退出。
- HologramLib **自己不写任何日志**（正常发布版连日志字符串都不编进二进制）。排查时看函数的返回值，或先用简单的 `shapeCreateText` 确认基本通路是通的。
- 画面不对时，先在单人/本地服上确认坐标与维度对不对：所有坐标都是**世界坐标**，维度参数是 `0`（主世界）/`1`（下界）/`2`（末地）。

---

## 7. 按域查函数

| 想做什么 | 前缀 | 函数数 | 看哪页 |
|---|---|---|---|
| 画线 / 方框 / 球 / 文字形状 | `shape*` | 39 | [形状](shapes.md) |
| 多行悬浮字、渐变线 | `holo*` · `gradient*` | 26 + 11 | [悬浮字与渐变线](holograms.md) |
| 显示「钻石 x64」、物品悬浮实体 | `itemDetail*` · `itemDisplay*` | 2 + 33 | [物品显示](items.md) |
| 协议层假玩家（NPC）、皮肤注册 / 采集 | `playerNpc*` | 29 | [假玩家 NPC](player-npc.md) |
| 自定义实体、粒子形状、交互事件 | `entity*` · `particle*` · `ghost*` | 37 + 22 + 2 | [实体、粒子与交互](entities.md) |
| 交易菜单、虚拟容器、NPC 对话框 | `trade*` · `container*` · `npcDialog*` | 10 + 10 + 9 | [界面与容器](interfaces.md) |
| 背包虚容器、硫磺立方体展示 | `fakeInv*` · `sulfur*` | 9 + 10 | [背包伪造与硫磺立方体](inventory.md) |
| 客户端视图覆盖（换模型/隐藏/换皮）、设备感知 | `view*` · `sensing*` | 18 + 3 | [视图覆盖与感知](view.md) |

函数总数 270，全部在运行期注册，`ll.import` 都能拿到。

---

## 8. 几条通用约定

- **坐标都是世界坐标**（浮点）；维度用整数 `0/1/2`。
- **id 都是 `Integer`**：创建类函数返回 id，之后用它设置属性或销毁；失败返回负数或 `0`（各函数页面会写明）。
- **`§` 颜色代码**和 `\n` 换行直接写在文本里，例如 `"§c第一行\n§7第二行"`。
- **不需要 declare 依赖**：脚本插件的 `manifest.json` 里只要依赖 LSE 引擎本身。
- C++ 插件侧有同一套能力（接口一一对应），见 [C++ 接口参考](../reference/cpp-api.md)。
