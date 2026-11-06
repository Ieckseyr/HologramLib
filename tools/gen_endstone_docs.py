"""生成 endstone 文档站：按域的分页（docs/endstone/*.md）+ 总览 + 444 条索引。

风格对齐 docs/lse/*：先讲这个域能做什么、有什么坑，给一段能直接跑的「快速上手」，
然后每个方法一节（签名 / 参数 / 返回 / 对应 LSE 导出 / 说明）。

方法签名从源码 inspect 取，不手抄；域简介与示例是本文件里的手写部分。

用法：
    python tools/gen_endstone_docs.py            # 生成
    python tools/gen_endstone_docs.py --check    # 只校验
"""

from __future__ import annotations

import importlib
import inspect
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "platform" / "endstone" / "src"
OUT = ROOT / "docs" / "endstone"

# group -> (slug, 中文标题, 模块, 类名, 插件属性, 简介, 快速上手代码)
DOMAINS = [
    (
        "Shape", "shape", "形状",
        "shape_api", "ShapeApi", "api",
        "形状域用 `PrimitiveShapes`（包 id 328）一次下发一组几何体，客户端自己画。"
        "悬浮的线框、球、圆、箭头、文字都走这里，不生成实体、不写存档。\n\n"
        "每个形状有独立 id，改位置/颜色/缩放后要再 `draw()` 一次才会重新下发；"
        "`draw_to_player()` 只发给一个人，`draw_to_dimension()` 发给整个维度。",
        """```python
lib = self.server.plugin_manager.get_plugin("hologramlib")

sid = lib.api.create_box(0, 64, 0, 3, 67, 3)   # 一个 3×3×3 的线框
lib.api.set_color(sid, 0.2, 1.0, 0.4)          # 换颜色
lib.api.draw(sid)                              # 下发

text_id = lib.api.create_text(0, 70, 0, "§eHello")
lib.api.draw(text_id)
```""",
    ),
    (
        "Hologram", "hologram", "悬浮字",
        "holograms", "HologramApi", "holograms",
        "多行悬浮字：每行独立设置文本、颜色、轮播池与变量解析。"
        "内部按 4 Hz 重下发改动过的行，只有内容真的变了才发包。\n\n"
        "`{online}` / `{player}` 这类变量要开启 `set_line_parse_variables`；"
        "轮播用 `set_line_pool(index, [..], interval_ms)` 配刷新间隔。",
        """```python
hid = lib.holograms.create(0, 101, 0)
lib.holograms.set_scale(hid, 1.2)
lib.holograms.add_line(hid, "§6在线 §f{online}")
lib.holograms.set_line_parse_variables(hid, 0, True)
lib.holograms.add_line(hid, "§7每 2 秒轮播")
lib.holograms.set_line_pool(hid, 1, ["§aA", "§bB", "§cC"], 2000)
lib.holograms.draw(hid)
```""",
    ),
    (
        "GradientLine", "gradient", "渐变线",
        "gradients", "GradientApi", "gradients",
        "一条线按两端颜色插值出 N 段，用粒子或形状画出来，适合做光柱、路径指示、"
        "传送门边框这类「有方向感」的提示。\n\n"
        "颜色是 RGB 0~1 的三元组，支持首尾不同色；密度决定每段的采样点数。",
        """```python
gid = lib.gradients.create(0, 64, 0, 0, 70, 0, 24, (1.0, 0.3, 0.1), (0.2, 0.6, 1.0))
lib.gradients.draw(gid)
```""",
    ),
    (
        "Particle", "particles", "粒子",
        "particles", "ParticleApi", "particles",
        "用原版粒子效果拼形状：点、线、面、盒子、盒子六面、多边形。"
        "每个粒子组可以设间隔（多少 tick 喷一次）、生命周期、可见玩家。\n\n"
        "粒子按玩家下发，所以 `create_*` 的第一个参数是玩家名；"
        "`set_visible_players` 会接管之后发给谁。",
        """```python
lib.particles.create_point("Steve", 0, 64, 0, "minecraft:end_rod", 4, 100)
lib.particles.create_box("Steve", 0, 0, 64, 0, 2, 2, 2, 0.5, "minecraft:flame", 4, 100)
```""",
    ),
    (
        "BossBar", "bossbar", "Boss 条",
        "bossbar", "BossBarApi", "bossbar",
        "屏幕顶部的 Boss 血条，纯协议下发：标题、血量百分比、颜色、样式都能改。"
        "适合做「Boss 血量」「任务进度」「倒计时」。\n\n"
        "一个玩家可以同时有多条（`add_on` 会叠上去），也可以一次清空。",
        """```python
bid = lib.bossbar.add("Steve", "§c世界 Boss", 1.0)
lib.bossbar.set_health(bid, 0.5)
lib.bossbar.set_style(bid, 0, 1)   # 颜色 红, 样式 分段
```""",
    ),
    (
        "Sidebar", "sidebar", "侧边栏",
        "sidebar", "SidebarApi", "sidebar",
        "右侧计分板：标题 + 若干行，逐玩家下发。适合做在线列表、统计面板、任务进度。\n\n"
        "行的先后顺序由调用顺序决定；`set_line` 只改一行，`set` 整体替换。",
        """```python
lib.sidebar.set("Steve", "§e服务器状态", ["§7在线: §f{online}", "§7TPS: §a20.0"])
lib.sidebar.set_line("Steve", 0, "§7在线: §f3")
```""",
    ),
    (
        "HUD", "hud", "HUD 显隐",
        "hud", "HudApi", "hud",
        "HUD 元素显隐：血量、饥饿、坐标、十字准星等，按玩家单独控制，可以只隐藏某几项。\n\n"
        "`set_elements` 传要**保留**的元素列表，返回真正被隐藏的条目数；"
        "`set_raw` 直接把整张位图写下去，用来做实验或兼容新版本。",
        """```python
lib.hud.set_elements("Steve", ["hotbar", "crosshair"])   # 只留快捷栏和准星
lib.hud.reset_all("Steve")
```""",
    ),
    (
        "PlayerEffect", "player-effect", "玩家表现",
        "playereffect", "PlayerEffectApi", "playereffect",
        "给玩家播原版表现：村庄英雄、不祥之兆、庆祝（烟花）、不祥药水。"
        "这些不是药水效果，而是客户端动画/音效事件，不需要真的给效果。",
        """```python
lib.playereffect.show_village_hero("Steve", 5)
lib.playereffect.play_celebrate("Steve")
```""",
    ),
    (
        "ActorEffect", "actor-effect", "Actor 表现",
        "actoreffect", "ActorEffectApi", "actoreffect",
        "对任意 runtimeId 的实体播动画/事件：受击、死亡、冻结、图腾，还有那个给朋友的彩蛋 `easterEggCreeper`（⑨），"
        "也能按玩家或按实体**拦截**指定事件（出站包过滤）。\n\n"
        "彩蛋的来历见 [LSE 的「朋友的⑨」](../lse/easter-egg.md)：_昨日一去不回，噢耶耶，开心比什么都贵........_\n\n"
        "动画名用原版 `animation.*` 控制器名；执行拦截要调一次 "
        "`ensure_packet_filter`（插件启用时已自动挂）。",
        """```python
rid = lib.actoreffect.runtime_id_of_entity(my_entity_id)
lib.actoreffect.play_hurt(rid)
lib.actoreffect.block_event(3)          # 全局拦掉某个事件
lib.actoreffect.unblock_event(3)
```""",
    ),
    (
        "FakeBlock", "fake-block", "假方块",
        "fakeblock", "FakeBlockApi", "fakeblock",
        "只对某个玩家显示成别的方块：把石头「变成」钻石矿，服务端世界里还是石头。"
        "底层是 `UpdateBlock`（包 id 21），不写世界、不进存档。\n\n"
        "包体 = 坐标（x/y/z 各 zigzag varint）+ 方块网络 id（varint）+ flag(3) + layer(0)。"
        "26.60 开着 `block-network-ids-are-hashes`，网络 id 是哈希值（`minecraft:stone` → `2150698529`），"
        "由 `create_block_data` 解析，不要自己拼数字。\n\n"
        "方块名接受 `minecraft:` 前缀，也接受带状态的字面量（如 `minecraft:oak_stairs[upside_down_bit=true]`）。\n\n"
        "批量替换（`blockBatch*`，含靠近切换成硫磺立方体）目前只在 LeviLamina 端提供，endstone 端待接。",
        """```python
lib.fakeblock.set("Steve", 10, 64, 10, "minecraft:diamond_ore")
lib.fakeblock.clear("Steve", 10, 64, 10)
```""",
    ),
    (
        "Form", "form", "表单",
        "forms", "FormApi", "forms",
        "关掉玩家当前打开的表单。endstone 自带 `send_form`，本库只补「强制关闭」这一半，"
        "因为很多插件关不干净。",
        """```python
lib.forms.close("Steve")
```""",
    ),
    (
        "ItemDetail", "item-detail", "物品详情",
        "itemdetail", "ItemDetailApi", "itemdetail",
        "在世界上方显示一个「物品 + 名称 + Lore」的悬浮详情，"
        "走文本/形状下发，不生成掉落物。适合做商店货架、奖励预览。",
        """```python
lib.itemdetail.show(0, 66, 0, "minecraft:diamond", "§b钻石", "§7价值 64 绿宝石")
```""",
    ),
    (
        "PlayerRespawn", "respawn", "复活",
        "playerrespawn", "PlayerRespawnApi", "playerrespawn",
        "死亡后强制把玩家复活到指定位置，或拦住本次重生。"
        "自己做复活点/竞技场重生逻辑时用得上。",
        """```python
lib.playerrespawn.respawn("Steve", 0, 64, 0, 0)
```""",
    ),
    (
        "ItemActor", "item-actor", "掉落物",
        "itemactor", "ItemActorApi", "itemactor",
        "协议层掉落物：客户端看到一个真实的物品实体，会飘、会被「捡走」，"
        "但服务端没有实体、不占实体系统。适合做奖励展示、商店陈列。",
        """```python
aid = lib.itemactor.spawn("minecraft:diamond", 1, 0, 65, 0, 0)
lib.itemactor.set_motion(aid, 0.0, 0.1, 0.0)
lib.itemactor.move_to(aid, 0, 64, 0, 0)
```""",
    ),
    (
        "ItemDisplay", "item-display", "物品展示",
        "itemdisplay", "ItemDisplayApi", "itemdisplay",
        "3D 物品展示：把物品贴在空间里旋转、缩放、拉伸，支持跟随玩家、只对某人可见、"
        "发光（附魔光效）、自定义 NBT。适合做旋转展台、掉落物特效、UI 装饰。\n\n"
        "`set_*_live` 是「立即生效」版本，`tick()` 驱动旋转和跟随（插件已自动调用）。\n\n"
        "彩蛋版见 `easter_egg(displayId, durationTicks, rotate)` —— 朋友的⑨：_昨日一去不回，噢耶耶，开心比什么都贵........_",
        """```python
did = lib.itemdisplay.create(0, 66, 0, 0, "minecraft:diamond_sword")
lib.itemdisplay.set_scale(did, 2.0)
lib.itemdisplay.set_anim(did, 30.0, 1)      # 每 tick 绕 Y 轴 30°
lib.itemdisplay.set_glint(did, True)
```""",
    ),
    (
        "Container", "containers", "虚拟容器",
        "container", "ContainerApi", "container",
        "协议层容器界面：给玩家开一个「箱子/熔炉」界面，里面的内容完全由本库决定，"
        "不存在的方块、不落存档。适合做商店、背包扩展、展示柜。\n\n"
        "关闭时容器即销毁；点击回传要接 `ItemStackRequest`（当前受限，见总览）。",
        """```python
cid = lib.container.open("Steve", 3, "§8商店")
lib.container.set_slot(cid, 0, "minecraft:diamond", 1, 0, "§b钻石", "§7售价 10")
lib.container.set_interactive(cid, True)
```""",
    ),
    (
        "FakeInventory", "fake-inventory", "假背包",
        "fakeinv", "FakeInventoryApi", "fakeinv",
        "给玩家伪造一整套背包内容：主背包、护甲、副手、末影箱都能改。"
        "底层是 `InventoryContent`，玩家看到的是假数据，真实背包不变。",
        """```python
lib.fakeinv.apply("Steve", [("minecraft:diamond", 64), ("minecraft:emerald", 16)])
lib.fakeinv.reset("Steve")
```""",
    ),
    (
        "Entity", "entities", "自定义实体",
        "entities", "EntityApi", "entities",
        "协议层自定义实体：给客户端下发一个原版实体（村民、盔甲架、任何 `minecraft:` 实体），"
        "位置、朝向、缩放、名称牌、装备、变体、可见玩家都能改，服务端不占用实体系统。\n\n"
        "视觉上等价于真实体，但**没有 AI、不会动、不进碰撞箱**，也不会被其他插件当成实体处理。",
        """```python
eid = lib.entities.create_advanced("minecraft:villager_v2", 0, 64, 0, 0,
                                  yaw=90.0, scale=1.0, nametag="§e商人")
lib.entities.set_equipment_slot(eid, 0, "minecraft:emerald")
lib.entities.set_variant(eid, 3)
lib.entities.set_pose(eid, 1)          # 下蹲
```""",
    ),
    (
        "Trade", "trade", "交易菜单",
        "trade", "TradeApi", "trade",
        "交易界面：用隐形的村民当载体，把原版交易 UI 拉起来，买卖项、等级、经验全由本库决定。"
        "适合做商店、兑换台、NPC 交易。\n\n"
        "报价用 `add_offer` 追加；改完记得 `update`/`set_tier` 让客户端刷新。",
        """```python
tid = lib.trade.open("Steve", "Shop", 1, 0, 0, "")
lib.trade.add_offer(tid, "minecraft:emerald", 3, "", 0, "minecraft:diamond", 1, "§b钻石")
lib.trade.set_tier(tid, 2, 20)
lib.trade.close(tid)
```""",
    ),
    (
        "NpcDialog", "npc-dialog", "NPC 对话",
        "npcdialog", "NpcDialogApi", "npcdialog",
        "原版 NPC 对话框：标题、台词、若干按钮，按钮可以带命令。"
        "载体是对该玩家单独可见的隐身实体，关掉即销毁。\n\n"
        "按钮规格：`标签|actionId|mode|命令1,命令2;第二个按钮|...`。",
        """```python
did = lib.npcdialog.open("Steve", "§e商人", "shop", "要点什么？",
                        "买钻石|buy|0|shop buy diamond;离开|bye|0|")
lib.npcdialog.update(did, "还有别的吗？", "买绿宝石|buy2|0|shop buy emerald")
lib.npcdialog.close(did)
```""",
    ),
    (
        "Sulfur", "sulfur", "硫磺方块",
        "sulfur", "SulfurApi", "sulfur",
        "`minecraft:sulfur_cube` 展示：一个方块变成的立方体生物，可以隐形、缩放、"
        "主手换成任意方块。适合做装饰、任务目标、活动道具。",
        """```python
sid = lib.sulfur.create(0, 64, 0, 0, "minecraft:diamond_block")
lib.sulfur.set_invisible(sid, False)
lib.sulfur.set_scale(sid, 2.0)
```""",
    ),
    (
        "PlayerNpc", "player-npc", "玩家 NPC",
        "playernpc", "PlayerNpcApi", "playernpc",
        "纯协议假玩家：客户端看到一个完整玩家模型，有名字牌、进 Tab 列表，"
        "但服务端没有这个人。适合商店老板、任务发布人、雕像。\n\n"
        "**皮肤必须先注册再创建**（endstone 拿不到 `Image`，只能从 PNG 解），"
        "外观改动（皮肤/缩放/名字）走「拆了重建」，会重发 PlayerList + AddPlayer。",
        """```python
lib.playernpc.register_skin("E:/server/skins/girl.png", "girl", "", 0)
nid = lib.playernpc.create(0, 64, 0, 0, "§e小美", "girl")
lib.playernpc.set_nametag(nid, "§e小美 §7<店员>")
lib.playernpc.play_animation(nid, "animation.player.attack.positions", "", 20)
lib.playernpc.set_ride_entity(nid, some_entity_id)
```""",
    ),
    (
        "View", "view", "视角覆盖",
        "view", "ViewApi", "view",
        "「一个人眼里的世界」：让某个方块只对某人显示成别的方块、让某个实体对某人不可见。"
        "方块直接发 `UpdateBlock`；实体隐藏靠取消出站包（按 runtimeId 命中）。\n\n"
        "改类型/皮肤/名字这类**改写**需要出站解码器，当前只做了隐藏与方块。",
        """```python
lib.view.view_block("Steve", 10, 64, 10, "minecraft:gold_block")
lib.view.view_self_hide("Steve", True)        # Steve 对别人隐身
lib.view.view_clear_all("Steve")
```""",
    ),
    (
        "Sensing", "sensing", "客户端感知",
        "sensing", "SensingApi", "sensing",
        "读客户端输入设备（键鼠/触屏/手柄）。endstone 的 Python 面没有 AuthInput 入口，"
        "当前三个函数返回空值，接口保留以便后续补齐。",
        """```python
is_touch = lib.sensing.is_touch("Steve")   # 目前恒为 False
```""",
    ),
    (
        "Visibility", "visibility", "可见性",
        "visibility", "VisibilityApi", "visibility",
        "让某个真实玩家对其他人不可见（隐身），可控制作用范围（生成/Tab/动作/状态/皮肤），"
        "恢复时补发 `AddPlayer` 重建实体。\n\n"
        "底层与视角覆盖共用一套出站过滤，所以两者会叠加。",
        """```python
lib.visibility.set_hidden("Steve", True)
lib.visibility.set_mask("Steve", spawn=True, tab=True, motion=True, state=True, skin=True)
lib.visibility.restore("Steve")
```""",
    ),
    (
        "Ghost", "ghost", "幽灵交互",
        "ghost", "GhostApi", "ghost",
        "轮询「玩家点了幽灵方块/幽灵实体」的交互事件。需要解析入站交互包，"
        "当前恒为空列表，接口保留。",
        """```python
for item in lib.ghost.poll_interactions():
    self.logger.info(item)
```""",
    ),
    (
        "Wire", "wire", "线路诊断",
        "wire", "WireApi", "wire",
        "抓出站包看「到底发了什么」：开始录制后按包 id、长度、收件人记一条，"
        "可以打标记、导出最近 64 条。排查客户端没反应、包被谁吃掉时最有用。\n\n"
        "只在 `start()` 之后才挂监听，不抓包时没有额外开销。",
        """```python
lib.wire.start("debug")
lib.wire.mark("开菜单前")
lib.bossbar.add("Steve", "test", 1.0)
print(lib.wire.dump())
lib.wire.stop()
```""",
    ),
    (
        "Misc", "misc", "杂项",
        None, None, None,
        "跨域的小工具：锚点（拿一个唯一的实体号）、假方块批量、Boss 条别名、"
        "线路诊断（出站包抓取）。",
        """```python
uid = lib.anchor.acquire("Steve")      # 拿一个唯一实体号当锚点
lib.anchor.release(uid)

lib.wire.start("debug")                # 开始抓出站包
lib.wire.mark("阶段 A")
print(lib.wire.dump())
lib.wire.stop()
```""",
    ),
]

