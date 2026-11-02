"""gen_hltestlse.py - 把 exports.json 的导出名/参数表注入 tests/lse/hltestlse.js

用法: python tools/gen_hltestlse.py
"""
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JS = os.path.join(ROOT, "..", "hltestlse", "hltestlse.js")
EXPORTS = os.path.join(ROOT, "lib", "exports.json")

PLAYER_NAMES = {"playerName", "viewerName", "targetName"}
POS = {"x": "$X", "y": "$Y", "z": "$Z"}
DIM_NAMES = {"dim", "dimId", "dimension", "dimId"}
SKIP_STRING_HINTS = (
    "spec", "json", "offers", "buttons", "items", "lore", "lines", "blob", "nbt", "vert", "edges",
)


def sample(param_name: str, typ: str):
    if param_name in PLAYER_NAMES:
        return "$PLAYER"
    if param_name in POS:
        return POS[param_name]
    if param_name in DIM_NAMES:
        return "$DIM"
    if typ == "number":
        return 1
    if typ == "boolean":
        return True
    # string
    low = param_name.lower()
    if "skin" in low:
        return "hltest_skin"
    if low in ("key", "group", "groupkey"):
        return "hltestkey"
    if low in ("item", "itemid", "type", "block", "effect"):
        return "minecraft:diamond"
    if low in ("name", "npcname", "title", "text", "label", "dialogue", "scene", "sceneName".lower()):
        return "hltest"
    if low in ("preset", "animation"):
        return "spin"
    return "hltest"


def main():
    with open(EXPORTS, encoding="utf-8") as f:
        data = json.load(f)
    exports = data["exports"]
    names = sorted({e["name"] for e in exports})

    smoke = {}
    for e in exports:
        name = e["name"]
        params = e.get("params") or []
        args = []
        skip = False
        for pname, ptype in params:
            low = pname.lower()
            if ptype == "string" and any(h in low for h in SKIP_STRING_HINTS):
                skip = True
                break
            if ptype not in ("number", "string", "boolean"):
                skip = True
                break
            args.append(sample(pname, ptype))
        if skip or not params and False:
            continue
        # 名字里带 All 的清场函数不自动调（由手写分组/clean 负责）
        if re.search(r"(DestroyAll|RemoveAll|CloseAll)$", name):
            continue
        smoke[name] = args

    js = open(JS, encoding="utf-8").read()
    names_literal = "[\n" + ",\n".join('    "%s"' % n for n in names) + "\n]"
    smoke_lines = []
    for n in sorted(smoke):
        smoke_lines.append('    "%s": %s' % (n, json.dumps(smoke[n], ensure_ascii=False)))
    smoke_literal = "{\n" + ",\n".join(smoke_lines) + "\n}"

    js, n1 = re.subn(r"const ALL_EXPORTS = \[.*?\];", "const ALL_EXPORTS = " + names_literal + ";", js, count=1, flags=re.S)
    js, n2 = re.subn(r"const SMOKE_ARGS = \{.*?\};", "const SMOKE_ARGS = " + smoke_literal + ";", js, count=1, flags=re.S)
    assert n1 == 1 and n2 == 1, (n1, n2)
    open(JS, "w", encoding="utf-8", newline="\n").write(js)
    print("injected: exports=%d smoke=%d" % (len(names), len(smoke)))


if __name__ == "__main__":
    main()
