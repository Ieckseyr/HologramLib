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
LSE 导出 298 → **339**。

#### 新域

- **掉落物 `IItemActor`**（`itemActor*` 8 个）：在客户端显示掉落物，服务端没有对应实体。
  `setMotion` 可随时修改速度与方向，`take` 播放拾取动画后删除。
  物品只含 id、数量与 aux，不携带 NBT；需要自定义名称用 `itemDisplay*`。
  实体号段新增 `0x70`，不参与交互事件路由。
- **表单关闭 `IFormControl`**（`form*` 2 个）：发送 `ClientboundCloseForm` 关闭客户端当前表单。
- **隐身 `IPlayerVisibility`**（`playerHide*` 5 个）：控制其他客户端是否接收某个玩家的数据包。
  掩码可分别屏蔽出生 / Tab / 移动 / 状态 / 皮肤；`setHidden` 使已出生的玩家立即消失；
  `restore` 清除记录并重新发送出生包。
- **方位条 `ILocatorBar`**（`locatorBar*` 6 个）：按玩家维护航点，`key` 由调用方命名，库据此生成 UUID，
  更新与删除无需保存 UUID。`hideFromOthers` 转发到 `IPlayerVisibility`。

#### 既有接口尾部追加

- `IContainerMenu::blockEvent(menuId, b0, b1)`（`containerBlockEvent`）：在假箱子坐标发送 `BlockEvent`。
- `IPlayerNpc::playEmote(id, pieceId, lengthTicks)`（`playerNpcPlayEmote`）：播放原版表情动作，
  只发送给已见过该 NPC 的玩家。

#### 脚本导入层

发布包内新增 `HologramLib.js`、`HologramLib.d.ts`、`exports.json`（与 DLL 平级）。
脚本可用 `require("plugins/HologramLib/HologramLib.js")` 后按分组或按函数名调用，
也可继续使用 `ll.import`。函数首次使用时才与库绑定；缺少 `lrca` 或函数不存在时调用被忽略并记录警告。

另有 `holoInfo()` / `holoApiVersion()` 两个版本信息导出。

#### 工具

- `tools/test_abi_tool.py` 的"尾部追加"用例改成**动态取最后一条虚函数**作锚点 —— 之前写死字符串，
  头文件尾部一长就插到中间去，自测反而误报。

#### 验证

`xmake build` 通过；`registry_test` / `click_event_test` / `abi_tool check` / `test_abi_tool` /
`check_exports --check-docs` 全过。