def effective_group(export: dict) -> str:
    """Misc 组里按名字前缀重新归类（导出清单把这些都塞在 Misc 里）。"""
    group = export["group"]
    if group != "Misc":
        return group
    name = export["name"]
    for prefix, target in (
        ("bossBar", "BossBar"),
        ("hud", "HUD"),
        ("sidebar", "Sidebar"),
        ("player", "PlayerEffect"),
        ("blockFake", "FakeBlock"),
        ("easterEgg", "ActorEffect"),
        ("wire", "Wire"),
        ("anchor", "Misc"),
    ):
        if name.startswith(prefix):
            return target
    return "Misc"


TITLES_EN = {
    "Shape": "Shapes", "Hologram": "Holograms", "GradientLine": "Gradient lines",
    "Particle": "Particles", "BossBar": "Boss bars", "Sidebar": "Sidebar", "HUD": "HUD",
    "PlayerEffect": "Player effects", "ActorEffect": "Actor effects", "LocatorBar": "Locator bar",
    "FakeBlock": "Fake blocks", "Form": "Forms", "ItemDetail": "Item detail",
    "PlayerRespawn": "Respawn", "ItemActor": "Item actors", "ItemDisplay": "Item displays",
    "Container": "Virtual containers", "FakeInventory": "Fake inventory", "Entity": "Custom entities",
    "Trade": "Trade", "NpcDialog": "NPC dialog", "Sulfur": "Sulfur cube", "PlayerNpc": "Player NPCs",
    "View": "View override", "Sensing": "Client sensing", "Visibility": "Visibility",
    "Ghost": "Ghost interaction", "Misc": "Misc",
}

