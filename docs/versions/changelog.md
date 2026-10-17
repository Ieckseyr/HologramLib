# 更新日志

按发布版本倒序排列。每个条目给出 API 版本、新增或变更的导出、行为变更，以及是否需要重新编译 C++ 消费方。
发布版本与 API 版本的对应关系见[版本与 ABI 对照](api-version-matrix.md)。

## 版本概览

| 发布版本 | API | 协议线 | 主要内容 | 新增导出 | C++ 消费方 |
|---|---|---|---|---|---|
| `26.40.11` / `26.51.4` | 1.30.0 | 26.40 · 26.51 | 三个协议原语 + 一个时机通知 | 3 | 无需重新编译（虚表未变） |
| `26.40.10` / `26.51.3` | 1.29.0 | 26.40 · 26.51 | 视图覆盖补齐 `AddItemActor` 拦截、姿态/缩放/变种/颜色；修复元数据 tick | 1（`viewItemSpawnHooked`） | 需重新编译（`EntityView` 结构体变大） |
| `26.40.9` | 1.27.0 | 26.40 | `EntityView::skinId`（同 id 换模型）、真实实体交互事件、ABI 布局戳 | 类方法 | 需重新编译（虚表追加） |
| `26.40.8` | 1.26.0 | 26.40 | 逐观看者覆盖、四个界面域 | 若干 | 需重新编译 |

## 26.40.11 / 26.51.4 — API 1.30.0

26.40 与 26.51 两条协议线的发布内容一致。

### 新增导出

| 导出 | 说明 | 返回值 |
|---|---|---|
| `viewPollSpawnEvents()` | 返回并清空出生事件队列。每条格式为 `drop\|uid\|dim\|x\|y\|z\|` 或 `actor\|uid\|dim\|x\|y\|z\|<类型名>`；同一实体只入队一次 | `Array<String>` |
| `itemDisplayCreateFull(...)` | 创建物品悬浮，并在创建时传入物品 NBT 与附魔光效，避免创建后再调用 `itemDisplaySetItemWithNbt` / `itemDisplaySetGlint` 引起的重建。参数与 `itemDisplayCreateAdvanced` 相同，末尾增加 `nbt`、`glint` | 悬浮 id |
| `itemDisplayEmitForEntity(playerName, uid, item, aux, offsetY, rotX, rotY, rotZ, scale, nbt, glint)` | 对指定玩家，把 uniqueId 为 `uid` 的现有实体按 FMBE 规则渲染为物品模型：依次发送 `RemoveActor`、`AddActor`（fox）、`MobEquipment`、`SetActorData` 与动画序列。使用调用方给定的 id，库内不保存状态。`rotY` 传 `"0"` 时按该实体当前速度方向计算朝向 | `1` 物品平面 · `2` 3D 方块 · `<0` 失败 |

### 修复

- **附魔物品不显示光效。** 此前的导出无法在创建或发射的同一步给出 `glint`，必须另调 `itemDisplaySetGlint` 才可能生效，漏调或晚调即无光效；同时附魔内容由 `nbt` 决定，调用方先 `itemDisplaySetItemWithNbt` 再 `itemDisplaySetGlint` 会触发一次重建。
  本版本 `itemDisplayCreateFull` 与 `itemDisplayEmitForEntity` 都在首次发包时接受 `nbt` 与 `glint`，附魔物品自第一帧起带光效，不依赖后续调用。

### 范围

本版本只新增上述三个导出。以下行为不由库实现，由调用方决定：是否接管某只实体、向哪些玩家发送、落地后如何恢复角度、方块与物品分别使用何种尺寸。

### 兼容性

本次只新增 LSE 导出（按名称注册）。`include/hologramlib/HologramLib.h` 的虚表未变，`HOLOGLIB_ABI_STAMP` 保持为 1，已编译的 C++ 消费方无需重新编译。

### 参考实现

`ItemPhys` 1.8.0（LSE 插件）以 `viewEntity` 的 `hidden=1` 配合 `itemDisplayEmitForEntity` 实现掉落物模型接管。

## 26.40.10 / 26.51.3 — API 1.29.0

26.51.3 将 26.40.8、26.40.9、26.40.10 三个批次迁移至 26.51 协议线，能力与 26.40.10 一致；26.51 侧仅有 `PlayerAuthInput` 输入位的适配。

### 新增与变更

