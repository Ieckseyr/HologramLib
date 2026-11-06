"""生成 Endstone API 索引（docs/endstone/api.md）：按域列出全部 444 个方法，链到对应域页的锚点。

方法名与 LSE 导出的对应关系来自 lib/exports.json + 源码 inspect；
域页里的方法节由 tools/gen_endstone_docs.py 生成，两边共用同一套解析规则。
"""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs" / "endstone"

sys.path.insert(0, str(ROOT / "tools"))

import gen_endstone_docs as gen  # noqa: E402


def build() -> str:
    exports = json.loads((ROOT / "lib" / "exports.json").read_text(encoding="utf-8"))["exports"]
    classes = gen.load_classes()
    by_group: dict[str, list] = {}
    for e in exports:
        by_group.setdefault(gen.effective_group(e), []).append(e)

    lines = [
        "# Endstone API 索引",
        "",
        f"全部 **{len(exports)}** 个导出，按域列出；点方法名进到对应域页的那一节，",
        "每个方法都有签名、参数、返回类型与对应的 LSE 导出名。",
        "",
        "命名规则：方法名 = LSE 导出名去掉域前缀后转 `snake_case`，域前缀对应插件属性"
        "（`lib.holograms`、`lib.entities` …）。",
        "",
    ]

    for entry in gen.DOMAINS:
        group, slug, title_zh = entry[0], entry[1], entry[2]
        items = by_group.get(group, [])
        if not items:
            continue
        lines.append(f"## {title_zh}（{len(items)}）")
        lines.append("")
        lines.append(f"页面：[{title_zh}]({slug}.md)")
        lines.append("")
        for e in items:
            name = e["name"]
            if name in gen.CONSTANTS:
                lines.append(f"- `{gen.CONSTANTS[name]}` ← `{name}`（常量）")
                continue
            resolved = gen.resolve(e, classes)
            if resolved is None:
                lines.append(f"- `{gen.snake(name)}` ← `{name}`（未映射）")
                continue
            _module, attr, method = resolved
            lines.append(f"- [`{method}`]({slug}.md#{method}) — `lib.{attr}.{method}(...)` ← `{name}`")
        lines.append("")
    return "\n".join(lines)


def main() -> int:
    check = "--check" in sys.argv
    text = build()
    path = DOCS / "api.md"
    if check:
        current = path.read_text(encoding="utf-8") if path.exists() else ""
        print("[OK] api.md" if current == text else "[DIFF] api.md")
        return 0 if current == text else 1
    path.write_text(text, encoding="utf-8")
    print(f"[endstone-api] api.md: {len(text)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