# group -> (模块, 类名, 属性) —— Misc 特殊：跨多个类
MODULE_OF = {
    "Shape": ("shape_api", "ShapeApi", "api"),
    "Hologram": ("holograms", "HologramApi", "holograms"),
    "GradientLine": ("gradients", "GradientApi", "gradients"),
    "Particle": ("particles", "ParticleApi", "particles"),
    "BossBar": ("bossbar", "BossBarApi", "bossbar"),
    "Sidebar": ("sidebar", "SidebarApi", "sidebar"),
    "HUD": ("hud", "HudApi", "hud"),
    "PlayerEffect": ("playereffect", "PlayerEffectApi", "playereffect"),
    "ActorEffect": ("actoreffect", "ActorEffectApi", "actoreffect"),
    "FakeBlock": ("fakeblock", "FakeBlockApi", "fakeblock"),
    "Form": ("forms", "FormApi", "forms"),
    "ItemDetail": ("itemdetail", "ItemDetailApi", "itemdetail"),
    "PlayerRespawn": ("playerrespawn", "PlayerRespawnApi", "playerrespawn"),
    "ItemActor": ("itemactor", "ItemActorApi", "itemactor"),
    "ItemDisplay": ("itemdisplay", "ItemDisplayApi", "itemdisplay"),
    "Container": ("container", "ContainerApi", "container"),
    "FakeInventory": ("fakeinv", "FakeInventoryApi", "fakeinv"),
    "Entity": ("entities", "EntityApi", "entities"),
    "Trade": ("trade", "TradeApi", "trade"),
    "NpcDialog": ("npcdialog", "NpcDialogApi", "npcdialog"),
    "Sulfur": ("sulfur", "SulfurApi", "sulfur"),
    "PlayerNpc": ("playernpc", "PlayerNpcApi", "playernpc"),
    "View": ("view", "ViewApi", "view"),
    "Sensing": ("sensing", "SensingApi", "sensing"),
    "Visibility": ("visibility", "VisibilityApi", "visibility"),
    "Ghost": ("ghost", "GhostApi", "ghost"),
}

