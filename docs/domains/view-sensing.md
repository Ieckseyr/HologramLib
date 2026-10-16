# 能力域指南 · 视图覆盖与感知

覆盖 **客户端视图覆盖**（`IViewOverride`）、**感知域**（`IPlayerSensing`），并说明 **千人千面 / 逐玩家变量** 这两个「不是独立域」的能力。

| 域 | C++ | LSE | 一句话 |
|---|---|---|---|
| 客户端视图覆盖 | `IViewOverride` | `view*` · 18 | 按玩家拦截出站包，改**这名玩家看到的客户端视图** |
| 感知域 | `IPlayerSensing` | `sensing*` · 3 | 逐包捕获客户端设备类型 |
| 千人千面 | `ICustomEntity` 逐观看者覆盖 | `entity*` 尾部 · 4 | 同一实体按观看者覆盖外观 |
| 逐玩家变量全息 | `IHologramText` + `{var}` | — | 文本含 `{var}` 时按观看者解析 |

---

## 客户端视图覆盖

### 它到底改了什么

**只改客户端视图。** 服务端世界 / 存档 / 碰撞**一个字不动**，其余玩家看到原样。

对**被覆盖的那名玩家**来说：他看到的实体换了个样子。而这名玩家以外的所有人、以及服务端逻辑，看到的都是**真身**。
反过来说更重要 —— 如果覆盖的是「把玩家 A 变成一只生物」：

> **`runtimeId` / `uniqueId` 没变 → 服务端那边仍是那个真玩家，打他就是打他本人。**
> 伤害、掉落、其他插件看到的全是真身。

### 两条纪律（这是本域的设计红线）

**1. 只拦不发**
出站钩子**只决定「这一包发给这名玩家吗」（放行 / 丢弃）**，**从不修改引擎包对象里的字段**。
需要「改」的地方一律：**丢掉原包 + 库自己手写协议包补发**（sculk 构造 → 回读校验 → 原始字节发送）。

> 历史：曾用「改引擎包字段再让引擎序列化」的做法，已按此约定整体删除（`26.40.8` 清掉了 `dataListOf` / `setItemById` / `cloneItems` / `restoreItems` 等就地改写死代码，判定链路全 `const` 化）。

**2. 数据只来自协议包**
位置 / 朝向 / 输入位来自入站 `PlayerAuthInput`；出生包类型 / 位置 / 名字来自出生包载荷；动作来自 `Animate` / `ActorEvent`。
**不读服务端实体状态**（不碰 `getAABB` / `fetchEntity` / `Player::getInventory` 之类）。

> 唯一例外是**身份识别**（网络标识 → 玩家、`AddPlayer` 里反查 `uniqueId`）—— 那是「认人」，不是读状态。
>
> **读引擎状态的功能项不进库**：背包镜像（读玩家背包 + 开容器界面 + 按 tick 刷新）曾短暂进过库，**已整体摘除**。消费方要用请直接用**容器域**的协议能力自己拼（`IContainerMenu::open` + 按槽 `setItem`），读谁的背包、多久刷一次由消费方决定。

### 钩子挂在哪（一个踩过的坑）

挂在 BDS 的**按收件人发包汇合点**：`NetworkSystem::send` / `sendToMultiple`。

> 反编译实测：**实体出生包不走 `LoopbackPacketSender`**（原先挂在那里 → 换类型不生效），故改挂这里。

**心跳兜底**：`Level::$tick` 每 tick 自查一次 —— 实体「重新进入视野」、玩家换区块时把覆盖重新推一遍。出生包拦漏 / 方块覆盖被区块重发冲掉都靠它恢复。没用到这个功能时**只是一次原子读**。

### 能做什么

| 能做的 | 怎么做 |
|---|---|
| **实体换类型** | 丢掉原出生包，库用**同一 `runtimeId` / `uniqueId`** 自己发一只目标类型，其余字段照抄原包 |
| **换名字牌** | 元数据 `Name(4)` / `NametagAlwaysShow(81)`：原包照发，库自己的 `SetActorData` **紧随其后压过去**（服务端后续改名也压得住） |
| **对该玩家隐藏** | 出生包与更新包不发给他；已在客户端上的补一条 `RemoveActor` |
| **同 id 换模型**（`skinId`，`26.40.9`） | 吃掉该实体出生包 → 发 `PlayerList(Add)` + `AddPlayer`（玩家模型 + 已注册皮肤） |
| **玩家变生物 = 替换** | 吃掉他的出生包，用同一 `runtimeId` / `uniqueId` 发一只该类型实体；`MovePlayer` 对该观看者吃掉，位置由 `MoveActorAbsolute` 推 |
| **玩家换皮肤** | 只给该观看者重发一条 `PlayerList(Add)`（该实体自己的 UUID + 源玩家皮肤），与原版「皮肤更新」同机制、**可立刻撤销** |
| **掉落物换类型 / 隐藏**（`26.40.10`） | 掉落物走 `AddItemActor`(id 15) 而不是 `AddActor` —— 两条都拦 |
| **方块显示成另一种方块** | 丢掉原 `UpdateBlock` / `UpdateBlockSynced`，库自己发一条带覆盖方块网络 id 的 `UpdateBlock`；服务端发的本来就是覆盖方块时照发不改。区块重发后由库自动补发 |

