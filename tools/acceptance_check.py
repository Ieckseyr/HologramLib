#!/usr/bin/env python3
"""acceptance_check.py —— 跑验收标准里能自动化的那一部分，输出 PASS/FAIL 表。

用法：python tools/acceptance_check.py [--json out.json]
退出码：0 = 全部通过；1 = 有失败项。

只覆盖 AC-B / AC-Q / AC-A / AC-E / AC-D / AC-PK 里可用命令判定的条目；
运行时功能、游戏内清单、性能预算需要人工执行（见 docs/internal/验收标准-1.35.0.md）。
"""
from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "platform" / "levilamina" / "src"
INC = ROOT / "include"


def run(cmd: list[str]) -> tuple[int, str]:
    env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUTF8="1")
    p = subprocess.run(
        cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env
    )
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


class Checker:
    def __init__(self) -> None:
        self.rows: list[tuple[str, str, bool, str]] = []
        self.exempt: list[tuple[str, str, str]] = []
        self.skipped: list[tuple[str, str, str]] = []

    def add(self, cid: str, title: str, ok: bool, detail: str = "") -> None:
        self.rows.append((cid, title, ok, detail))

    def skip(self, cid: str, title: str, why: str) -> None:
        self.skipped.append((cid, title, why))

    def exemption(self, rule: str, path: Path, line_count: int) -> dict | None:
        """登记制例外：只认 tools/acceptance_exceptions.json 里逐条写明的条目。"""
        f = ROOT / "tools" / "acceptance_exceptions.json"
        if not f.is_file():
            return None
        data = json.loads(read_text(f))
        for e in data.get("exceptions", []):
            if e.get("rule") != rule:
                continue
            if e.get("path") and Path(e["path"]).as_posix() != path.as_posix():
                continue
            if line_count <= int(e.get("max_lines", 0)):
                return e
        return None

    def rule_exemption(self, rule: str) -> dict | None:
        """整条规则的口径裁决（无 path）：只登记，不算通过。"""
        f = ROOT / "tools" / "acceptance_exceptions.json"
        if not f.is_file():
            return None
        for e in json.loads(read_text(f)).get("exceptions", []):
            if e.get("rule") == rule and not e.get("path"):
                return e
        return None

    def python(self, cid: str, title: str, args: list[str], expect: str | None = None) -> None:
        code, out = run([sys.executable] + args)
        ok = code == 0 and (expect is None or expect in out)
        self.add(cid, title, ok, out.strip().splitlines()[-1][:120] if out.strip() else f"exit={code}")

    def node(self, cid: str, title: str, args: list[str]) -> None:
        code, out = run(["node"] + args)
        self.add(cid, title, code == 0, out.strip().splitlines()[-1][:120] if out.strip() else f"exit={code}")

    def report(self) -> int:
        failed = 0
        for cid, title, ok, detail in self.rows:
            if not ok:
                failed += 1
            print(f"[{'PASS' if ok else 'FAIL'}] {cid:8s} {title}" + (f"  — {detail}" if detail else ""))
        for cid, title, why in self.skipped:
            print(f"[SKIP] {cid:8s} {title}  — {why}")
        for clause, what, reason in self.exempt:
            print(f"[EXEMPT] {clause:12s} {what}  — {reason}")
        print(f"\n{len(self.rows) - failed}/{len(self.rows)} 通过")
        return 0 if failed == 0 else 1