CONSTANTS = {"holoApiVersion": "lib.holo_api_version"}



PARAM_DOCS = {
    "player_name": ("String", "玩家名（在线玩家，大小写敏感）"),
    "player_names": ("String", "玩家名列表，逗号分隔或用 List"),
    "entity_id": ("Int", "实体 id（`lib.entities.create(...)` 的返回值）"),
    "entity_unique_id": ("Int", "实体的 uniqueId（网络层 id，不是库内 id）"),
    "runtime_id": ("Int", "实体 runtimeId（`lib.actoreffect.runtime_id_of_entity(...)` 可取）"),
    "unique_id": ("Int", "uniqueId"),
    "vehicle_entity_id": ("Int", "载具的库内实体 id"),
    "vehicle_unique_id": ("Int", "载具的 uniqueId"),
    "shape_id": ("Int", "形状 id（`create_*` 的返回值）"),
    "ids": ("List[Int]", "形状 id 列表"),
    "text_id": ("Int", "悬浮字 id（`lib.holograms.create(...)` 的返回值）"),
    "display_id": ("Int", "展示 id（`lib.itemdisplay.create(...)` 的返回值）"),
    "menu_id": ("Int", "容器菜单 id"),
    "dialog_id": ("Int", "对话 id"),
    "npc_id": ("Int", "玩家 NPC id"),
    "trade_id": ("Int", "交易菜单 id"),
    "sulfur_id": ("Int", "硫磺方块 id"),
    "actor_id": ("Int", "掉落物 id"),
    "index": ("Int", "行号 / 槽位号，从 0 开始"),
    "x": ("Float", "X 坐标"),
    "y": ("Float", "Y 坐标"),
    "z": ("Float", "Z 坐标"),
    "x1": ("Float", "起点 X"),
    "y1": ("Float", "起点 Y"),
    "z1": ("Float", "起点 Z"),
    "x2": ("Float", "终点 X"),
    "y2": ("Float", "终点 Y"),
    "z2": ("Float", "终点 Z"),
    "dim": ("Int", "维度 id（0 主世界 / 1 下界 / 2 末地）"),
    "dim_id": ("Int", "维度 id"),
    "dimension": ("Int", "维度 id"),
    "text": ("String", "文本，支持 `§` 颜色码与 `{player}` 之类变量"),
    "title": ("String", "标题文本"),
    "lines": ("List[String]", "多行文本"),
    "name": ("String", "名字 / 标识符"),
    "identifier": ("String", "实体标识符，如 `minecraft:villager_v2`"),
    "item": ("String", "物品标识符，如 `minecraft:diamond`"),
    "aux": ("Int", "物品数据值（damage / aux）"),
    "count": ("Int", "数量"),
    "nbt": ("String", "SNBT 字符串（可空）"),
    "damage": ("Int", "耐久损伤值"),
    "scale": ("Float", "缩放倍率，1.0 = 原始大小"),
    "yaw": ("Float", "水平朝向（度）"),
    "pitch": ("Float", "俯仰角（度）"),
    "roll": ("Float", "翻滚角（度）"),
    "color": ("Int", "颜色（RGB 整数或枚举，见各域说明）"),
    "r": ("Float", "红色分量 0~1"),
    "g": ("Float", "绿色分量 0~1"),
    "b": ("Float", "蓝色分量 0~1"),
    "a": ("Float", "透明度 0~1"),
    "enabled": ("Boolean", "开 / 关"),
    "hidden": ("Boolean", "是否隐藏"),
    "on": ("Boolean", "开 / 关"),
    "visible": ("Boolean", "是否可见"),
    "transparent": ("Boolean", "是否保留透明"),
    "percent": ("Float", "百分比 0~1"),
    "effect": ("String", "粒子效果标识符，如 `minecraft:flame`"),
    "interval": ("Int", "下发间隔（tick）"),
    "lifetime": ("Int", "生命周期（tick），0 = 永久"),
    "ticks": ("Int", "tick 数"),
    "duration_ticks": ("Int", "持续时间（tick）"),
    "seconds": ("Float", "持续秒数"),
    "skin_id": ("String", "皮肤 id（注册时起的名字）"),
    "skin_of_player": ("String", "要复制的那个玩家的名字"),
    "png_path": ("String", "PNG 文件路径（建议绝对路径）"),
    "pngPath": ("String", "PNG 文件路径（建议绝对路径）"),
    "geometry": ("String", "几何模型名；空串 = 标准玩家模型"),
    "arm_size": ("Int", "手臂粗细：0 粗（wide）/ 1 细（slim）"),
    "animation": ("String", "原版动画名，如 `animation.player.attack.positions`"),
    "stop_expression": ("String", "停止表达式（molang）；空 = 由 duration 决定"),
    "entity_unique": ("Int", "实体 uniqueId"),
    "dist": ("Float", "距离（格）"),
    "distance": ("Float", "距离（格）"),
    "width": ("Float", "宽"),
    "height": ("Float", "高"),
    "slot": ("Int", "槽位号"),
    "type_": ("String", "类型标识符"),
    "message": ("String", "消息文本"),
}