### 26.40 做不到的（所以没进 API）

**逐玩家的缩放 / 发光 / 隐身** —— 26.40 元数据表里没有 `scale`，也没有 `glowing` / `invisible` 旗标（基岩版隐身是**效果**，走 `MobEffectPacket`）。

自定义实体的逐客户端缩放 / 装备槽仍归 `ICustomEntity` 的 per-viewer 覆盖（`1.21.0`），不要指望视图覆盖域。

### 逐玩家隔离 / 幂等 / 零开销 / fail-safe

| 性质 | 做法 |
|---|---|
| **逐玩家隔离** | 覆盖表按玩家名 + 一份全局表；多收件人路径（`sendToClients` / 广播）展开成**逐收件人套用**。因为从不改字段，不存在「后一个收件人看到前一个覆盖」的串味 |
| **幂等** | 判定**认 id 不认位置**，同一条包被多条路径各判一次结果相同；已是覆盖方块的更新包直接放行 |
| **零开销** | 库内没有任何覆盖时，钩子**一条分支直接放行**（不查玩家、不读包）；命中判定也只看目标玩家有没有覆盖 |
| **fail-safe** | 不认识的包、认不出来的实体**一律原样放行** |

### 同 id 换模型（`EntityView::skinId`）

`26.40.9` 起，`skinId` 让**非玩家实体**换成「我们的模型」：客户端看到已注册皮肤（可自定义几何）的玩家模型，**服务端那边还是那只生物**。
配套：

- 该实体的 `MoveActor*`(18/111) 对这些观看者**吃掉**，位置 / 朝向由每 tick 心跳用 `MoveActorAbsolute` 推（复用 PlayerNpc 的轻推通道）；
- `PlayerList` 条目约 **20 tick 后摘掉**（不进 Tab，实体保留）；
- **撤销 = `clearEntity` 重发真实出生包**；
- `scale` 等比碰撞箱（元数据 53/54）、`yOffset` 把模型原点对到脚位；
- 皮肤来自 `IPlayerNpc` 注册表（`registerSkin` / `captureSkin` / `registerSkinFromBlob`）。

> **这是「给生物换自定义模型」的正路。** PlayerNpc 载体那套要「隐形真身 + 逐 tick 跟随 + ghost 转发攻击」，这条都不需要。

### 出生包配方（线上验证过的那一套）

自建出生包**必须**带：

1. flags 元数据项；
2. `minecraft:health` / `minecraft:scale` **属性**；
3. 且**后面紧跟一条 `UpdateAttributesPacket`** —— 客户端对出生包里的属性**只初始化不生效**。

> 之前缺这几项，玩家进服时心跳一发重发就会让客户端报错（表现为「第一次进出错」）。
> 另修正了 `NametagAlwaysShow` 的**协议值反转**（`0` = 常显），并给刚进服的玩家约 **2 秒宽限期**（客户端还在收区块时不塞实体包）。
>
> `26.40.10` 又补了一条同源问题：`SetActorDataPacket` / `UpdateAttributesPacket` 在 26.40 线格式里**都带 `mTick`**，而视图覆盖这条路一直没填（`= 0`）→ 客户端把「元数据更新 / 属性更新」当**过期包丢掉**（抓包证明：出生包里 `id2`/`id43`/`id3`/`id82` 都在且值正确，画面却仍是默认外观）。
> 现在这两个包统一填 `currentViewTick()`（`tickPulse` 每拍同步），出生包之后**紧跟一条同计划的 `SetActorData`**，再排一条 **+2 tick** 的确认（`scheduleMetaPush` / `flushMetaPushes`）。
> 对比：customentity 域一直带着 tick 发（`sadp.mTick = tick`），所以那边 `setVariant` / `setColorIndex` 一直是好的。

### 自己的客户端不参与自己实体的覆盖

> **玩家变生物会把本人客户端卡死**（覆盖对「自己」也生效：给本人发 `RemoveActor` + 同 runtimeId 的 `AddActor`）。
> 现在规则是：**自己的客户端不参与自己实体的覆盖** —— 覆盖 / 恢复 / 出站三条链路全部跳过本人（自己的 `SetActorData`、`MovePlayer` 也不再被吃）。
>
> **换肤类例外：含本人。** `PlayerList(Add)` 发给本人正是「**自己看自己被换肤**」的正路，自视也随之变。

### 实体类改动都是「立刻生效」

换类型 / 隐藏 / 撤销都是对那只实体做一次「**移除 + 按目标状态重发一只**」（同一个 `runtimeId` 与 `uniqueId`，服务端后续的移动 / 状态更新照常作用在它身上）。

