# HologramLib for Endstone

Endstone 侧的 HologramLib：与 LeviLamina 版共用同一份协议编码（仓库根 `core/`），
协议字节由 `holo_core.dll` 产出，Python 只负责状态与调度。

## 装法

```bat
:: 仓库根目录
build.cmd --deploy <bedrock_server>\plugins
```

或者分两步：

```bat
xmake build holo_core                      &:: 编出 holo_core.dll 并部署到本包 _native/
python tools/pack_endstone.py --deploy <bedrock_server>\plugins
```

产物：`endstone_hologramlib-<version>-py3-none-win_amd64.whl`，内嵌 `holo_core.dll` 与物品 id 表。

> 同一版本重复部署不会重装（endstone 会 pip 装到 `plugins/.local/Lib/site-packages/`），
> 升级前先删掉那里的 `hologramlib_endstone` 目录。

## 用它的 API

```python
from endstone.plugin import Plugin

class MyPlugin(Plugin):
    depend = ["HologramLib"]

    def on_enable(self):
        lib = self.server.plugin_manager.get_plugin("hologramlib")
        lib.api.create_text(0, 100, 0, "hello")                 # 形状域
        hid = lib.holograms.create(0, 101, 0)                   # 悬浮字域
        lib.holograms.add_line(hid, "在线 {online} · 你是 {player}")
        lib.holograms.draw(hid)
        lib.gradients.create(0, 99, 0, 5, 99, 5, 24)            # 渐变线
        lib.particles.create_point("Steve", 0, 0, 99, 0)        # 粒子
        lib.itemdisplay.create(0, 100, 0, 0, "minecraft:diamond")  # 物品展示
        lib.entities.create("minecraft:villager_v2", 0, 100, 0, 0) # 自定义实体
```

调试命令：`/holo demo`（形状 + 悬浮字 + 渐变线 + 粒子 + Boss 条 + 侧边栏 + HUD + 玩家效果）、`/holo clear`、`/holo stats`。

## 已覆盖

形状 46 · 悬浮字 28 · 渐变线 11 · 粒子 27 · Boss 条 8 · 侧边栏 7 · HUD 5 · 玩家表现 5 · 定位条 14 ·
假方块 5 · 物品实体 9 · 硫磺方块 10 · 线路诊断 8 · 表单 2 · 物品详情 2 · 复活 3 ·
物品展示 37/45 · 自定义实体 41/47 · 玩家 NPC 38/55 · 皮肤注册表 10/14 · 虚拟容器 12/16 ·
假背包 7/10 · 交易 10/12 · NPC 对话 6/10 · Actor 表现 18/23

未覆盖项与原因见仓库 `docs/architecture/overview.md#endstone-端状态`。

## 自测

```bat
python platform/endstone/tests/test_shape_api.py
```

不需要服务器：用假 Player / 假 Server 驱动各域 API，断言发出的包 id、包内容与状态变化（当前 505 项）。

## 目录

```
src/hologramlib_endstone/
├── native.py        holo_core.dll 的 ctypes 绑定（全部 C ABI 都在这里）
├── plugin.py        插件入口、生命周期、/holo 命令、出站包过滤
├── shapes.py        形状表与形状构造
├── shape_api.py     形状域（46 导出）
├── holograms.py     悬浮字域（28）
├── gradients.py     渐变线（11）
├── particles.py     粒子（27）
├── bossbar.py       Boss 条（8）
├── sidebar.py       侧边栏（7）
├── hud.py           HUD（5）
├── playereffect.py  玩家药水/动画（5）
├── actoreffect.py   Actor 表现 + 出站事件过滤（18）
├── locatorbar.py    定位条（14）
├── fakeblock.py     假方块（5）
├── forms.py         表单关闭（2）
├── itemdetail.py    物品详情（2）
├── playerrespawn.py 复活（3）
├── itemactor.py     物品实体（9）
├── itemdisplay.py   物品展示（37）
├── container.py     虚拟容器（12）
├── fakeinv.py       假背包（7）
├── entities.py      自定义实体（41）
├── trade.py         交易（10）
├── npcdialog.py     NPC 对话（6）
├── sulfur.py        硫磺方块（10）
├── wire.py          线路诊断（8）
├── skins.py         皮肤注册表 + PNG 解码（10）
├── playernpc.py     玩家形态 NPC（38）
├── items.py         物品名 → 数字 id 查表
└── data/item_ids.json   由 tools/gen_item_ids.py 生成
```
