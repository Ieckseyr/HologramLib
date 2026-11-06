"""从 core/c_api.h 生成 ABI 符号清单（docs/abi/symbols.md）。

解析 `HOLO_CORE_EXPORT <ret> name(args);`（可能跨行），按用途分组：
包 id / 编码器 / 生命周期。函数体里的实现细节不参与。
"""

from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
HEADER = ROOT / "core" / "c_api.h"
OUT = ROOT / "docs" / "abi" / "symbols.md"


def parse() -> list[tuple[str, str, str]]:
    text = HEADER.read_text(encoding="utf-8")
    # 去掉注释，保留声明
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    out = []
    for m in re.finditer(r"HOLO_CORE_EXPORT\s+([A-Za-z_][\w \*]*?)\s+(holo_[\w]+)\s*\(([^;]*?)\)\s*;", text, re.S):
        ret = " ".join(m.group(1).split())
        name = m.group(2)
        args = " ".join(m.group(3).split())
        out.append((ret, name, args))
    return out


def group_of(name: str) -> str:
    if name.startswith("holo_core_packet_id_"):
        return "包 id"
    if name.startswith("holo_core_encode_") or name.startswith("holo_core_build_"):
        return "编码 / 构造"
    return "生命周期与版本"


def main() -> int:
    symbols = parse()
    groups: dict[str, list] = {}
    for ret, name, args in symbols:
        groups.setdefault(group_of(name), []).append((ret, name, args))

    lines = [
        "# ABI 符号清单",
        "",
        f"`holo_core.dll` 导出 **{len(symbols)}** 个 C 符号（全部 `extern \"C\"`、`__declspec(dllexport)`），",
        "声明在仓库 `core/c_api.h`。本页由 `tools/gen_abi_docs.py` 从该头文件生成，不手抄。",
        "",
        "| 组 | 数量 | 用途 |",
        "|---|---|---|",
    ]
    purpose = {
        "包 id": "把包 id 从 core 取出来，避免在消费方硬编码",
        "编码 / 构造": "把参数编成包体（body，不含包头）",
        "生命周期与版本": "版本号、ABI 版本、释放 core 分配的内存",
    }
    for key in ("包 id", "编码 / 构造", "生命周期与版本"):
        items = groups.get(key, [])
        if items:
            lines.append(f"| {key} | {len(items)} | {purpose[key]} |")
    lines.append("")

    for key in ("包 id", "编码 / 构造", "生命周期与版本"):
        items = groups.get(key, [])
        if not items:
            continue
        lines.append(f"## {key}（{len(items)}）")
        lines.append("")
        lines.append("| 符号 | 返回 | 参数 |")
        lines.append("|---|---|---|")
        for ret, name, args in items:
            args_disp = args.replace("|", "\\|") if args else "—"
            lines.append(f"| `{name}` | `{ret}` | `{args_disp}` |")
        lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("完整用法见 [ABI 接口总览](index.md)。")
    lines.append("")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"[abi-docs] symbols.md: {len(symbols)} 个符号")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