代价：**客户端那只实体会被重建** —— 只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步。
撤销时**离得太远（> 64 格）或已消失的实体不重建**；**玩家实体无法重建**（`AddPlayer` 没有类型字段）。

### 判定逻辑是独立可测的

判定逻辑独立在 `src/view/ViewOverrideLogic.h`（坐标打包 / 区块归属 / 幂等判断 / 名字牌计划 / 类型名清洗 / `planItemSpawn`），**不碰 BDS 类型**，所以能离线跑：

```powershell
./tests/check-view-override.bat    # 59 项
```

### LSE 侧：`view*` 的 18 个函数

| 分组 | 函数 |
|---|---|
| 主入口 | `viewEntity` · `viewBlock` · `viewClearEntity` · `viewClearBlock` · `viewClearAll` |
| 逐字段 | `viewTargetType` · `viewClearId` · `viewMetaProbe` · `viewDescribe` · `viewEcho` |
| 「自己」一族 | `viewSelf` · `viewSelfClear` · `viewSelfType` · `viewSelfSkin` · `viewSelfName` · `viewSelfHide` |
| 查询 | `viewUniqueIdOf` |
| 能力协商 | `viewItemSpawnHooked` |

`viewEntity` 的 spec 串支持这些键：

```
type=   name=   skin=   pose=   scale=   variant=   mark=   color=   color2=
```

`pose` / `scale` / `variant` / `mark` / `color` / `color2` 是 `26.40.10` 补的（对应 `EntityView` 的 `pose` / `metaScale` / `variant` / `markVariant` / `colorIndex` / `color2`，各带 `hasXxx`，`false` = 该项不动）。

> ⚠ **`viewEntity` 的「唯一 id 判定」在 `26.40.8` 修过一处**：**玩家 id 本身是负数** —— 旧判定会把玩家 id 当成无效值。若你按旧行为绕行过，可以去掉绕行。
>
> ⚠ **`EntityView` 大小在 `26.40.10` 变了**（尾部追加了上面六个字段）→ **直接构造该结构体的 C++ 消费方要重新编译**。C++ 接口本身只做尾部追加，虚表槽位不变。

---

## 感知域

```cpp
if (lib.playerSensing().isTouch("Steve")) {
    lib.containerMenus().open("Steve", menu);   // 触屏：用虚拟容器，点击必然发包
}
```

挂钩 `PlayerAuthInputPacket` **逐包**读 `InputMode` → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`。

| 行为 | 说明 |
|---|---|
| `Unknown` 不覆盖已知值 | AuthInput 早段可能报 `Undefined`，不能让它把已知设备抹成未知 |
| 玩家离线即清 | — |
| 换设备**下一包**即更新 | 没有缓存延迟 |

**没有事件，不用轮询** —— LSE 侧直接查询：`sensingDeviceOf` / `sensingIsTouch` / `sensingDeviceCode`。

**典型用法是按设备分流界面**：触屏走虚拟容器（点击必然发包），键鼠走交易菜单（完整交互）。

---

## 千人千面（逐客户端渲染）

不是独立域，而是 `ICustomEntity` 上的逐观看者覆盖：

```cpp
lib.customEntities().setPlayerNametag(entityId, "Steve", "§c队长");
lib.customEntities().setPlayerScale(entityId, "Steve", 1.5f);
lib.customEntities().setPlayerEquipmentSlot(entityId, "Steve", 0, item);
lib.customEntities().clearPlayerAppearance(entityId, "Steve");
```

出生包与增量包**都按观看者覆盖值下发**，装备变更**即时单发、无闪烁**。

LSE 侧是同名的四个导出：`entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`。

> **是逐玩家发包，适合少数人场景**（队长视角、任务追踪），**不适合全服广播式的高频刷新** —— 发包量随观看者数线性增长。

### 逐玩家变量全息

`IHologramText` 的文本一旦含 `{var}`，库就改为**维护每个观看者一份形状**，各自收到按自己解析的结果（观看者离线 / 离开维度时那份形状就地销毁）。

```cpp
lib.holograms().addLine(holo, "§b{player}§r 你好");
```

代价与适用场景同「千人千面」：**形状数 N 倍、重发复杂度 N 倍**。详见 [渲染与文本 · 逐玩家变量](rendering.md#逐玩家变量千人千面)。

---

## 相关

- 完整签名：[C++ 接口参考](../reference/cpp-api.md)（§1.13 感知域 / 千人千面 / 逐玩家变量、§1.16 视图覆盖）
- LSE 函数清单：[LSE 接口参考](../reference/lse-api.md)（§2.13 / §2.16）
- 边界纪律（只拦不发 / 数据只来自协议包）的精简版：[README · 设计说明](https://github.com/Ieckseyr/HologramLib/blob/main/README.md#设计说明)
- 发包原语与回读校验：[协议层实现笔记](../architecture/protocol-notes.md)
