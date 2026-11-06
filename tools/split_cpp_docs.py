"""把 docs/reference/cpp-api.md 拆成 docs/cpp/ 下的独立分页（C++ 接口大节）。

- 1.1 入口 + 通用约定 -> docs/cpp/index.md
- 1.2 ~ 1.17 每个接口一页
拆完保留原文的签名块，只在标题层级上做调整，不手改内容。
"""

from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "docs" / "reference" / "cpp-api.md"
OUT = ROOT / "docs" / "cpp"

SLUGS = {
    "1.2": ("shape-drawer", "IShapeDrawer · 形状渲染"),
    "1.3": ("hologram-text", "IHologramText · 悬浮字"),
    "1.4": ("item-detail", "IItemDetail · 物品详情"),
    "1.5": ("item-display", "IItemDisplay · 物品悬浮展示"),
    "1.6": ("custom-entity", "ICustomEntity · 自定义实体"),
    "1.7": ("ghost-interact", "GhostInteractEvent · 交互事件"),
    "1.8": ("particle-shape", "IParticleShape · 粒子形状"),
    "1.9": ("player-npc", "IPlayerNpc · 假玩家 NPC"),
    "1.10": ("trade-menu", "ITradeMenu · 交易菜单"),
    "1.11": ("npc-dialogue", "INpcDialogue · NPC 对话"),
    "1.12": ("container-menu", "IContainerMenu · 虚拟容器"),
    "1.13": ("sensing", "感知域 · 逐玩家变量"),
    "1.14": ("fake-inventory", "IFakeInventory · 假背包"),
    "1.15": ("sulfur-display", "ISulfurDisplay · 硫磺方块"),
    "1.16": ("view-override", "IViewOverride · 视图覆盖"),
    "1.17": ("item-actor-form-locator", "IItemActor / IFormControl / ILocatorBar"),
}


def split_sections(text: str) -> tuple[str, list[tuple[str, str, str]]]:
    """返回 (前言, [(编号, 标题, 正文)...])。"""
    parts = re.split(r"\n(?=## 1\.\d+)", text)
    preamble = parts[0]
    sections = []
    for chunk in parts[1:]:
        m = re.match(r"## (1\.\d+) ([^\n]+)\n", chunk)
        if not m:
            continue
        sections.append((m.group(1), m.group(2).strip(), chunk[m.end():]))
    return preamble, sections


def main() -> None:
    text = SRC.read_text(encoding="utf-8")
    preamble, sections = split_sections(text)
    OUT.mkdir(parents=True, exist_ok=True)

    # ── 总览页：标题 + 前言 + 1.1 入口 + 清单 ──
    entry = next((body for num, _t, body in sections if num == "1.1"), "")
    index = [
        "# C++ 接口",
        "",
        "HologramLib 的 C++ 接口是**冻结的纯虚接口**：消费方 `enable()` 时 `getInstance()` 拿到它，",
        "不需要声明依赖，也不用管实现类怎么变。",
        "",
        "!!! tip \"先看这个\"",
        "",
        "    各个接口的完整签名在下面分页里；本页只讲入口、通用约定、线程模型和一条最小可跑的例子。",
        "",
        "## 30 秒上手",
        "",
        "```cpp",
        '#include "HologramLib/HologramLib.h"',
        "",
        "void onEnable() {",
        "    auto& lib = hologramlib::IHologramLib::getInstance();",
        "    if (auto* shapes = &lib.shapes(); shapes != nullptr) {",
        '        shapes->createText({0, 100, 0}, "hello");',
        "    }",
        "}",
        "```",
        "",
        "## 接口清单",
        "",
        "| 接口 | 取法 | 能力 |",
        "|---|---|---|",
        "| `IShapeDrawer` | `lib.shapes()` | 线框、球、圆、箭头、文字形状 |",
        "| `IHologramText` | `lib.holograms()` | 多行悬浮字、轮播、逐玩家变量 |",
        "| `IItemDetail` | `lib.itemDetails()` | 「物品 + 名称 + Lore」悬浮详情 |",
        "| `IItemDisplay` | `lib.itemDisplays()` | 3D 物品悬浮（FMBE） |",
        "| `ICustomEntity` | `lib.customEntities()` | 协议层自定义实体 |",
        "| `IParticleShape` | `lib.particleShapes()` | 粒子拼形状 |",
        "| `IPlayerNpc` | `lib.playerNpcs()` | 假玩家 NPC |",
        "| `ITradeMenu` | `lib.tradeMenus()` | 村民交易菜单 |",
        "| `INpcDialogue` | `lib.npcDialogs()` | NPC 对话框 |",
        "| `IContainerMenu` | `lib.containerMenus()` | 虚拟容器 / 列表界面 |",
        "| `IPlayerSensing` | `lib.playerSensing()` | 客户端感知、千人千面 |",
        "| `IFakeInventory` | `lib.fakeInventories()` | 假背包 |",
        "| `ISulfurDisplay` | `lib.sulfurDisplays()` | 硫磺方块展示 |",
        "| `IViewOverride` | `lib.viewOverrides()` | 客户端视图覆盖 |",
        "| `IItemActor` | `lib.itemActors()` | 协议层掉落物 |",
        "| `IFormControl` | `lib.formControls()` | 强制关闭表单 |",
        "| `ILocatorBar` | `lib.locatorBars()` | 方位条 |",
        "",
        "## 分页",
        "",
    ]
    for num, title, _body in sections:
        if num == "1.1":
            continue
        slug, human = SLUGS.get(num, (f"section-{num}", title))
        index.append(f"- [{num} {human}]({slug}.md)")
    index.append("")
    index.append("## 入口与通用约定")
    index.append("")
    index.append(entry.strip())
    index.append("")
    (OUT / "index.md").write_text("\n".join(index), encoding="utf-8")

    # ── 每个接口一页 ──
    for num, title, body in sections:
        if num == "1.1":
            continue
        slug, human = SLUGS.get(num, (f"section-{num}", title))
        page = [
            f"# {num} {human}",
            "",
            f"完整签名（`#include \"HologramLib/HologramLib.h\"`）：",
            "",
            body.strip(),
            "",
            "---",
            "",
            "[← 返回 C++ 接口总览](index.md)",
            "",
        ]
        (OUT / f"{slug}.md").write_text("\n".join(page), encoding="utf-8")

    print(f"[cpp-split] {len(sections)} 节 -> {OUT}")


if __name__ == "__main__":
    main()