def snake(name: str) -> str:
    return re.sub(r"(?<=[a-z0-9])([A-Z])", r"_\1", name).lower().lstrip("_")


def load_classes():
    sys.path.insert(0, str(SRC))
    out = {}
    for module_name, class_name, _attr in MODULE_OF.values():
        if module_name in out:
            continue
        mod = importlib.import_module(f"hologramlib_endstone.{module_name}")
        out[module_name] = getattr(mod, class_name)
    from hologramlib_endstone import anchor as anchor_mod
    from hologramlib_endstone import fakeblock as fb_mod
    from hologramlib_endstone import wire as wire_mod
    from hologramlib_endstone import bossbar as boss_mod

    out["anchor"] = anchor_mod.AnchorApi
    out["fakeblock"] = fb_mod.FakeBlockApi
    out["wire"] = wire_mod.WireApi
    out["bossbar"] = boss_mod.BossBarApi
    return out


MISC_MAP = {
    "holoRemove": ("holograms", "destroy"),
    "anchorAcquire": ("anchor", "acquire"),
    "anchorRelease": ("anchor", "release"),
    "blockFakeClear": ("fakeblock", "clear"),
    "blockFakeClearAll": ("fakeblock", "clear_all"),
    "blockFakeResolve": ("fakeblock", "resolve"),
    "blockFakeSet": ("fakeblock", "set"),
    "blockFakeSetAll": ("fakeblock", "set_all"),
    "bossBarAdd": ("bossbar", "add"),
    "bossBarAddOn": ("bossbar", "add_on"),
    "bossBarCount": ("bossbar", "count"),
    "bossBarGetInfo": ("bossbar", "get_info"),
    "bossBarProbe": ("bossbar", "probe"),
    "bossBarRemove": ("bossbar", "remove"),
    "bossBarRemoveAll": ("bossbar", "remove_all"),
    "bossBarSetHealth": ("bossbar", "set_health"),
    "bossBarSetStyle": ("bossbar", "set_style"),
    "bossBarSetTitle": ("bossbar", "set_title"),
    "easterEggCreeper": ("actoreffect", "creeper_display"),
    "easterEggCreeperDisplay": ("itemdisplay", "easter_egg"),
    "hudProbe": ("hud", "probe"),
    "hudResetAll": ("hud", "reset_all"),
    "hudSetElement": ("hud", "set_element"),
    "hudSetElements": ("hud", "set_elements"),
    "hudSetRaw": ("hud", "set_raw"),
    "playerPlayCelebrate": ("playereffect", "play_celebrate"),
    "playerPlayOminousDrink": ("playereffect", "play_ominous_drink"),
    "playerShowBadOmen": ("playereffect", "show_bad_omen"),
    "playerShowEffect": ("playereffect", "show_effect"),
    "playerShowVillageHero": ("playereffect", "show_village_hero"),
    "sidebarIsShown": ("sidebar", "is_shown"),
    "sidebarLineCount": ("sidebar", "line_count"),
    "sidebarRemove": ("sidebar", "remove"),
    "sidebarRemoveAll": ("sidebar", "remove_all"),
    "sidebarSet": ("sidebar", "set"),
    "sidebarSetLine": ("sidebar", "set_line"),
    "sidebarSetTitle": ("sidebar", "set_title"),
    "wireResetStats": ("wire", "clear"),
    "wireStats": ("wire", "stats"),
    "wireTraceActive": ("wire", "is_active"),
    "wireTraceClear": ("wire", "clear"),
    "wireTraceCount": ("wire", "count"),
    "wireTraceDump": ("wire", "dump"),
    "wireTraceMark": ("wire", "mark"),
    "wireTraceStart": ("wire", "start"),
    "wireTraceStop": ("wire", "stop"),
}


