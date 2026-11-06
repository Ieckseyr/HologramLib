"""生成 endstone 全量调用测试的用例表（platform/endstone/tests/hobotest/cases.py）。

对 lib/exports.json 里的 444 个导出，按参数名生成示例实参：
    $PLAYER  目标玩家名        $X/$Y/$Z  锚点坐标        $DIM  维度
    $ID      最近一次 create 返回的 id          $UID  锚点玩家的 uniqueId

跑法（服务端内）：/hobotest all | /hobotest <group> | /hobotest bind | /hobotest clean
"""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "platform" / "endstone" / "tests" / "hobotest" / "cases.py"

sys.path.insert(0, str(ROOT / "tools"))
import gen_endstone_docs as gen  # noqa: E402

# 导出名 -> (模块属性, 方法名) 由命名规则推导，与文档生成器同一套规则
PREFIX_MODULE = {
    "Shape": "api",
    "Hologram": "holograms",
    "GradientLine": "gradients",
    "Particle": "particles",
    "BossBar": "bossbar",
    "Sidebar": "sidebar",
    "HUD": "hud",
    "PlayerEffect": "playereffect",
    "ActorEffect": "actoreffect",
    "LocatorBar": "locatorbar",
    "FakeBlock": "fakeblock",
    "Form": "forms",
    "ItemDetail": "itemdetail",
    "PlayerRespawn": "playerrespawn",
    "ItemActor": "itemactor",
    "ItemDisplay": "itemdisplay",
    "Container": "container",
    "FakeInventory": "fakeinv",
    "Entity": "entities",
    "Trade": "trade",
    "NpcDialog": "npcdialog",
    "Sulfur": "sulfur",
    "PlayerNpc": "playernpc",
    "View": "view",
    "Sensing": "sensing",
    "Visibility": "visibility",
    "Ghost": "ghost",
}

# Misc 组按前缀分派
MISC_PREFIX = [
    ("bossBar", "bossbar"),
    ("hud", "hud"),
    ("sidebar", "sidebar"),
    ("player", "playereffect"),
    ("blockFake", "fakeblock"),
    ("easterEgg", "actoreffect"),
    ("wire", "wire"),
    ("anchor", "anchor"),
]

PLAYER_PARAMS = {"playerName", "viewerName", "targetName", "name"}
POS_PARAMS = {"x": "$X", "y": "$Y", "z": "$Z", "x1": "$X", "y1": "$Y", "z1": "$Z", "x2": "$X", "y2": "$Y", "z2": "$Z"}
DIM_PARAMS = {"dim", "dimId", "dimension"}
ID_PARAMS = {"id", "uniqueId", "runtimeId", "displayId", "menuId", "dialogId", "npcId", "tradeId",
             "sulfurId", "actorId", "shapeId", "textId", "entityId", "vehicleEntityId", "desiredId",
             "vehicleUniqueId", "entityUniqueId", "libId", "barId", "targetId", "id"}
STRING_DEFAULT = {
    "item": "minecraft:diamond",
    "type": "minecraft:stone",
    "block": "minecraft:stone",
    "state": "minecraft:stone",
    "blockType": "minecraft:stone",
    "effect": "minecraft:flame",
    "skin": "hltest_skin",
    "name": "hltest",
    "title": "hltest",
    "text": "hltest",
    "label": "hltest",
    "dialogue": "hltest",
    "scene": "hltest",
    "sceneName": "hltest",
    "npcName": "hltest",
    "geometry": "",
    "armSize": "wide",
    "animation": "animation.player.attack.positions",
    "key": "hltestkey",
    "group": "hltestgroup",
    "groupKey": "hltestgroup",
    "identifier": "minecraft:villager_v2",
    "spec": "",
    "pngPath": "",
    "path": "",
    "dirPath": "",
    "texture": "",
    "itemName": "minecraft:diamond",
    "buyAType": "minecraft:emerald",
    "buyBType": "",
    "sellType": "minecraft:diamond",
    "sellName": "hltest",
    "buttonsSpec": "确定|ok|0|say ok",
    "tradeType": "Trade",
    "carrierIdentifier": "minecraft:villager_v2",
    "containerType": "inventory",
    "instanceKey": "hltest",
}


def snake(name: str) -> str:
    return re.sub(r"(?<=[a-z0-9])([A-Z])", r"_\1", name).lower().lstrip("_")


def module_of(group: str, name: str) -> str | None:
    if group == "Misc":
        for prefix, module in MISC_PREFIX:
            if name.startswith(prefix):
                return module
        return None
    return PREFIX_MODULE.get(group)


def sample(param: str, typ: str):
    if param in PLAYER_PARAMS:
        return "$PLAYER"
    if param in POS_PARAMS:
        return POS_PARAMS[param]
    if param in DIM_PARAMS:
        return "$DIM"
    if param in ("uniqueId", "entityUniqueId", "vehicleUniqueId"):
        return "$UID"
    if param in ID_PARAMS or param.endswith("Id"):
        return "$ID"
    low = param.lower()
    for key, value in STRING_DEFAULT.items():
        if low == key.lower():
            return value
    if typ == "number":
        return 1
    if typ == "boolean":
        return True
    return "hltest"


def main() -> int:
    exports = json.loads((ROOT / "lib" / "exports.json").read_text(encoding="utf-8"))["exports"]
    classes = gen.load_classes()
    groups: dict[str, list] = {}
    for e in exports:
        group = gen.effective_group(e)
        resolved = gen.resolve(e, classes)
        if resolved is None:
            continue
        module_name, attr, method = resolved
        groups.setdefault(group, []).append((e["name"], attr, method, e["params"]))

    lines = [
        '"""全量调用测试用例表（由 tools/gen_hobotest.py 生成，不要手改）。',
        "",
        "每条：[标签, 插件属性, 方法名, 实参列表]。实参里的 $PLAYER/$X/$Y/$Z/$DIM/$ID/$UID",
        "由 harness 在运行时替换。",
        '"""',
        "",
        "CASES = {",
    ]
    total = 0
    for group in sorted(groups):
        rows = groups[group]
        lines.append(f'    "{group}": [')
        for name, module, method, params in rows:
            args = [sample(p[0], p[1] if len(p) > 1 else "") for p in params]
            args_text = ", ".join(repr(a) for a in args)
            lines.append(f'        ("{name}", "{module}", "{method}", [{args_text}]),')
            total += 1
        lines.append("    ],")
    lines.append("}")
    lines.append("")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"[hobotest] cases.py: {total} 条，{len(groups)} 组")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
