# Endstone 版

HologramLib 的 endstone 版本：与 LeviLamina 版**共用同一份协议编码**（仓库 `core/`），
区别只在外壳 —— LL 版是 C++ 插件，endstone 版是 Python 插件 + `holo_core.dll`。

本节按域讲解 endstone 的用法，写作标准与 [LSE 脚本接口](../lse/index.md) 一致：
先说这个域能做什么、有哪些限制，再给能直接跑的示例，最后每个方法一节。

## 安装

```bat
:: 仓库根目录；--deploy 直接装到服务端
build.cmd --deploy E:\some_server\plugins
```

产物是 `dist/endstone_hologramlib-<版本>-py3-none-win_amd64.whl`，内嵌 `holo_core.dll` 与物品 id 表。

!!! warning "同版本不会重装"

    endstone 把插件装到 `plugins/.local/Lib/site-packages/`，**同版本号不会覆盖**；
    升级前先删掉那里的 `hologramlib_endstone` 目录（或提升版本号）。

启用日志：

```
[Hologramlib] HologramLib 已启用 (endstone 版, core 26.60.1, PrimitiveShapes id=328)
```

## 第一个功能

```python
from endstone.plugin import Plugin


class MyPlugin(Plugin):
    depend = ["HologramLib"]

    def on_enable(self) -> None:
        lib = self.server.plugin_manager.get_plugin("hologramlib")
        lib.api.create_text(0, 100, 0, "hello")
        hid = lib.holograms.create(0, 101, 0)
        lib.holograms.add_line(hid, "在线 {online}")
        lib.holograms.draw(hid)
```

## 命名规则

方法名 = LSE 导出名去掉域前缀后转 `snake_case`，域前缀对应插件上的属性：

| LSE 导出 | endstone 写法 |
|---|---|
| `holoCreate` | `lib.holograms.create(...)` |
| `shapeCreateText` | `lib.api.create_text(...)` |
| `particleCreatePoint` | `lib.particles.create_point(...)` |
| `itemDisplayCreate` | `lib.itemdisplay.create(...)` |
| `entityCreate` | `lib.entities.create(...)` |
| `playerNpcCreate` | `lib.playernpc.create(...)` |
| `viewBlock` | `lib.view.view_block(...)` |
| `playerHideSetHidden` | `lib.visibility.set_hidden(...)` |

全部 444 条的完整签名见 [Endstone API 索引](api.md)。

## 与 LeviLamina 版的关系

| | LeviLamina | Endstone |
|---|---|---|
| 插件形态 | C++ DLL | Python wheel + `holo_core.dll` |
| 调用方式 | C++ 接口 / LSE `ll.import` | Python 属性 |
| 协议编码 | `core/` 编进 DLL | `core/` 编成 `holo_core.dll` |
| 事件 | LL 事件总线 + Hook | `PacketSendEvent` / `PacketReceiveEvent` |
| 调度 | LL tick 驱动 | `server.scheduler`（20 tick/s） |

两边**字节同源**：同一个形状、同一个悬浮字，在两个平台发出的包体完全一致。

## 查 API 版本

```python
api = lib.holo_api_version          # "1.35.0"
major, minor, patch = (int(x) for x in api.split("."))
if (major, minor) >= (1, 36):
    lib.fakeblock.set_many_near(...)  # 按 hasattr 判断是否可用
else:
    lib.logger.info(f"版本 {api} 不支持批量靠近切换，跳过")
```

`lib.holo_api_version` 是字符串；LSE 的 `holoApiVersion()` 是整数（`0x012900`），两边是同一个版本的不同表示。

## 按域指南

| 域 | 条目 |
|---|---|
| [形状](shape.md) | 46 |
| [悬浮字](hologram.md) | 28 |
| [渐变线](gradient.md) | 11 |
| [粒子](particles.md) | 27 |
| [Boss 条](bossbar.md) | 10 |
| [侧边栏](sidebar.md) | 7 |
| [HUD 显隐](hud.md) | 5 |
| [玩家表现](player-effect.md) | 5 |
| [Actor 表现](actor-effect.md) | 25 |
| [假方块](fake-block.md) | 5 |
| [表单](form.md) | 2 |
| [物品详情](item-detail.md) | 2 |
| [复活](respawn.md) | 3 |
| [掉落物](item-actor.md) | 9 |
| [物品展示](item-display.md) | 45 |
| [虚拟容器](containers.md) | 16 |
| [假背包](fake-inventory.md) | 10 |
| [自定义实体](entities.md) | 47 |
| [交易菜单](trade.md) | 12 |
| [NPC 对话](npc-dialog.md) | 10 |
| [硫磺方块](sulfur.md) | 10 |
| [玩家 NPC](player-npc.md) | 55 |
| [视角覆盖](view.md) | 19 |
| [客户端感知](sensing.md) | 3 |
| [可见性](visibility.md) | 5 |
| [幽灵交互](ghost.md) | 2 |
| [线路诊断](wire.md) | 9 |
| [杂项](misc.md) | 2 |