def resolve(export: dict, classes) -> tuple[str, str, str] | None:
    name = export["name"]
    group = export["group"]
    if name in MISC_MAP:
        module, method = MISC_MAP[name]
        return module, "wire" if module == "wire" else {"anchor": "anchor", "fakeblock": "fakeblock",
                                                        "bossbar": "bossbar"}.get(module, module), method
    spec = MODULE_OF.get(group)
    if spec is None:
        return None
    module, _cls, attr = spec
    cls = classes[module]
    parts = snake(name).split("_")
    for i in range(min(4, len(parts))):
        candidate = "_".join(parts[i:])
        if candidate and hasattr(cls, candidate):
            return module, attr, candidate
    return None


def signature_of(cls, method: str):
    func = inspect.getattr_static(cls, method)
    if isinstance(func, staticmethod):
        func = func.__func__
    try:
        sig = inspect.signature(func)
    except (ValueError, TypeError):
        return [], "", ""
    params = []
    for pname, p in sig.parameters.items():
        if pname == "self":
            continue
        if p.kind in (p.VAR_POSITIONAL, p.VAR_KEYWORD):
            params.append((("**" if p.kind is p.VAR_KEYWORD else "*") + pname, "", ""))
            continue
        ann = p.annotation if isinstance(p.annotation, str) else ""
        default = "" if p.default is p.empty else repr(p.default)
        if len(default) > 28:
            default = default[:25] + "..."
        params.append((pname, ann, default))
    ret = sig.return_annotation
    ret_text = "" if ret is inspect.Signature.empty else (ret if isinstance(ret, str) else getattr(ret, "__name__", str(ret)))
    return params, ret_text, str(sig).split(")")[0].split("(", 1)[1] if "(" in str(sig) else ""


