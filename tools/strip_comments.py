#!/usr/bin/env python3
"""strip_comments.py —— 清空 C++ 源码里的全部注释（保留行号与字符串字面量）。

只做一件事：把 // 行注释、/* */ 块注释删掉，其他字节原样保留；注释里的换行保留，
所以行号不变。字符串 / 字符 / 原始字符串（R"x(...)x"）里的 // 与 /* 不动。
"""
from __future__ import annotations

import sys
from pathlib import Path


def strip(text: str) -> str:
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if c == "/" and nxt == "/":
            i += 2
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == "/" and nxt == "*":
            i += 2
            while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                if text[i] == "\n":
                    out.append("\n")
                i += 1
            i += 2
            continue

        # 原始字符串 R"delim( ... )delim"
        if c == "R" and nxt == '"' and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")):
            j = i + 2
            delim = []
            while j < n and text[j] not in "(\n":
                delim.append(text[j])
                j += 1
            if j < n and text[j] == "(":
                close = ")" + "".join(delim) + '"'
                end = text.find(close, j + 1)
                if end >= 0:
                    end += len(close)
                    out.append(text[i:end])
                    i = end
                    continue
        if c == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                j += 1
            out.append(text[i:j])
            i = j
            continue
        if c == "'":
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == "'":
                    j += 1
                    break
                j += 1
            out.append(text[i:j])
            i = j
            continue

        out.append(c)
        i += 1
    return "".join(out)


def tidy(text: str) -> str:
    lines = [ln.rstrip() for ln in text.split("\n")]
    out: list[str] = []
    blank = 0
    for ln in lines:
        if ln == "":
            blank += 1
            if blank > 1:
                continue
        else:
            blank = 0
        out.append(ln)
    while out and out[0] == "":
        out.pop(0)
    while out and out[-1] == "":
        out.pop()
    return "\n".join(out) + "\n"


def main(argv: list[str]) -> int:
    roots = [Path(a) for a in argv[1:]] or [Path("src"), Path("include")]
    files = []
    for root in roots:
        if root.is_file():
            files.append(root)
        else:
            files.extend(sorted(p for p in root.rglob("*") if p.suffix in (".cpp", ".h")))
    total = 0
    for p in files:
        raw = p.read_text(encoding="utf-8", errors="surrogateescape")
        mtime_lines = raw.count("\n")
        new = tidy(strip(raw))
        if new != raw:
            p.write_text(new, encoding="utf-8", errors="surrogateescape", newline="\n")
            total += 1
    print(f"{total} files stripped ({len(files)} scanned)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