- **`AddItemActor`（id 15）纳入拦截。** 掉落物出生走该包而非 `AddActor`，此前只拦后者，"换类型 / 隐藏"只能作用于设置覆盖时已在观看的玩家；引擎随后发送的 `AddItemActor` 会重新写入物品图标（2026-10-16 抓包：`RemoveActor` 与 `AddActor(cod)` 之后 13 ms 出现同 uniqueId 的 `AddItemActor`）。现为：`hidden` 丢弃该包；`identifier` 丢弃原包并手写一只同 `runtimeId` / `uniqueId` 的 `AddActor`，属性采用 health 20 / scale 1 的已验证配方。
- **`EntityView` 增加变形后的外观项（尾部追加）。** 新增 `pose`、`metaScale`、`variant`、`markVariant`、`colorIndex`、`color2`，各带 `hasXxx` 标志，`false` 表示该项不变。LSE `viewEntity` 的 spec 串增加 `pose=`、`scale=`、`variant=`、`mark=`、`color=`、`color2=`。协议项：`PoseIndex(79)`、`Reserved38`、`Reserved53`/`Reserved54`、`Variant(2)` 为 Int、`MarkVariant(43)` 为 Int、`ColorIndex(3)` 为 Byte、`Color2Index(82)` 为 Byte；类型取自解码后的真实出生包。
- **新增导出 `viewItemSpawnHooked()`**，用于脚本侧能力协商。

### 修复

- `SetActorDataPacket` 与 `UpdateAttributesPacket` 在 26.40 线格式中含 `mTick`，视图覆盖路径此前填 0，客户端按过期包丢弃，导致元数据与属性更新不生效（抓包显示出生包内 `id2`、`id43`、`id3`、`id82` 值正确，画面仍为默认外观）。现统一填写 `currentViewTick()`，并在出生包之后补发一条 `SetActorData`、再排一条 +2 tick 的确认。
- `ghost*` 的两个导出（`ghostPollInteractions`、`ghostClearInteractions`）此前未接入 `ModEntry::exportLseFunctions()`，脚本侧 `ll.import` 返回 `null`；现已接回。

### 需要重新编译

`EntityView` 结构体大小改变（尾部追加六个字段），直接构造该结构体的 C++ 消费方需重新编译；虚表槽位不变。

### 备注

- `26.51.2` 已撤回，使用 `26.51.3` 或 `26.51.4`。
- 26.40.9 与 26.40.10 关于实体元数据与 `AddItemActor` 的结论基于 26.40 抓包；26.51 线只核对了协议常量逐字节一致，未在 26.51 客户端复测。

## 历史条目

以下为更早版本的原始条目，保持提交时的文本。

- `26.40.9`（API 1.27.0）：**`EntityView::skinId`（+ `scale` / `yOffset`）—— 非玩家实体"换成我们的模型"**（拦下的出生包 + 库自己的包替换, **同一个 runtimeId/uniqueId**）: 客户端看到已注册皮肤（可自定义几何）的玩家模型, 服务端那边还是那只生物 —— **点它/打它/瞄准都是它本人**。配套吃掉该实体的 `MoveActor*`(18/111) 并每 tick 用 `MoveActorAbsolute` 推位置/朝向、`PlayerList` 条目 20 tick 后摘掉、撤销时重发真实出生包；`scale` 等比碰撞箱（元数据 53/54）、`yOffset` 把模型原点对齐到脚位。消费方示例: MSkinventory 的"同 id 换皮"。
  - **（批次内追加，未发布）移除：NPC 对话框 `avatarViewSpec` 的 `skin=` 分支**（收缩, 明记）。它会用 `PlayerList(Add)`+`AddPlayer` 把载体换成"玩家模型"—— 实测客户端在该载体上点进菜单会把界面关掉（同一次测试里 `type=` 载体：僵尸/鸡, 一切正常）, 属该路径当前不可用。按"库只做机制"的定位, "让客户端认它是玩家"的路线留给消费方自建（`playerNpcs()` 造真·假玩家 + `npcUniqueIdOverride` 把对话挂到它 id 上）。**保留**: `type=` / `name=`（可自定义实体类型/标题）与 `avatarSkinVariant`（NPC 内置皮肤变体）。`avatarViewSpec` / `avatarSkinVariant` 字段本身不动（**ABI 不变**, 消费方无需重编译）; 内部顺带掉一段从未生效的死代码（玩家模型载体的正段 id 分配）。
  - **真实实体交互事件**：`addActorInteractListener` / `removeActorInteractListener`（类尾追加）—— 玩家右键 / 攻击**服务端真实实体**时回调；复用库已有的收包钩子（不新增 detour 层），多播互不覆盖，返回 token 用于移除。
  - **ABI 布局戳**：头文件携带虚表布局描述符 + 消费方编译期自检（库侧运行期拦截"布局不一致的消费方"）—— 把 2026-10-09 / 10-10 两次"中段挪槽"启动崩溃固化成机制；详见 [版本与ABI对照.md](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)「ABI 事故（明记）」。
  - LSE：`viewEntity` 的 spec 串新增 **`skin=<皮肤id>`**（等价 `EntityView::skinId`；皮肤取自 `IPlayerNpc` 注册表）。