SAMPLE_VALUES = {
    "player_name": '"Steve"',
    "entity_id": "1",
    "shape_id": "1",
    "text_id": "1",
    "display_id": "1",
    "actor_id": "1",
    "menu_id": "1",
    "dialog_id": "1",
    "npc_id": "1",
    "trade_id": "1",
    "sulfur_id": "1",
    "unique_id": "1",
    "runtime_id": "1",
    "x": "0.0", "y": "64.0", "z": "0.0",
    "x1": "0.0", "y1": "64.0", "z1": "0.0", "x2": "1.0", "y2": "65.0", "z2": "1.0",
    "dim": "0", "dim_id": "0", "dimension": "0",
    "text": '"hello"', "title": '"hello"', "name": '"hello"', "identifier": '"minecraft:cow"',
    "item": '"minecraft:diamond"', "block_type": '"minecraft:stone"', "state": '"minecraft:stone"',
}


def sample_args(params):
    out = []
    for pname, ptype, pdefault in params:
        out.append(SAMPLE_VALUES.get(pname, "1" if (ptype or "").lower().startswith(("int", "float")) else '"..."'))
    return ", ".join(out)


def method_section(module: str, attr: str, method: str, classes, exports: list[dict]) -> list[str]:
    cls = classes[module]
    func = inspect.getattr_static(cls, method)
    if isinstance(func, staticmethod):
        func = func.__func__
    params, ret, _ = signature_of(cls, method)
    signature_params = ", ".join(p + (f": {t}" if t else "") + (f" = {d}" if d else "") for p, t, d in params)

    out = [f"### `{method}`", ""]
    out.append(f"`lib.{attr}.{method}({signature_params})`")
    out.append("")
    if params:
        out.append("- 参数：")
        for pname, ptype, pdefault in params:
            doc_type, doc_desc = PARAM_DOCS.get(pname, (ptype or "Any", ""))
            shown_type = ptype or doc_type
            line = f"  - `{pname}` : `{shown_type}`"
            if doc_desc:
                line += f"<br>{doc_desc}"
            if pdefault:
                line += f"（默认 `{pdefault}`）"
            out.append(line)
    else:
        out.append("- 参数：无")

    doc = inspect.getdoc(func) or ""
    doc = doc.strip().splitlines()[0] if doc else ""
    out.append(f"- 返回值：{doc if doc else '成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）'}")
    out.append(f"- 返回值类型：{('`' + ret + '`') if ret else '—'}")
    out.append("- 对应 LSE 导出：" + "、".join(f"`{n['name']}`" for n in exports))
    out.append("- 示例：")
    out.append("")
    out.append("    ```python")
    out.append(f"    lib.{attr}.{method}({sample_args(params)})")
    out.append("    ```")
    out.append("")
    return out


def build_domain(entry, exports_all, classes) -> tuple[str, str]:
    group, slug, title_zh, module_name, class_name, attr, intro, example = entry
    exports = [e for e in exports_all if effective_group(e) == group]
    lines = [f"# {title_zh} · Endstone", ""]
    lines.append(intro)
    lines.append("")
    lines.append("## 快速开始")
    lines.append("")
    lines.append(example)
    lines.append("")
    lines.append("## 函数")
    lines.append("")
    current_attr = None
    for e in exports:
        if e["name"] in CONSTANTS:
            lines.append(f"#### `{CONSTANTS[e['name']]}`")
            lines.append("")
            lines.append(f"| | |")
            lines.append("|---|---|")
            lines.append(f"| 对应 LSE 导出 | `{e['name']}` |")
            lines.append("| 说明 | 库 API 版本常量（`1.35.0`），与 LL 版 `HOLOGLIB_API_VERSION` 对齐 |")
            lines.append("")
            continue
        resolved = resolve(e, classes)
        if resolved is None:
            continue
        module, attr_name, method = resolved
        if attr_name != current_attr:
            current_attr = attr_name
        lines.extend(method_section(module, attr_name, method, classes, [e]))
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append(f"完整索引见 [Endstone API 索引](api.md)。")
    lines.append("")
    return slug, "\n".join(lines)