def main() -> int:
    c = Checker()

    hdr = read_text(INC / "hologramlib" / "HologramLib.h")
    manifest = json.loads(read_text(ROOT / "manifest.json"))
    tooth = json.loads(read_text(ROOT / "tooth.json"))

    api_macro = re.search(r"#define\s+HOLOGLIB_API_VERSION\s+(0x[0-9A-Fa-f]+)", hdr)
    api_value = api_macro.group(1).lower() if api_macro else "?"
    manifest_api = str(manifest.get("extraInfo", {}).get("apiVersion", "")).lower()

    c.add("AC-A01", "API 版本 1.35.0(0x012900)", api_value == "0x012900", f"当前 {api_value}")
    c.add("AC-A02", "manifest 与宏一致", manifest_api == api_value, f"manifest={manifest_api}")
    semver = manifest.get("extraInfo", {}).get("apiVersionSemver", "x")
    abi_file = ROOT / "abi" / f"v{semver}.json"
    if not abi_file.is_file():
        abi_file = ROOT / "abi" / f"{semver}.json"
    abi_api = str(json.loads(read_text(abi_file)).get("apiVersion", "")).lower() if abi_file.is_file() else ""
    c.add(
        "AC-A03",
        "abi/ 基线存在且 apiVersion 一致",
        abi_file.is_file() and abi_api == api_value,
        f"{abi_file.name} apiVersion={abi_api or '缺失'}",
    )

    # 必须对 v1.34.0 对拍：默认基线会取到本次从头文件生成的 v1.35.0.json，对拍恒过
    c.python(
        "AC-A04",
        "ABI 槽位只追加（对 1.34.0 基线）",
        ["tools/abi_tool.py", "check", "--baseline", "abi/v1.34.0.json"],
        "通过",
    )
    c.python("AC-E01", "导出清单与文档一致", ["tools/check_exports.py", "--check-docs"], "通过")
    c.python("AC-E02", "内部分组清单与源码一致", ["tools/gen_export_list.py", "--check"], "一致")
    c.python("AC-E03", "shim 与源码同步", ["tools/gen_lse_shim.py", "--check"])
    c.node("AC-E04", "shim 自测", ["tools/shim_test.js"])
    c.python("AC-D01", "文档站可构建", ["-m", "mkdocs", "build", "-f", "tools/mkdocs.yml", "-d", str(Path("/tmp") / "site-accept")])
    c.python("AC-D02", "文档措辞检查", ["tools/check_docs_wording.py"], "通过")
    c.python("AC-PK01", "tooth 校验", ["tools/check_tooth.py"], "通过")
    c.python("AC-PK02", "可打发布包", ["tools/pack_release.py", "--check"], "齐全")

    c.add(
        "AC-PK04",
        "包内版本 26.60.1 / api 0x012900",
        str(manifest.get("version")) == "26.60.1" and manifest_api == "0x012900",
        f"version={manifest.get('version')} api={manifest_api}",
    )
    dest = tooth.get("variants", [{}])[0].get("assets", [{}])[0].get("placements", [{}])[0].get("dest", "")
    c.add("AC-D06", "tooth 安装目录 mods/HologramLib", dest == "mods/HologramLib", dest)

    # 代码卫生（可自动化的部分）
    def grep_count(pattern: str, roots: list[Path], include: tuple[str, ...] = (".cpp", ".h")) -> int:
        n = 0
        for root in roots:
            for p in root.rglob("*"):
                if p.suffix in include and p.is_file():
                    n += len(re.findall(pattern, read_text(p)))
        return n

    catch_n = grep_count(r"catch\s*\(\s*\.\.\.\s*\)", [SRC, INC])
    c.add("AC-Q04", "catch(...) ≤ 9", catch_n <= 9, f"{catch_n} 处")
    todo_n = grep_count(r"TODO|FIXME|XXX|HACK", [SRC, INC])
    c.add("AC-Q06", "无 TODO/FIXME", todo_n == 0, f"{todo_n} 处")
    deco = 0
    for p in list(SRC.rglob("*")) + list(INC.rglob("*")):
        if p.suffix in (".cpp", ".h") and p.is_file():
            for line in read_text(p).splitlines():
                s = line.strip()
                if s.startswith("//") and ("====" in s or "----" in s or "──" in s):
                    deco += 1
    c.add("AC-Q01", "无装饰性分隔注释", deco == 0, f"{deco} 行")
    over = [p for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")) + list(INC.rglob("*.h"))
            if p.is_file() and len(read_text(p).splitlines()) > 900]
    fails = []
    for p in over:
        n = len(read_text(p).splitlines())
        e = c.exemption("AC-Q08", p.relative_to(ROOT), n)
        if e is None:
            fails.append(f"{p.name}:{n}")
        else:
            c.exempt.append((e.get("clause", "?"), f"{p.name}:{n}", e.get("reason", "")))
    c.add("AC-Q08", "单文件 ≤ 900 行（src+include；例外须登记在 acceptance_exceptions.json）",
          not fails, ", ".join(fails[:4]))

    # AC-E05 导出分组抽查：口径已裁决（actorBlock*→ActorEffect、playerRespawn→PlayerRespawn、
    # playerRespawn 归 PlayerRespawn），裁决必须落在 acceptance_exceptions.json，
    # 否则门禁红、人工记录绿，下一个人只能再判一次。
    e05 = c.rule_exemption("AC-E05")
    if e05 is None:
        c.add("AC-E05", "导出分组抽查（口径裁决须登记）", False, "acceptance_exceptions.json 缺 AC-E05 条目")
    else:
        c.exempt.append((e05.get("clause", "AC-E05-EX1"), "AC-E05 导出分组抽查", e05.get("reason", "")))

    # 纯逻辑单测：以前只在 tests/README.md 里手敲，CI 不跑等于没门禁
    cpp = shutil.which("clang++") or shutil.which("g++")
    unit = [
        ("AC-E06", "registry_test", ["-pthread"]),
        ("AC-E07", "click_event_test", []),
        ("AC-E08", "shape_id_map_test", []),
        ("AC-E09", "visibility_logic_test", []),
    ]
    if cpp is None:
        for cid, name, _ in unit:
            c.skip(cid, f"{name} 单测", "本机没有 clang++/g++")
    else:
        outdir = ROOT / "build" / "unit"
        outdir.mkdir(parents=True, exist_ok=True)
        for cid, name, flags in unit:
            exe = outdir / (name + (".exe" if os.name == "nt" else ""))
            args = [cpp, "-std=c++20", "-O1", "-I", str(SRC), str(ROOT / "tests" / f"{name}.cpp"), "-o", str(exe)]
            code, out = run(args + flags)
            if code != 0:
                last = out.strip().splitlines()[-1][:100] if out.strip() else "编译失败"
                c.add(cid, f"{name} 单测", False, f"编译失败: {last}")
                continue
            code, out = run([str(exe)])
            tail = out.strip().splitlines()[-1][:100] if out.strip() else f"exit={code}"
            c.add(cid, f"{name} 单测", code == 0, tail)

    return c.report()


if __name__ == "__main__":
    sys.exit(main())