- `26.40.8`（API 1.26.0）：**视图覆盖残骸清理 + `IPlayerNpc` 追加五方法 + 悬浮字/text 域收缩与补全**（收缩明记，见 [版本与ABI对照.md](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)）。
  - **视图覆盖（`IViewOverride`）代码整理**：行为不变 —— 出站钩子只决定"放行 / 丢弃"，**原包字段一律不改**
    （自 26.40.7 起即如此）。本次清掉与行为不符的**旧世界残骸**：重写过期的"能原包改的就原包改"顶注、
    删除就地改写的死代码（`dataListOf` / `setItemById` / `cloneItems` / `restoreItems`）、判定链路全 const 化
    （读包只为识别身份）。需要"改"的地方一律**丢原包 + 库自己手写协议包**（换类型出生包其余字段照抄原包；
    名字牌由库的 `SetActorData` 紧随其后压过去；方块覆盖在收到区块后自动补发；被替换玩家的位移包丢掉、
    位置由心跳用 `MoveActorAbsolute` 推）。`view*` LSE 导出补齐**逐字段**（6→16 函数；唯一 id 判定修正——玩家 id 本身是负数）。
  - **`IPlayerNpc` 尾部追加五个方法**（+ `setEntitySpawnCallback`；既有 ABI 不变）：
  - **`setPositionLight(id, x, y, z, dim)`**：只发 `MoveActorAbsolute`（不重建实体、不重发皮肤，无闪烁），
    与 `setRotationLight` 共用同一条轻脏通道，同一 tick 内合并为一条包。用途是"每 tick 跟随会移动的东西"
    （典型场景：把自定义模型挂在移动中的生物身上）。`dim` 与当前维度不同时返回 `false`，跨维度请仍走 `setPosition` 重建。
  - **`injectSkin(viewerName, targetName, skinId)` / `injectSkinAll(targetName, skinId)`**：把注册表里的皮肤
    按 **target 自己的 UUID / uniqueId** 发一条 `PlayerList(Add)` 就地更新皮肤条目。`viewerName` 空串 = 所有在线玩家
    （**含 target 本人** —— 这是"让玩家自己看到自己被换肤"的路径；Geyser 对 session 玩家自身也是走 PlayerList 而不是 `PlayerSkinPacket`）。
    条目带真实 xuid、`TrustedSkinFlag = true`；皮肤对象自带 `OverridesPlayerAppearance = true`（覆盖玩家客户端上已装备的皮肤，缺它客户端会拒绝）。
    **不自动重发**：客户端偶发丢帧时由消费方自己再调一次（Geyser 的做法是 ~100ms 后重发）。
  - **`playAnimation(id, animation, stopExpression, durationTicks)` / `playAnimationTo(id, playerName, ...)`**：对已见过该 NPC
    的玩家发 `AnimateEntityPacket`（`animation` = 资源包里的动画标识符; `controller` 名库内按 NPC id 自动唯一化, 不需要资源包里
    预先存在同名控制器; `stopExpression` 空 = 常驻, `"query.any_animation"` = 立刻停; `durationTicks > 0` 到期自动补发停止包）。
    同一版追加 `setEntitySpawnCallback`（NPC 对某玩家出生完成回调, 供补发动画/状态）。与 customentity 域同款队列（+2 tick 发出）。
  - `setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` 为 **C++ 接口（暂无 LSE 导出）**；
    **皮肤一族已补 LSE 导出**：`playerNpcInjectSkin` / `playerNpcInjectSkinAll`（注入给指定/全体观看者，
    **含被换肤者本人——自己看自己被换肤**）与 `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64`
    （皮肤 blob 的 base64 持久化配对：消费方落盘，重启后源玩家不在线也能恢复）——`playerNpc*` 25→29，
    总计 265（Disguise / MSkinventory 类"玩家自视换肤"的脚本侧正路）。消费方用 `>= 0x012000` 门住这些方法。
  - **收缩（移除 8 个 holo 导出 + 2 组内部能力）**：`holoSetLineGradient`（渐变）/ `holoSetLineRainbow`（彩虹）/
    `holoSetLineColor`（行级颜色）/ `holoSetLineScroll`（滚动）/ `holoSetVerticalAnimation`（垂直动画）/
    `holoSetLineSpacing`（行距）/ `holoSetLineScale`（行级缩放）/ `holoTick` —— 这些在"整块文本 = 单一文本形状"
    的模型下**不可实现或空转**（行级样式只有第 0 行生效、彩虹落成白色、行距字段本协议不存在、自驱 tick 的偏移
    无人消费）。行级颜色请改用文本内嵌 § 颜色代码；行级缩放改 `holoSetScale`（整块）。
  - **补全（新增 6 个 holo 导出 + 3 个 shape 导出）**：`holoSetScale`（整块缩放）/ `holoSetBackgroundColor` /
    `holoClearBackgroundColor`（背景框颜色; 不设 = 客户端默认色）/ `holoSetDepthTest`（穿墙可见性:
    `false` = 始终渲染（默认，穿墙可见）, `true` = 被方块/实体遮挡）/ `holoSetRotation` / `holoClearRotation`
    （三轴 Euler [Pitch, Yaw, Roll]，**单位 = 度**（官方脚本 API 口径）; 设置后文本不再面向相机）;
    形状域同步补 `shapeSetBackgroundColor` / `shapeClearBackgroundColor` / `shapeSetDepthTest`。
  - **动态行回归（重构自 Phantom——LGPL-3.0，已按许可标注来源，见文末「第三方来源标注」）**：`holoSetLinePool(id, line, [内容池], 间隔ms)`
    （内容池按间隔**时间取模轮播**，无状态、多个浮字同池同相）+ `holoSetLineParseVariables(id, line, on)`
    （行级变量开关，默认开）。轮播/变量刷新由库内 **0.5s 节流**承担、**内容变了才重发**
    （区别于本版移除的自驱"偏移"：那时每拍都在动、但驱出来的东西没有消费方）；变量集补
    `{dimension}` / `{x}` / `{y}` / `{z}`。另修**逐观看者（含 `{var}`）文本的刷新只改内存、不发包**
    （此前更新要等 15s 兜底重发才到客户端）——现在 `refresh`/驱动会逐玩家原地重发（同 networkId，无闪烁）。
    `holo*` 24→26，导出总数 265→267。
  - **NPC 对话框头像自定义**：`NpcDialogSpec` 尾部新增两个参数 —— `avatarSkinVariant`
    （NPC **内置皮肤变体** 0..59：写进载体 ActorData 的 `SkinId(104)`，聊天框里的头像/模型换内置皮肤；
    `-1` = 不加该项，与旧行为逐字节一致）+ `avatarViewSpec`（**在这只载体上叠一层 IViewOverride**：
    spec 语法同 `viewEntity` —— `type=` 换载体类型（测试矩阵: 僵尸 / 鸡）；`skin=` 分支已移除（见上, 未发布批次内的收缩）。原描述留档：`skin=` 用 **playernpc 注册表
    里的皮肤**（MHR/MeowSkin 注册/采集的都在这张表）→ 载体改为**玩家模型**（PlayerList+AddPlayer 同 id，
    Tab 条目 1s 后摘掉）、`name=` 换界面标题；`type` 换非 NPC 家族能否照常弹界面需实机验证）。
    配套 LSE：`npcDialogSetAvatar(id, skinVariant, viewSpec)`（就地更新 + 重开，与翻页同一条 0 闪烁路径）；
    `npcDialog*` 8→9，导出总数 267→268。要"整只模型完全自定义"也可走既有 `npcUniqueIdOverride` 路线：
    用视图域 / 自定义实体生成展示实体、把对话挂在它 id 上
    （1.27.0 的 `EntityView::skinId` 同-id 换模型路线也可用于那只展示实体）。
  - **修复**：① 文本形状的 `setRotation` 此前**不生效**（缺少 `useRotation` 标志，文本永远面向相机）——
    现在同时下发 `useRotation` 与双面渲染，`clearRotation` 恢复面向相机；② `shapeDestroyAll` /
    `shapeDestroyBatch` 只清服务端内存、**不发移除包**（客户端残留显示），现已补齐；
    ③ 跟随玩家不再依赖库内 tick —— 位置在 `setFollowPlayer` / `draw` / `refresh` 调用时就地解析；
    ④ **颜色 R/B 通道颠倒**（int32 颜色沿用 v944 旧字节序，v2168 实际为 **ARGB** —— 填红显蓝）：
    打包 `toPacked` 与解包 `getColor` 已统一修正（形状 / 文本颜色 / 背景框全部受影响；
    修好后配置里的颜色按字面显示，若此前为凑颜色反填过值请改回）；
    ⑤ **"伪装成某个玩家"被当成撤销**（`overrideEntity` 的"全空视为撤销"判空漏了 `asPlayer`
    字段）—— 报"无法伪装"且顺手清掉旧覆盖；已把 `asPlayer` 计入判空；
    ⑥ **玩家变生物会把本人客户端卡死**（覆盖对"自己"也生效：给本人发 `RemoveActor` + 同
    runtimeId 的 `AddActor`）—— 新增"**自己的客户端不参与自己实体的覆盖**"规则，覆盖/恢复/
    出站三条链路全部跳过本人（自己的 `SetActorData`、`MovePlayer` 也不再被吃）；
    **换肤类例外：含本人**（`PlayerList(Add)` 发给本人是"自己看自己被换肤"的正路，自视也随之变）；
    ⑦ **被替换玩家的位移从未推送**（`sendMoveActorAbsolute` 是死代码，替身站桩不动）——
    现在每次收到他的输入包就按输入位置（眼位 → 脚位）给看得见的观看者推 `MoveActorAbsolute`（跳过本人）。
  - 消费方提示：升级后在 `>= 0x012000` 门住新方法；按旧 holo 行级 API 写的代码**重新编译会得到弃用警告**
    （`[[deprecated]]`），LSE 侧 `ll.import` 得 null 显式失败——**不会静默变行为**。**ABI 安全**：被收缩的
    8 个方法在 C++ 接口里保留为**废弃空槽**（非纯虚 + 空实现），虚表槽位与 1.25.0 一致——未重编译的旧
    消费方二进制不会错位（与 26.40.3 交易菜单收缩的区别：那次删的是尾部方法，这次在中段，直接删会
    整体前移虚表）。库内消费者已同步（MeowHolographicRenderer、ZXPanel、Meow）。
