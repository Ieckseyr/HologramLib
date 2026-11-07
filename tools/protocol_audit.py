#!/usr/bin/env python3
"""协议文档对拍工具（人工升级协议的辅助脚本）。

用途：换 BDS 协议版本时，快速核对"我们实际用到的包"与
protocol-docs 对应版本是否一致。不做代码生成——schema 仍在
bedrock-protocol 仓库里人工维护（见 docs/internal/协议升级流程.md）。

用法：
    python tools/protocol_audit.py ids
    python tools/protocol_audit.py ids --docs ../../.llsdk/protocol-docs-r26_u6
    python tools/protocol_audit.py fields AddPlayerPacket SerializedSkinRef

ids   ：从 core/ 与 platform/ 里扫出用到的包名，编译探针取 bp 侧 Id，
        与 protocol-docs 的 id 比较；有不一致则退出码 1。
fields：打印指定 packet/type 的文档字段列表，供人工与生成头对照。
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_DOCS = ROOT.parent.parent / ".llsdk" / "protocol-docs-r26_u6"
INCLUDE = ROOT.parent.parent / ".llsdk" / "endstone-proto" / "install" / "include"
CLANG_CL = pathlib.Path(r"C:\Program Files\LLVM\bin\clang-cl.exe")


def collect_packets() -> list[str]:
    names: set[str] = set()
    pattern = re.compile(r"\b(?:bp|proto|core::proto)::([A-Z]\w+Packet)\b")
    for base in ("core", "platform"):
        for path in (ROOT / base).rglob("*"):
            if path.suffix not in (".cpp", ".h"):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            names.update(pattern.findall(text))
    return sorted(names)


def docs_ids(docs: pathlib.Path, names: list[str]) -> dict[str, int | None]:
    out: dict[str, int | None] = {}
    for name in names:
        path = docs / "packets" / f"{name}.json"
        out[name] = json.loads(path.read_text(encoding="utf-8"))["id"] if path.exists() else None
    return out


def probe_ids(names: list[str]) -> dict[str, int]:
    if not CLANG_CL.exists():
        raise SystemExit(f"找不到 clang-cl：{CLANG_CL}（可用 --clang 指定）")
    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = pathlib.Path(tmp)
        src = tmpdir / "probe.cpp"
        lines = ["#include <bedrock/protocol.hpp>", "#include <cstdio>", "namespace bp = bedrock::protocol;", "int main() {"]
        lines += [f'    std::printf("{n} %d\\n", bp::{n}::Id);' for n in names]
        lines += ["    return 0;", "}"]
        src.write_text("\n".join(lines), encoding="utf-8")
        exe = tmpdir / "probe.exe"
        subprocess.run(
            [str(CLANG_CL), "/nologo", "/std:c++latest", "/EHsc", "/MD", f"-I{INCLUDE}", str(src), f"/Fe:{exe}"],
            check=True,
            capture_output=True,
        )
        out = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout
    result: dict[str, int] = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[1].lstrip("-").isdigit():
            result[parts[0]] = int(parts[1])
    return result


def cmd_ids(args: argparse.Namespace) -> int:
    names = collect_packets()
    if not names:
        print("没有从源码里扫到任何包名，检查正则或路径")
        return 2
    documented = docs_ids(args.docs, names)
    ours = probe_ids(names)
    missing_docs = [n for n, v in documented.items() if v is None]
    bad = [(n, documented[n], ours.get(n)) for n in names if documented[n] is not None and ours.get(n) != documented[n]]
    print(f"协议包 {len(names)} 个：文档 {len(names) - len(missing_docs)}，一致 {len(names) - len(missing_docs) - len(bad)}，不一致 {len(bad)}")
    if missing_docs:
        print("文档缺条目：", ", ".join(missing_docs))
    for name, doc_id, our_id in bad:
        print(f"  ✗ {name}: docs={doc_id} ours={our_id}")
    if bad or missing_docs:
        return 1
    print("全部一致")
    return 0


def cmd_fields(args: argparse.Namespace) -> int:
    for name in args.names:
        for folder in ("packets", "types"):
            path = args.docs / folder / f"{name}.json"
            if not path.exists():
                continue
            data = json.loads(path.read_text(encoding="utf-8"))
            print(f"## {folder}/{name}  id={data.get('id')}")
            for field in data.get("fields", []):
                ftype = json.dumps(field.get("type"), ensure_ascii=False)
                extra = " ".join(f"{k}={field[k]}" for k in ("enum", "repeat", "value") if k in field)
                print(f"   - {field.get('name')} | {ftype} {extra}".rstrip())
            print()
            break
        else:
            print(f"## {name}: 文档里没有这个 packet/type")
    return 0


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except (AttributeError, ValueError):
            pass
    parser = argparse.ArgumentParser(description="protocol-docs 对拍工具")
    parser.add_argument("action", choices=("ids", "fields"))
    parser.add_argument("names", nargs="*", help="fields 模式下要打印的 packet/type 名")
    parser.add_argument("--docs", type=pathlib.Path, default=DEFAULT_DOCS)
    parser.add_argument("--clang", type=pathlib.Path, default=CLANG_CL)
    args = parser.parse_args()
    if not args.docs.exists():
        print(f"文档目录不存在：{args.docs}")
        return 2
    if args.action == "ids":
        return cmd_ids(args)
    return cmd_fields(args)


if __name__ == "__main__":
    sys.exit(main())
