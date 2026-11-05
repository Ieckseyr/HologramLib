#!/usr/bin/env python3
"""gen_export_list.py —— 重新生成 docs/internal/LSE导出清单.md 的「分组清单」一段

为什么要有这个：那一页的清单是手抄出来的，抄到 270 条就再没跟上过源码
（源码 414 条的时候它还写着 270）。清单本身有唯一机械真值 —— 每个
`exportAs(NAMESPACE, "名", ...)` 调用点 —— 所以让它生成，别让它手抄。

生成范围只有「## 分组清单」到「## 计数核对」之间那一段；
页头说明、计数核对、历史异常项是人工内容，脚本一个字都不碰。

用法：
    python tools/gen_export_list.py            # 重写那一段
    python tools/gen_export_list.py --check    # 只检查是否最新（CI 用）

退出码：0 = 通过 / 已更新，1 = --check 时发现不一致。
"""
from __future__ import annotations

import argparse
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
DOC = os.path.join(ROOT, "docs", "internal", "LSE导出清单.md")

BEGIN = "## 分组清单"
END = "## 计数核对"

# 与 tools/gen_lse_shim.py 的 GROUPS 保持一致（长前缀在前，避免 item 吃掉 itemDetail）
GROUPS = [
    ("shape", "shape*"),
    ("holo", "holo*"),
    ("gradient", "gradient*"),
    ("itemDetail", "itemDetail*"),
    ("itemDisplay", "itemDisplay*"),
    ("entity", "entity*"),
    ("ghost", "ghost*"),
    ("particle", "particle*"),
    ("playerNpc", "playerNpc*"),
    ("trade", "trade*"),
    ("container", "container*"),
    ("npcDialog", "npcDialog*"),
    ("sensing", "sensing*"),
    ("fakeInv", "fakeInv*"),
    ("sulfur", "sulfur*"),
    ("view", "view*"),
    ("itemActor", "itemActor*"),
    ("playerRespawn", "playerRespawn*"),
    ("actor", "actor*"),
    ("playerHide", "playerHide*"),
    ("locatorBar", "locatorBar*"),
    ("form", "form*"),
    ("anchor", "anchor*"),
    ("bossBar", "bossBar*"),
    ("hud", "hud*"),
    ("sidebar", "sidebar*"),
    ("wire", "wire*"),
]

EXPORT_RE = re.compile(r'exportAs\s*\(\s*[^,\n]+,\s*"([A-Za-z0-9_]+)"')


def collect() -> list[tuple[str, int, str]]:
    """返回 [(名字, 行号, 文件相对路径)]，按文件遍历顺序。

    匹配跨行（`exportAs(` ↵ `NAMESPACE,` ↵ `"名"` 这种写法名字在第三行），
    行号取名字字面量所在行，与旧清单口径一致。
    """
    found: list[tuple[str, int, str]] = []
    for root, dirs, files in os.walk(SRC):
        dirs.sort()
        for name in sorted(files):
            if not name.endswith(".cpp"):
                continue
            path = os.path.join(root, name)
            rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
            text = io.open(path, encoding="utf-8", errors="replace").read()
            for match in EXPORT_RE.finditer(text):
                lineno = text.count("\n", 0, match.start(1)) + 1
                found.append((match.group(1), lineno, rel))
    return found


def group_for(func_name: str) -> str:
    for prefix, label in GROUPS:
        if func_name.startswith(prefix):
            return label
    return "其他"


def render(items: list[tuple[str, int, str]]) -> str:
    buckets: dict[str, list[tuple[str, int, str]]] = {}
    for item in items:
        buckets.setdefault(group_for(item[0]), []).append(item)

    order = [label for _p, label in GROUPS] + ["其他"]
    lines = [
        BEGIN,
        "",
        "> 本节由 `python tools/gen_export_list.py` 从 `src/**/*.cpp` 里所有 `exportAs(NAMESPACE, \"名\", …)`",
        "> 调用点生成，**不要手改**；`--check` 会在 CI 里挡住漂移。行号 = 函数名字面量所在行。",
        "",
        f"共 **{len(items)}** 条，分布在 {len({rel for _n, _l, rel in items})} 个文件。",
        "",
    ]
    for label in order:
        bucket = buckets.get(label)
        if not bucket:
            continue
        files = sorted({rel for _n, _l, rel in bucket})
        lines.append(f"### {label}（{len(bucket)} 个）")
        lines.append("")
        lines.append(f"来源：{', '.join(f'`{f}`' for f in files)}")
        lines.append("")
        for index, (func_name, lineno, _rel) in enumerate(bucket, 1):
            lines.append(f"{index}. `{func_name}` :{lineno}")
        lines.append("")
    return "\n".join(lines)


def splice(existing: str, section: str) -> str:
    start = existing.index(BEGIN)
    stop = existing.index(END)
    return existing[:start] + section + "\n" + existing[stop:]


def main() -> int:
    ap = argparse.ArgumentParser(description="重新生成 LSE 导出清单的分组清单段")
    ap.add_argument("--check", action="store_true", help="只检查是否最新")
    args = ap.parse_args()

    # docs/internal/ 不进仓库，干净 clone / CI 里这份清单不存在：没有可比的生成物就跳过。
    if not os.path.exists(DOC):
        print(f"[export-list] 没有 {os.path.relpath(DOC, ROOT)}（docs/internal 未入库），跳过")
        return 0

    items = collect()
    section = render(items)
    existing = io.open(DOC, encoding="utf-8").read()
    updated = splice(existing, section)

    if args.check:
        if updated != existing:
            print("[export-list] 清单与源码不一致，跑 python tools/gen_export_list.py 重新生成")
            return 1
        print(f"[export-list] 一致（{len(items)} 条）")
        return 0

    if updated == existing:
        print(f"[export-list] 无变化（{len(items)} 条）")
        return 0
    io.open(DOC, "w", encoding="utf-8", newline="\n").write(updated)
    print(f"[export-list] 已写回 {os.path.relpath(DOC, ROOT)}（{len(items)} 条）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