- `26.40.7`（API 1.25.0）：**新增客户端视图覆盖 `IViewOverride`**（协议层拦截; 只拦不发）。
  - **拦截的是"服务端本来要发给这名玩家的包"**：库挂在 BDS 的按收件人发包汇合点
    （`NetworkSystem::send` / `sendToMultiple`）—— 在这些汇合点上库**只决定拦不拦**
    （放行 / 丢弃），**从不修改引擎包字段**；需要"改"的一律丢原包 + 库**自己手写协议包**补发
    （sculk 构造 → 回读校验 → 原始字节发送）。反编译实测实体出生包不走 `LoopbackPacketSender`
    （原先挂在那里 → 换类型不生效），故改挂这里。
  - **心跳兜底**：服务器每 tick 自查一次 —— 实体"重新进入视野"、玩家换区块时把覆盖重新推一遍。
    出生包拦漏 / 方块覆盖被区块重发冲掉都靠它恢复；没用到这个功能时只是一次原子读。
  - **能做**：实体换类型（丢掉原出生包，库用同一 `runtimeId`/`uniqueId` 自己发一只目标类型，其余字段照抄原包）、
    换名字牌（元数据 `Name(4)` / `NametagAlwaysShow(81)`：原包照发，库自己的 `SetActorData` 紧随其后压过去，
    服务端后续改名也压得住）、**对该玩家隐藏**（出生包与更新包不发给他，已在客户端上的补一条 `RemoveActor`）、
    方块显示成另一种方块（丢掉原 `UpdateBlock` / `UpdateBlockSynced`，库自己发一条带覆盖方块网络 id 的
    `UpdateBlock`；服务端发的本来就是覆盖方块时照发不改）。
  - **逐玩家隔离**：覆盖表按玩家名 + 一份全局表；多收件人路径（`sendToClients` / 广播）展开成逐收件人套用 ——
    只为让"放行 / 丢弃"逐人生效；因为从不改字段，不存在"后一个收件人看到前一个覆盖"的串味。**幂等**：
    判定认 id 不认位置，同一条包被多条路径各判一次结果相同；已是覆盖方块的更新包直接放行。
  - **零开销**：库内没有任何覆盖时，钩子一条分支直接放行（不查玩家、不读包）；命中判定也只看
    目标玩家有没有覆盖，没命中就原样放行。**fail-safe**：不认识的包、认不出来的实体一律原样放行。
  - **26.40 做不到的（所以没进 API）**：逐玩家的**缩放 / 发光 / 隐身** —— 元数据表里没有 `scale`，
    也没有 `glowing` / `invisible` 旗标（基岩版隐身是效果，走 `MobEffectPacket`）。
    自定义实体的逐客户端缩放/装备槽仍归 `ICustomEntity` 的 per-viewer 覆盖（1.21.0）。
  - **实体类改动都是立刻生效**（换类型 / 隐藏 / 撤销）: 库里对那只实体做一次"移除 + 按目标状态
    重发一只"（同一个 `runtimeId` 与 `uniqueId`，服务端后续的移动/状态更新照常作用在它身上）。
    代价是客户端那只实体会被重建 —— 只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步。
    撤销时离得太远（>64 格）或已消失的实体不重建；玩家实体无法重建（`AddPlayer` 没有类型字段）。
  - 方块覆盖在区块重发后由库自动补发（收到区块时）。
  - 判定逻辑独立在 `src/view/ViewOverrideLogic.h`（坐标打包 / 区块归属 / 幂等判断 / 名字牌计划 / 类型名清洗），
    离线可测：`tests\check-view-override.bat`（**54 项**）。
  - **玩家实体也能覆盖**（同一版内）：`EntityView::asPlayer = "<在线玩家名>"` → 只给该观看者重发一条
    `PlayerList(Add)`（该实体自己的 UUID + 源玩家皮肤），与原版"皮肤更新"同机制、可立刻撤销;
    `identifier = "<生物>"` → 玩家模型渲染不了生物 → 库走**替换**: 吃掉他的出生包, 用**同一个
    runtimeId/uniqueId** 发一只该类型的实体, 位置/朝向按他发来的输入包用 `MoveActorAbsolute` 推。
    **id 没变 → 服务端那边仍是那个真玩家, 打它就是打他本人。** 撤销时把替代实体移走。
  - **出生包配方对齐 customentity 域**（线上验证过的那一套）: 必带 flags 元数据项 + `minecraft:health` /
    `minecraft:scale` 属性, 且**后面紧跟一条 `UpdateAttributesPacket`**（客户端对出生包里的属性只初始化
    不生效）—— 之前缺这几项, 玩家进服时心跳一发重发就会让客户端报错（"第一次进出错"）。
    另修正 `NametagAlwaysShow` 的**协议值反转**（0 = 常显）, 并给刚进服的玩家约 2 秒宽限期（客户端还在
    收区块时不塞实体包）。
  - **输入操作 → 代理（协议层执行）**：入站 `PlayerAuthInput` 当真相 —— 位置/朝向/头顶朝向、66 位输入位
    （跳跃/潜行/疾跑/游泳/爬行/飞行…）、`ClientPlayMode`（界面状态）与物品操作字段；代理按它复现 A 的动作
    （1 秒内无新输入才退回服务端状态）。**动作类包**（挥臂/攻击/受伤/死亡）从服务端的 `Animate` /
    `ActorEvent` 照搬到代理（runtimeId 换成代理的，100ms 去重防重复播放）。手持物品/快捷栏持续镜像。
  - **代理落点修正**：玩家在协议层上报的 y 在**眼睛**处（Bedrock 特有）—— 代理改用碰撞箱下沿取真实脚位，
    输入包的 y 用"眼到脚"差值换算（站/潜行/游泳都对）。
  - **两条纪律**: 出站钩子**只决定拦不拦**（放行 / 丢弃）, **从不修改引擎包字段** —— 需要"改"的一律
    丢原包 + 库**自己手写协议包**补发; **数据来源只有协议包**（入站 `PlayerAuthInput` / 出生包载荷 /
    `Animate`·`ActorEvent`），不读服务端实体状态。

