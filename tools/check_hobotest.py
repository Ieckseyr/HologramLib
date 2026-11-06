"""检查 hobotest 的 444 条用例是否都能解析到真实方法（不需要服务器）。

用法：python tools/check_hobotest.py
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "platform" / "endstone" / "src"))
sys.path.insert(0, str(ROOT / "platform" / "endstone" / "tests" / "hobotest"))

from cases import CASES  # noqa: E402

import hologramlib_endstone as hl  # noqa: E402
from hologramlib_endstone.plugin import HologramLibPlugin  # noqa: E402

ATTR = {
    "api": "ShapeApi", "holograms": "HologramApi", "gradients": "GradientApi", "particles": "ParticleApi",
    "bossbar": "BossBarApi", "sidebar": "SidebarApi", "hud": "HudApi", "playereffect": "PlayerEffectApi",
    "actoreffect": "ActorEffectApi", "locatorbar": "LocatorBarApi", "fakeblock": "FakeBlockApi",
    "forms": "FormApi", "itemdetail": "ItemDetailApi", "playerrespawn": "PlayerRespawnApi",
    "itemactor": "ItemActorApi", "itemdisplay": "ItemDisplayApi", "container": "ContainerApi",
    "fakeinv": "FakeInventoryApi", "entities": "EntityApi", "trade": "TradeApi", "npcdialog": "NpcDialogApi",
    "sulfur": "SulfurApi", "playernpc": "PlayerNpcApi", "view": "ViewApi", "sensing": "SensingApi",
    "visibility": "VisibilityApi", "ghost": "GhostApi", "anchor": "AnchorApi", "wire": "WireApi",
}


def main() -> int:
    mods = {}
    missing = []
    for attr in {row[1] for rows in CASES.values() for row in rows}:
        if not hasattr(HologramLibPlugin, attr):
            missing.append(f"插件缺少属性 {attr}")
    total = 0
    for rows in CASES.values():
        for label, attr, method, _args in rows:
            total += 1
            if attr not in mods:
                mods[attr] = getattr(hl, ATTR[attr])
            if not hasattr(mods[attr], method):
                missing.append(f"{label} -> {attr}.{method}")
    print(f"[hobotest] {total} 条用例，缺失 {len(missing)} 条")
    for item in missing:
        print("  " + item)
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