def build_index(exports_all) -> str:
    counts = {}
    for e in exports_all:
        g = effective_group(e)
        counts[g] = counts.get(g, 0) + 1
    lines = [
        "# Endstone 版",
        "",
        "HologramLib 的 endstone 版本：与 LeviLamina 版**共用同一份协议编码**（仓库 `core/`），",
        "区别只在外壳 —— LL 版是 C++ 插件，endstone 版是 Python 插件 + `holo_core.dll`。",
        "",
        "本节按域讲解 endstone 的用法，写作标准与 [LSE 脚本接口](../lse/index.md) 一致：",
        "先说这个域能做什么、有哪些坑，再给能直接跑的示例，最后每个方法一节。",
        "",
        "## 安装",
        "",
        "```bat",
        ":: 仓库根目录；--deploy 直接装到服务端",
        "build.cmd --deploy E:\\some_server\\plugins",
        "```",
        "",
        "产物是 `dist/endstone_hologramlib-<版本>-py3-none-win_amd64.whl`，内嵌 `holo_core.dll` 与物品 id 表。",
        "",
        "!!! warning \"同版本不会重装\"",
        "",
        "    endstone 把插件装到 `plugins/.local/Lib/site-packages/`，**同版本号不会覆盖**；",
        "    升级前先删掉那里的 `hologramlib_endstone` 目录（或提升版本号）。",
        "",
        "启用日志：",
        "",
        "```",
        "[Hologramlib] HologramLib 已启用 (endstone 版, core 26.60.1, PrimitiveShapes id=328)",
        "```",
        "",
        "## 第一个功能",
        "",
        "```python",
        "from endstone.plugin import Plugin",
        "",
        "",
        "class MyPlugin(Plugin):",
        "    depend = [\"HologramLib\"]",
        "",
        "    def on_enable(self) -> None:",
        "        lib = self.server.plugin_manager.get_plugin(\"hologramlib\")",
        "        lib.api.create_text(0, 100, 0, \"hello\")",
        "        hid = lib.holograms.create(0, 101, 0)",
        "        lib.holograms.add_line(hid, \"在线 {online}\")",
        "        lib.holograms.draw(hid)",
        "```",
        "",
        "## 命名规则",
        "",
        "方法名 = LSE 导出名去掉域前缀后转 `snake_case`，域前缀对应插件上的属性：",
        "",
        "| LSE 导出 | endstone 写法 |",
        "|---|---|",
        "| `holoCreate` | `lib.holograms.create(...)` |",
        "| `shapeCreateText` | `lib.api.create_text(...)` |",
        "| `particleCreatePoint` | `lib.particles.create_point(...)` |",
        "| `itemDisplayCreate` | `lib.itemdisplay.create(...)` |",
        "| `entityCreate` | `lib.entities.create(...)` |",
        "| `playerNpcCreate` | `lib.playernpc.create(...)` |",
        "| `viewBlock` | `lib.view.view_block(...)` |",
        "| `playerHideSetHidden` | `lib.visibility.set_hidden(...)` |",
        "",
        "全部 444 条的完整签名见 [Endstone API 索引](api.md)。",
        "",
        "## 与 LeviLamina 版的关系",
        "",
        "| | LeviLamina | Endstone |",
        "|---|---|---|",
        "| 插件形态 | C++ DLL | Python wheel + `holo_core.dll` |",
        "| 调用方式 | C++ 接口 / LSE `ll.import` | Python 属性 |",
        "| 协议编码 | `core/` 编进 DLL | `core/` 编成 `holo_core.dll` |",
        "| 事件 | LL 事件总线 + Hook | `PacketSendEvent` / `PacketReceiveEvent` |",
        "| 调度 | LL tick 驱动 | `server.scheduler`（20 tick/s） |",
        "",
        "两边**字节同源**：同一个形状、同一个悬浮字，在两个平台发出的包体完全一致。",
        "",
        "## 查 API 版本",
        "",
        "```python",
        "api = lib.holo_api_version          # \"1.35.0\"",
        "major, minor, patch = (int(x) for x in api.split(\".\"))",
        "if (major, minor) >= (1, 36):",
        "    lib.fakeblock.set_many_near(...)  # 1.36.0 起才有的接口",
        "else:",
        "    lib.logger.info(f\"版本 {api} 不支持批量靠近切换，跳过\")",
        "```",
        "",
        "`lib.holo_api_version` 是字符串；LSE 的 `holoApiVersion()` 是整数（`0x012900`），两边是同一个版本的不同表示。",
        "",
        "## 按域指南",
        "",
        "| 域 | 条目 |",
        "|---|---|",
    ]
    for entry in DOMAINS:
        group, slug, title_zh = entry[0], entry[1], entry[2]
        lines.append(f"| [{title_zh}]({slug}.md) | {counts.get(group, 0)} |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    exports = json.loads((ROOT / "lib" / "exports.json").read_text(encoding="utf-8"))["exports"]
    classes = load_classes()
    OUT.mkdir(parents=True, exist_ok=True)
    check = "--check" in sys.argv
    outputs: dict[pathlib.Path, str] = {}
    outputs[OUT / "index.md"] = build_index(exports)
    for entry in DOMAINS:
        slug, text = build_domain(entry, exports, classes)
        outputs[OUT / f"{slug}.md"] = text

    failed = False
    for path, text in outputs.items():
        if check:
            current = path.read_text(encoding="utf-8") if path.exists() else ""
            if current != text:
                print(f"[DIFF] {path.name}")
                failed = True
        else:
            path.write_text(text, encoding="utf-8")
    if check:
        print(f"[endstone-docs] {len(outputs)} 个文件校验" + ("失败" if failed else "通过"))
        return 1 if failed else 0
    print(f"[endstone-docs] 生成 {len(outputs)} 个文件到 docs/endstone/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