- `26.40.5`（API 1.23.0）：**新增背包虚容器 `IFakeInventory` 与硫磺立方体展示 `ISulfurDisplay`**。
  - **协议层改写客户端看到的玩家背包**：一条 `InventoryContentPacket`（`ContainerId = Inventory(0)` +
    `FullContainerName = InventoryContainer(29)` + 0..35 号描述符）把整份内容换成调用方给的那份，
    单格改动走 `InventorySlotPacket`；服务端背包一个字都不动。方案取自参考实现 GMLIB 的 `ChestUI`
    （它逐格写 `InventorySlot`, 容器 id 同样是 `Inventory(0)` + `InventoryContainer(29)`）。
  - **功能项与虚拟容器一致**：点自己背包里的伪造物品 → 回调报槽位号；物品不会真的被拿走
    （客户端按伪造内容发请求, 服务端真实槽位对不上 → BDS 判失败 → 客户端撤回预测）。库随后把那一格
    重发一次, 让伪造内容不被这次回滚冲掉。
  - **与交易菜单 / 虚拟容器共存**：打开界面时 BDS 会重发背包内容把伪造内容覆盖，所以本域按
    `refreshIntervalTicks` 周期重发（默认 20 tick = 1s），也可 `refresh()` 手动补。
  - **LSE**: `fakeInvApply` / `fakeInvSetSlot` / `fakeInvRefresh` / `fakeInvClear` / `fakeInvClearAll` /
    `fakeInvIsActive` / `fakeInvGetPlayers` / `fakeInvPollClicks` / `fakeInvClearClicks`。
  - **离线检查** `tests/check-fake-inventory.bat`（本地工具）：拿 **BDS 真实抓包**核对线格式（帧落在包尾、容器 id 0 /
    36 格 / 容器名字节 0 / storage 空描述符、以及物品 UserData 的方言指纹：`06 00 "Damage"` 而不是 `06 "Damage"`），
    10 项全过。物品本身由 BDS 序列化, 所以不再有"逐字节自造"的检查, 改为钉住布局事实。
  - 物品请求钩子（147 + AuthInput 内嵌）现在由两个域共用，从 `container/` 移到了
    `src/interaction/PlayerInteractionHooks.cpp`：容器域认 `LevelEntityContainer(7)` 与 dyn 101..199，
    背包域认 `CombinedHotbarAndInventory(12)` / `Hotbar(28)` / `Inventory(29)`，互不抢。
  - **修复（实测事故）**：第一版手写物品描述符写错了 User Data 的 NBT 方言 → 客户端卡死；且会话没随掉线清理，
    重连时 tick 驱动又重发一遍 → "进不去服务器"。现在物品交给 BDS 序列化，并注册了 `PlayerDisconnectEvent`
    清理会话（`FakeInventoryManager::onPlayerLeave`），周期重发间隔也钳了下限（最快 5 tick）。
  - **新增硫磺立方体展示 `ISulfurDisplay`（第二种摆放方式）**：生成 `minecraft:sulfur_cube`，把方块**装在它的主手**
    （行为包里立方体就是靠 `slot.weapon.mainhand` 拿"吞下去"的方块, 客户端按装备渲染）—— `SulfurDisplaySpec::block`
    与 `setBlock` 是本域的核心 API；**隐身是 spec 上的一个参数**（`invisible`, 默认关）。外观档位用**手写的
    `ChangeMobProperty`(182)** 下发 `minecraft:sulfur_cube_archetype`。本域全部委托给 `ICustomEntity`，所以
    缩放/视距/白名单/逐客户端朝向直接继承；`ICustomEntity::setMobProperty` 同时开放给其它实体的 `client_sync` 属性。
    **"吞生物"不做**：协议层实体没有 AI，试过"让实体骑在立方体上"的近似，实测观感不成立，已整体移除。
