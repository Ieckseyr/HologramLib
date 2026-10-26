# 更新日志

按发布版本倒序排列。每个条目给出 API 版本、新增或变更的导出、行为变更，以及是否需要重新编译 C++ 消费方。
发布版本与 API 版本的对应关系见[版本与ABI对照.md](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)（维护者文档，不上文档站）。

## 版本概览

| 发布版本 | API | 协议线 | 主要内容 | 新增导出 | C++ 消费方 |
|---|---|---|---|---|---|
| `26.51.6` | 1.32.0 | 26.51 | 内部重构（发包 / tick / 生命周期收敛）、ABI 契约工具、支付规划修复、三个新域（掉落物 / 表单关闭 / 方位条） | 16 | **需重新编译**（虚表尾部追加：3 个新接口 + `IContainerMenu` / `IPlayerNpc` 各加方法） |
| `26.51.5` / `26.40.12` | 1.31.0 | 26.51 · 26.40 | 拉伸 / live 变换 / 动画预设扩展 / 停止配方 / 假玩家姿势装备骑乘 / FMBE 抽搐修复 / 图片平面与透明图层 / 容器会话隔离 | 若干 | 建议重新编译（虚表尾部追加） |
| `26.40.11` / `26.51.4` | 1.30.0 | 26.40 · 26.51 | 三个协议原语 + 一个时机通知 | 3 | 无需重新编译（虚表未变） |
| `26.40.10` / `26.51.3` | 1.29.0 | 26.40 · 26.51 | 视图覆盖补齐 `AddItemActor` 拦截、姿态/缩放/变种/颜色；修复元数据 tick | 1（`viewItemSpawnHooked`） | 需重新编译（`EntityView` 结构体变大） |
| `26.40.9` | 1.27.0 | 26.40 | `EntityView::skinId`（同 id 换模型）、真实实体交互事件、ABI 布局戳 | 类方法 | 需重新编译（虚表追加） |
| `26.40.8` | 1.26.0 | 26.40 | 逐观看者覆盖、四个界面域 | 若干 | 需重新编译 |

## 26.51.6 — API 1.32.0（内部重构、三个新域与修复）

### 新增域（API 1.31.0 → 1.32.0）

只加能力，没有改动既有行为。**虚表尾部追加**：3 个新接口 + `IContainerMenu` / `IPlayerNpc` 各加 1~2 个方法，
`HOLOGLIB_ABI_STAMP` 仍为 1；带旧头文件编译的 C++ 消费方需要重新编译（`abi/v1.32.0.json` 已更新）。
LSE 导出 314 → **330**。

#### 新域

- **协议层掉落物 `IItemActor`**（`itemActor*` 8 个导出）：客户端凭空出现一只掉落物，服务端没有它 ——
  不参与拾取、不进存档、没有物理。`spawn` 给初速度，`setMotion` 可以**飞行途中改速度与方向**
  （`SetActorMotion` 是"设定当前速度"，不是一次性冲量），`take` 只播"飞向玩家"的动画再删掉。
  走 `AddItemActor` / `SetActorMotion` / `TakeItemActor`。
  **边界**：物品不带 NBT（普通掉落物不需要），所以不碰物品描述符的 NBT 方言问题；
  要带名字的物品用 `itemDisplay*`。actor 号段新增 `0x70` 段，不参与 ghost 路由。
- **客户端表单控制 `IFormControl`**（`form*` 2 个）：库不生成表单，只提供"强制关掉"
  （`ClientboundCloseForm`），玩家 ESC 不掉、界面被别的插件顶掉、过场收界面时用。
- **玩家出站包控制 `IPlayerVisibility`**（`playerHide*` 5 个）：按"被看的人（主体）"屏蔽出站包 ——
  出生 / Tab / 移动 / 状态 / 皮肤逐类可关（`PlayerPacketMask`），或整体隐身（`setHidden`，复用视图覆盖域的
  hidden 通道），`restore` 清掩码并**立刻把出生包重新发给所有观看者**（借视图覆盖域"撤销即补发真身"那条流程）。
  在出站钩子上判定：认不出包 / 多玩家混装的 `PlayerList` / 收件人就是本人（且没开 `alsoSelf`）一律放行。
- **方位条 `ILocatorBar`**（`locatorBar*` 6 个）：按玩家维护航点（`LocatorBar`）。key 由调用方起名，
  库按 key 稳定派生 UUID，更新/删除不需要保管 UUID。另有 `hideFromOthers` / `isHiddenFromOthers`：
  把某个玩家从**其他人的客户端**上藏起来（其他人的客户端不再收到他的出生/移动/状态/皮肤/Tab 包），
  本人照常看得到自己；实现已并入 `IPlayerVisibility`（`locatorBarHideFromOthers` 是它的薄包装）。
  图标与颜色由客户端按这两个字段渲染。

#### 既有接口尾部追加

- `IContainerMenu::blockEvent(menuId, b0, b1)`：在假箱子的坐标发 `BlockEvent`（`containerBlockEvent`）。
  箱子盖 `b0=1` 开 / `0` 合；参数原样透传，别的方块事件也能用。
- `IPlayerNpc::playEmote(id, pieceId, lengthTicks)`：原版表情动作（`EmotePacket`，`playerNpcPlayEmote`）。
  只发给已见过该 NPC 的玩家。

#### 脚本导入层

`lse/HologramLib.js`（+ `.d.ts` / `exports.json`）由 `tools/gen_lse_shim.py` 从 `exportAs` 生成：
`const HL = require("./HologramLib.js")` 之后 `HL.itemActorSpawn(...)`，不用再写 `lrca` 探测与名字清单。
懒绑定（用几个绑几个）、缺 `lrca`/缺函数降级为空操作、拼错名字给 `undefined` 并警告、首次绑定核对一次版本。
另有 `holoInfo()` / `holoApiVersion()` 两个元信息导出。设计记录见 `docs/internal/脚本导入层设计.md`。

#### 工具

- `tools/test_abi_tool.py` 的"尾部追加"用例改成**动态取最后一条虚函数**作锚点 —— 之前写死字符串，
  头文件尾部一长就插到中间去，自测反而误报。

#### 验证

`xmake build` 通过；`registry_test` / `click_event_test` / `abi_tool check` / `test_abi_tool` /
`check_exports --check-docs` 全过。
