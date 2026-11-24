#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""pack_llsdk.py —— 把本机 xmake 缓存里的 LeviLamina 1.9.7 SDK 打成可离线复用的压缩包

CI 不再联网装 levilamina（那要现编 LL，还要 bedrockdata），改用这份预打好的 SDK：

    holo-llsdk-1.9.7/
      include/        依赖头 + ll/ + mc/（合并成一份搜索路径）
      lib/            LeviLamina.lib + 各依赖 .lib
      tools/          prelink.exe / bedrock_runtime_data
      VERSION.txt     版本与宏，供排查

用法:
    python tools/pack_llsdk.py                     # 默认 %LOCALAPPDATA%\\.xmake\\packages
    python tools/pack_llsdk.py --packages D:/pkgs --out dist/holo-llsdk-1.9.7.zip

打包来源是 `levilamina 1.9.7` 包里的 levilamina.pc：它的 -I/-L 就是编译链接真正要用的集合，
所以这里直接按它抓，不靠猜。
"""

import argparse
import os
import re
import shutil
import sys
import zipfile
from pathlib import Path

LL_VERSION = "1.9.7"
PRELINK_VERSION = "v0.7.1"
BEDROCKDATA_VERSION = "v1.21.132-server.8"
RAPIDJSON_OVERRIDE = "2025.02.05"  # v1.1.0 的 document.h clang 22 编不过


def default_packages() -> Path:
    local = os.environ.get("LOCALAPPDATA")
    if not local:
        sys.exit("找不到 %LOCALAPPDATA%，用 --packages 指定 xmake packages 目录")
    return Path(local) / ".xmake" / "packages"


def first_hashed_dir(root: Path) -> Path:
    """<pkg>/<version>/<hash> —— 取第一个带内容的那层。"""
    for d in sorted(root.iterdir()):
        if d.is_dir():
            return d
    sys.exit(f"包目录是空的: {root}")


def find_pc(packages: Path) -> Path:
    root = packages / "l" / "levilamina" / LL_VERSION
    for d in sorted(root.iterdir()):
        for pc in (d / "lib" / "pkgconfig").glob("*.pc"):
            return pc
    sys.exit(f"找不到 levilamina.pc: {root}")


def parse_pc(pc: Path) -> tuple[list[Path], list[Path], list[str]]:
    """返回 (include 目录, lib 目录, 定义)。相对 ${includedir}/${libdir} 的条目靠 SDK 自己补。"""
    incs, libs, defs = [], [], []
    prefix = pc.parent.parent.parent  # <hash>/lib -> <hash>
    for line in pc.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("Cflags"):
            for m in re.finditer(r"-I([^\s]+)", line):
                raw = m.group(1).replace("${includedir}", str(prefix / "include"))
                if not raw.startswith("${"):
                    incs.append(Path(raw))
            for m in re.finditer(r"-D([^\s]+)", line):
                defs.append(m.group(1))
        elif line.startswith("Libs:"):
            for m in re.finditer(r"-L([^\s]+)", line):
                raw = m.group(1).replace("${libdir}", str(prefix / "lib"))
                if not raw.startswith("${"):
                    libs.append(Path(raw))
    return incs, libs, defs


def copy_tree(src: Path, dst: Path) -> int:
    n = 0
    for item in src.rglob("*"):
        if not item.is_file():
            continue
        rel = item.relative_to(src)
        out = dst / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(item, out)
        n += 1
    return n


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--packages", default=str(default_packages()))
    ap.add_argument("--out", default=str(Path("dist") / f"holo-llsdk-{LL_VERSION}.zip"))
    ap.add_argument("--work", default=str(Path(".cache") / "llsdk-pack"))
    args = ap.parse_args()

    packages = Path(args.packages)
    if not packages.is_dir():
        sys.exit(f"packages 目录不存在: {packages}")

    pc = find_pc(packages)
    incs, libdirs, defs = parse_pc(pc)
    print(f"[sdk] pc: {pc}")
    print(f"[sdk] include 目录 {len(incs)} 个, lib 目录 {len(libdirs)} 个")

    work = Path(args.work)
    if work.exists():
        shutil.rmtree(work)
    root = work / f"holo-llsdk-{LL_VERSION}"
    inc_dir, lib_dir, tool_dir = root / "include", root / "lib", root / "tools"
    for d in (inc_dir, lib_dir, tool_dir):
        d.mkdir(parents=True, exist_ok=True)

    # 1) 依赖头（按 pc 顺序合并；flat 条目也拷进同一棵树）
    for inc in incs:
        if not inc.is_dir():
            print(f"[sdk] 跳过缺失的 include: {inc}")
            continue
        copy_tree(inc, inc_dir)

    # 2) rapidjson 换成能编过的 2025.02.05
    rj = packages / "r" / "rapidjson" / RAPIDJSON_OVERRIDE
    if rj.is_dir():
        rj_inc = first_hashed_dir(rj) / "include"
        if rj_inc.is_dir():
            n = copy_tree(rj_inc, inc_dir)
            print(f"[sdk] rapidjson {RAPIDJSON_OVERRIDE} 覆盖 {n} 个文件")

    # 3) LL 自己的头（ll/ + mc/）
    ll_hash = pc.parent.parent.parent
    n = copy_tree(ll_hash / "include", inc_dir)
    print(f"[sdk] levilamina 头 {n} 个文件")

    # 4) 链接库
    n = copy_tree(ll_hash / "lib", lib_dir)
    for libdir in libdirs:
        if libdir.is_dir():
            n += copy_tree(libdir, lib_dir)
    print(f"[sdk] 库 {n} 个文件")

    # 5) prelink / bedrock_runtime_data
    prelink = packages / "p" / "prelink" / PRELINK_VERSION
    for d in sorted(p for p in prelink.iterdir() if p.is_dir()):
        exe = d / "bin" / "prelink.exe"
        if exe.is_file():
            shutil.copy2(exe, tool_dir / "prelink.exe")
            break
    else:
        sys.exit(f"找不到 prelink.exe: {prelink}")

    data = packages / "b" / "bedrockdata" / BEDROCKDATA_VERSION
    for d in sorted(p for p in data.iterdir() if p.is_dir()):
        f = d / "bin" / "bedrock_runtime_data"
        if f.is_file():
            shutil.copy2(f, tool_dir / "bedrock_runtime_data")
            break
    else:
        sys.exit(f"找不到 bedrock_runtime_data: {data}")

    (root / "VERSION.txt").write_text(
        "\n".join(
            [
                f"levilamina {LL_VERSION}",
                f"prelink {PRELINK_VERSION}",
                f"bedrockdata {BEDROCKDATA_VERSION}",
                f"rapidjson {RAPIDJSON_OVERRIDE}",
                "defines " + " ".join(defs),
                "",
            ]
        ),
        encoding="utf-8",
        newline="\n",
    )

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()
    print(f"[sdk] 压缩 {out} ...")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for item in root.rglob("*"):
            if item.is_file():
                z.write(item, item.relative_to(work))
    size_mb = out.stat().st_size / 1024 / 1024
    print(f"[sdk] 完成: {out}  {size_mb:.1f} MB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