- `26.40.4`（API 1.22.0）：**交易菜单改为纯展示 + 新域补齐 LSE 导出**。
  - **交易菜单不再有任何点击监听**：`TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` 与 `ITradeMenu` 的六个监听方法整体删除，`TradeMenuSpec` 去掉 `displayOnly` / `acceptPaymentPlacement` —— 本域从此只负责"打开界面 + 摆出交易表"。原来的 147/AuthInput 钩子与放料接住逻辑随之移除（**留在 `src/container/ContainerInteractionHooks.cpp`**：那两个钩子如今只服务虚拟容器的点击）。要"能点、点了有回调"的列表界面用虚拟容器（`IContainerMenu`）。
  - `ITradeMenu` 新增 `addOffer` / `setTier`：追加一条交易或改档位/经验条时**就地重发**交易表，不必关掉重开。
  - **四个新域补齐 LSE 导出**：`trade*`（7）、`container*`（9）、`npcDialog*`（7）、`sensing*`（3）；容器与 NPC 对话的点击在脚本侧走**轮询队列**（`containerPollClicks` / `npcDialogPollClicks`，取走并清空，条目是可切分的字符串），`ghost*` 早就用的同一套办法。
  - `entity*` 补逐客户端渲染导出：`entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`。
  - API 版本因此抬到 **1.22.0**：撤回已发布过的东西属于收缩（正常该走大版本），这里抬次版本并在 [`版本与ABI对照.md`](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md) 明记原因。按 1.21.0 写过交易菜单点击监听的代码升上来会**编译报错**（不会静默变行为）。
- `26.40.3`（API 1.21.0）：**四个新能力域（村民交易菜单 / NPC 对话框 / 虚拟容器 / 感知域）+ 逐客户端渲染 + 一次裁剪**（清单见上表 1.21.0 各行）。
  - **村民交易菜单 `ITradeMenu`**：协议层自建 `UpdateTrade` 开界面（包级字段与 Offers NBT 都与 BDS 抓包逐字节对拍），隐身载体实体驱动经验条；点击回传按配方 netId 精确定位条目（`offerIndex` / `recipeNetId`），另有与参考实现 GMLIB ChestUI 同语义的逐动作回调（`src` / `dst` / `amount`，关闭时以 `slot=-1, amount=-1` 哨兵收尾）。**（本条的点击回传已在 26.40.4 按需求撤回 —— 详见上一条。）**两条路径由 `usePacketOffers` 选：**默认纯协议层**（服务端不放交易表 → 天然只读），置 `false` 走真实交易表 + BDS `openTrading`（成交真的换物品）。载体类型 `carrierIdentifier` 可选村民或流浪商人。
  - **NPC 对话框 `INpcDialogue`**：`NpcDialoguePacket` + 合成 `minecraft:npc` 载体（放在世界下方 → 客户端看不到实体，但对话框头像照常渲染）；按钮 / 关闭回传，多层级对话按场景名路由。
  - **虚拟容器 `IContainerMenu`**：逐条复刻参考实现 GMLIB 的 `ChestUI` —— 客户端侧箱子方块（`UpdateBlock`）+ 方块实体 NBT（`BlockActorData`，物品走 `Items`、标题走 `CustomName`）+ `ContainerOpen`（类型 `Container(0)`、绑方块坐标、目标实体 `-1`）；`rows=3` 小容器 27 格、`rows=6` 大容器 54 格（`pairx` / `pairz` / `pairlead` 配对）；点击回传槽位号 = 条目下标，关闭回传 `closed=true`。**任何输入设备都能点**（点击就是一次物品拾取）。
  - **感知域 `IPlayerSensing`**：挂钩 `PlayerAuthInputPacket` 逐包捕获 `InputMode` → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`；玩家离线即清，换设备下一包即更新。
  - **逐客户端渲染（千人千面）**：`ICustomEntity` 新增 `setPlayerNametag` / `setPlayerScale` / `setPlayerEquipmentSlot` / `clearPlayerAppearance`（出生包与增量包都按观看者覆盖值下发，装备变更即时单发无闪烁）；`IHologramText` 文本含 `{var}` 时自动切换为**逐观看者形状**，`{player}` 从此对全员绘制也逐人正确。
  - **裁剪**：移除逐客户端音效与短命飘字（讨论后不再需要，代码整体删除；两者是早期按"协议层手写包"做的实验）。
  - **修复**：①文本动画（滚动 / 弹跳）改由库内自驱（`ServerLevelTickEvent` 监听）—— 此前 `tick` 只有 LSE 导出，原生侧没有驱动源，动画既不动也不消失（**该自驱与相关 API 已在 26.40.8 收缩中整体移除**）；②147 钩子原先只在**交易**菜单打开时才进入，虚拟容器点击因此全部漏掉；③点击上报的是背包侧槽位（现在优先报交易侧）；④`src/DiagLog.h` 遗留在发布产物里的诊断开关（`check-no-diagnostics.bat` 会挡住）。
  - **实测沉淀（详见下文各节）**：容器打开延迟 7 tick 可用 / 6 tick 不可用的下限；物品请求有 `ItemStackRequestPacket(147)` 与 `PlayerAuthInputPacket(144)` 内嵌两条通道；命中虚拟容器的动作只回传不拦（自己代答失败应答会让客户端弹错误提示）。
- `26.40.2`（API 1.20.0）：修复假玩家 NPC 皮肤不渲染（客户端回退默认模型）—— PlayerList 皮肤条目的 `Id` / `FullId` 为空或残留原玩家的缓存键；PNG 注册、在线采集、blob 恢复统一走 `finalizeSkinIds` 补全 `Id` 并重建 `FullId`，且在 Persona / 采集改写完成后才设置，不再沿用来源玩家的身份。PlayerList 发送前校验 2168 包体前缀（Add `01 01 00`，Remove `01 00 01`），前缀异常时丢弃并记录实际前缀；首次提交到 NetworkPeer 时输出 `PlayerList submitted` 日志。新增离线回归检查 `tests/check-npc-playerlist.ps1`（用实际链接的静态库产生字节 + 独立 Python 解码器核对 2168 字段、可信标记位置、`Id` / `FullId` 与包体完全消费）。配套 Protocol 静态库完成 PlayerList 2168 帧格式移植（variant 数组、`ActionType` Add=0/Remove=1、可信标记改为皮肤内三态字符串）；本插件 ABI / API 版本不变
- `26.40.1`（API 1.20.0）：适配 LeviLamina 26.40 / BDS 1.26.40（协议 2168，形状渲染改用 Protocol v2168 静态库）；修复事件 ID 与官方 LeviLamina 不一致导致监听器全部收不到事件（`src/EventIdCompat.h`）；新增逐客户端朝向（`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations`，实体与 NPC 通用）与轻量朝向更新（`setRotationLight`）；新增 ghost 交互多播监听（1.19.1）；NPC 创建/脏刷新合并到 tick 末尾统一发包（同一 tick 内多次下发会让客户端收到密集"新玩家"而断线）；不再下发 PlayerList 移除（客户端在皮肤条目仍活跃时移除该条目会崩，实体照常消失）；PlayerList / AddPlayer 发送跳过 BDS 回读校验
- `26.10.7`（API 1.19.0）：新增皮肤从内存导出（`getSkinBlob`/`registerSkinFromBlob`，消费方自行持久化）；目录批量导入皮肤（一个子文件夹 = PNG + 可选 `.json` 模型）；`PlayerNpcSkin.geometryData` 自定义几何模型；库移除磁盘存储，改为纯 API；新增 NPC 缩放（`PlayerNpcConfig.scale` + `playerNpcSetScale`，0.0625~10，碰撞箱等比）；修复 NPC 视距裁剪/脏刷新/Tab 移除失效（tick hook 未注册）；修复 NPC 重生（缩放/换肤等脏刷新）皮肤丢失变默认史蒂夫（过期 Tab 移除条目误删新皮肤条目）
- `26.10.6`（API 1.17.1）：皮肤采集永久存储修复
