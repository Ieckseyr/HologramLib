#!/usr/bin/env python3
"""gen_lse_shim.py —— 从源码里的 exportAs 调用生成脚本侧导入层。

生成三个文件（都在插件根目录，随包分发）：

  HologramLib.js        脚本直接 require 的模块（懒绑定 + 缺 lrca 时安全空操作）
  HologramLib.d.ts      编辑器补全用的类型声明（参数名 / 类型 / 返回）
  exports.json         机器可读清单（名称 → 参数 → 返回），文档与 CI 共用

为什么生成而不是手写：导出数是 337 且还在长，手写的列表一定会和源码漂移。
CI 里跑 --check 比对，源码与生成物不一致就失败。

用法：
  python tools/gen_lse_shim.py            # 生成/更新
  python tools/gen_lse_shim.py --check    # 只检查是否最新（CI 用）
"""
from __future__ import annotations

import argparse
import io
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
# 产物直接放插件根目录：部署后就是 plugins/HologramLib/HologramLib.js，
# 脚本 require("plugins/HologramLib/HologramLib.js") 即可；发布包里也在根。
OUT_DIR = ROOT
NAMESPACE = "HologramLib"

# 前缀 → 分组（参考 GMLIB-LegacyRemoteCallApi 的"类.方法"写法）
#   (前缀, 分组名, 方法名是否去掉前缀)
# 方法名 = 导出名去掉前缀、首字母小写（itemActorSpawn → ItemActor.spawn）
GROUPS = [
    ("shape", "Shape"),
    ("holo", "Hologram"),
    ("gradient", "GradientLine"),
    ("itemDetail", "ItemDetail"),
    ("itemDisplay", "ItemDisplay"),
    ("entity", "Entity"),
    ("ghost", "Ghost"),
    ("particle", "Particle"),
    ("playerNpc", "PlayerNpc"),
    ("trade", "Trade"),
    ("container", "Container"),
    ("npcDialog", "NpcDialog"),
    ("sensing", "Sensing"),
    ("fakeInv", "FakeInventory"),
    ("sulfur", "Sulfur"),
    ("view", "View"),
    ("itemActor", "ItemActor"),
    ("playerHide", "Visibility"),
    ("locatorBar", "LocatorBar"),
    ("form", "Form"),
]
# 注意顺序：先长后短（itemDetail 要先于 item... 之类），group_for 会按长度降序试

# C++ 形参类型 → TS 类型（够用即可；拿不准的一律 any）
TYPE_MAP = [
    (r"std::string", "string"),
    (r"char const\*", "string"),
    (r"int64_t", "number"),
    (r"std::uint64_t", "number"),
    (r"int32_t", "number"),
    (r"int\b", "number"),
    (r"float", "number"),
    (r"double", "number"),
    (r"bool", "boolean"),
    (r"std::vector<[^>]*>", "any[]"),
]


def ts_type(cpp: str) -> str:
    cpp = cpp.strip()
    for pattern, ts in TYPE_MAP:
        if re.search(pattern, cpp):
            return ts
    return "any"


def match_close(text: str, open_idx: int) -> int:
    """text[open_idx] 是 '(' —— 返回与之配对的 ')' 下标。"""
    depth = 0
    i = open_idx
    in_str = False
    while i < len(text):
        ch = text[i]
        if in_str:
            if ch == "\\":
                i += 2
                continue
            if ch == '"':
                in_str = False
        elif ch == '"':
            in_str = True
        elif ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_top_level(text: str, sep: str = ",") -> list[str]:
    """按顶层分隔符切分（忽略 () [] {} <> 与字符串里的）。"""
    parts: list[str] = []
    depth = 0
    cur: list[str] = []
    in_str = False
    i = 0
    while i < len(text):
        ch = text[i]
        if in_str:
            cur.append(ch)
            if ch == "\\":
                if i + 1 < len(text):
                    cur.append(text[i + 1])
                    i += 2
                    continue
            elif ch == '"':
                in_str = False
        elif ch == '"':
            in_str = True
            cur.append(ch)
        elif ch in "([{<":
            depth += 1
            cur.append(ch)
        elif ch in ")]}>":
            depth -= 1
            cur.append(ch)
        elif ch == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
        i += 1
    if cur:
        parts.append("".join(cur))
    return [p.strip() for p in parts if p.strip()]


def parse_params(lambda_src: str) -> list[tuple[str, str]]:
    """lambda 的形参表 → [(名字, TS 类型)]。形如 `[&mgr](std::string const& playerName, bool hidden) -> bool {`"""
    m = re.search(r"\]\s*\(", lambda_src)
    if not m:
        return []
    open_idx = m.end() - 1
    close_idx = match_close(lambda_src, open_idx)
    if close_idx < 0:
        return []
    raw = lambda_src[open_idx + 1 : close_idx].strip()
    if not raw:
        return []
    out: list[tuple[str, str]] = []
    for piece in split_top_level(raw):
        # 去掉默认值与末尾 const
        piece = re.sub(r"=\s*[^,]+$", "", piece).strip()
        nm = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", piece)
        if not nm:
            continue
        name = nm.group(1)
        ctype = piece[: nm.start()].strip()
        out.append((name, ts_type(ctype)))
    return out


def parse_return(lambda_src: str) -> str:
    m = re.search(r"\)\s*(?:const\s*)?(?:mutable\s*)?->\s*([^{]+)\{", lambda_src)
    if not m:
        return "any"
    return ts_type(m.group(1))


def group_for(name: str) -> tuple[str, str]:
    """导出名 → (分组名, 方法名)。没有匹配前缀的归到 Misc，方法名就是原名。"""
    best = None
    for prefix, group in GROUPS:
        if name.startswith(prefix) and (best is None or len(prefix) > len(best[0])):
            best = (prefix, group)
    if best is None:
        return ("Misc", name)
    prefix, group = best
    rest = name[len(prefix):]
    if not rest:
        return ("Misc", name)
    method = rest[0].lower() + rest[1:]
    return (group, method)


def collect_exports() -> list[dict]:
    found: list[dict] = []
    for root, _dirs, files in os.walk(SRC):
        for fname in sorted(files):
            if not fname.endswith(".cpp"):
                continue
            path = os.path.join(root, fname)
            text = io.open(path, encoding="utf-8", errors="replace").read()
            for m in re.finditer(r"exportAs\s*\(", text):
                open_idx = m.end() - 1
                close_idx = match_close(text, open_idx)
                if close_idx < 0:
                    continue
                args = split_top_level(text[open_idx + 1 : close_idx])
                if len(args) < 3:
                    continue
                name_m = re.search(r'"([A-Za-z0-9_]+)"', args[1])
                if not name_m:
                    continue
                name = name_m.group(1)
                lambda_src = args[2]
                found.append(
                    {
                        "name": name,
                        "params": parse_params(lambda_src),
                        "returns": parse_return(lambda_src),
                        "file": os.path.relpath(path, ROOT).replace("\\", "/"),
                        "group": group_for(name)[0],
                        "method": group_for(name)[1],
                    }
                )
    found.sort(key=lambda e: e["name"])
    return found


def render_js(exports: list[dict], api_version: str, abi_stamp: str) -> str:
    names = ",\n  ".join(f'"{e["name"]}"' for e in exports)

    # 分组成员表：{ ItemActor: { spawn:"itemActorSpawn", ... }, ... }
    by_group: dict[str, list[dict]] = {}
    for e in exports:
        by_group.setdefault(e["group"], []).append(e)
    group_lines = []
    for group in sorted(by_group):
        members = sorted(by_group[group], key=lambda x: x["method"])
        body = ",".join(f'{m["method"]}:"{m["name"]}"' for m in members)
        group_lines.append(f"  {group}: {{{body}}}")
    group_members = ",\n".join(group_lines)

    return f'''// HologramLib.js —— 脚本侧导入层（自动生成，不要手改）
//
// 生成器: tools/gen_lse_shim.py    目标版本: API {api_version} / ABI 戳 {abi_stamp}
//
// 用法一（推荐，按类分组 —— 同 GMLIB-LegacyRemoteCallApi 的写法）:
//
//   const {{ ItemActor, Visibility, LocatorBar }} = require("./HologramLib.js");
//   ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//   Visibility.setHidden("Steve", true);
//
// 用法二（平铺，名字就是导出名）:
//
//   const HL = require("./HologramLib.js");
//   HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//
// 设计要点:
//   · ll.import 只在**第一次取某个函数**时做一次, 之后用缓存 —— 脚本用几个就绑几个,
//     不会像"开局循环导入全部导出"那样付几百次绑定;
//   · 库里没有 lrca / 函数名写错 → 拿到的是**安全空操作**（返回 undefined, 记一条日志）, 
//     不会让整个脚本在加载期就抛异常;
//   · 想完全避开 Proxy 的那一点点开销, 在启动时解构一次即可:
//         const {{ holoCreate, holoSetLineText }} = require("./HologramLib.js");
'use strict';

const NAMESPACE = "{NAMESPACE}";
const API_VERSION = "{api_version}";
const ABI_STAMP = "{abi_stamp}";

const NAMES = [
  {names}
];

const NAME_SET = new Set(NAMES);
const bound = Object.create(null);
let warned = Object.create(null);

function logOnce(key, message) {{
  if (warned[key]) return;
  warned[key] = true;
  try {{
    if (typeof logger !== "undefined" && logger.warn) logger.warn("[HologramLib] " + message);
    else console.warn("[HologramLib] " + message);
  }} catch (_) {{}}
}}

let versionChecked = false;

// 第一次真正绑定时, 拿 DLL 的 holoInfo() 跟生成时的版本对一次（只付一次 RPC）
function ensureVersion() {{
  if (versionChecked) return;
  versionChecked = true;
  if (typeof ll === "undefined" || typeof ll.import !== "function") return;
  try {{
    const probe = ll.import(NAMESPACE, "holoInfo");
    if (typeof probe !== "function") {{
      logOnce("no-info", "这版库没有 holoInfo（比导入层旧）—— 缺的函数会降级为空操作");
      return;
    }}
    const info = JSON.parse(probe());
    if (info && info.apiVersion && info.apiVersion !== API_VERSION) {{
      logOnce(
        "api-mismatch",
        "导入层按 API " + API_VERSION + " 生成，DLL 是 " + info.apiVersion + " —— 对不上的函数会降级为空操作"
      );
    }}
    if (info && info.abiStamp !== undefined && String(info.abiStamp) !== ABI_STAMP) {{
      logOnce("stamp-mismatch", "ABI 戳不一致（导入层 " + ABI_STAMP + " / DLL " + info.abiStamp + "），建议重新生成导入层");
    }}
  }} catch (_) {{
    /* 探测失败就不管，正常绑定照样走 */
  }}
}}

function importFn(name) {{
  // lrca 没加载 / ll.import 不可用 → 安全空操作
  if (typeof ll === "undefined" || typeof ll.import !== "function") {{
    logOnce("no-lrca", "ll.import 不可用（缺 lrca?）—— 调用 " + name + " 会被忽略");
    return function noop() {{}};
  }}
  let fn = null;
  try {{
    fn = ll.import(NAMESPACE, name);
  }} catch (err) {{
    logOnce("throw:" + name, "导入 " + name + " 抛异常: " + err);
  }}
  if (typeof fn !== "function") {{
    logOnce("missing:" + name, "库里没有 " + name + "（版本不匹配? API " + API_VERSION + "）");
    return function noop() {{}};
  }}
  return fn;
}}

function bind(name) {{
  if (bound[name]) return bound[name];
  ensureVersion();
  const fn = importFn(name);
  bound[name] = fn;
  return fn;
}}

// ── 按类分组（GMLIB 风格：类.方法）──
// 每组一个普通对象, 方法同样是懒绑定; 组对象本身是常量, 解构之后再没有 Proxy 开销。
function makeGroup(methods) {{
  const group = Object.create(null);
  for (const method in methods) {{
    const exportName = methods[method];
    group[method] = function () {{
      return bind(exportName).apply(null, arguments);
    }};
  }}
  return Object.freeze(group);
}}

const GROUP_MEMBERS = {{
{group_members}
}};

const groups = {{}};
for (const groupName in GROUP_MEMBERS) {{
  groups[groupName] = makeGroup(GROUP_MEMBERS[groupName]);
}}

const api = {{
  /** 全部分组（也可以直接解构到变量） */
  groups: groups,
  /** 库的 API 版本（如 "1.32.0"）；拿不到时是生成时的那个值 */
  apiVersion: API_VERSION,
  /** 虚表布局戳；消费方升级库时要重新编译（脚本不用管） */
  abiStamp: ABI_STAMP,
  /** 这个库到底导出了哪些函数（数组，来自源码） */
  list: function list() {{ return NAMES.slice(); }},
  /** 有没有这个函数（不看运行时, 只看生成时的清单） */
  has: function has(name) {{ return NAME_SET.has(name); }},
  /** 显式绑定一个函数（等价于取属性, 但可读性更好） */
  bind: bind,
}};

// 每个分组也挂成 api 的自有属性, 于是下面两种写法都成立:
//   const {{ ItemActor }} = require("./HologramLib.js");
//   const ItemActor = require("./HologramLib.js").ItemActor;
for (const groupName in groups) {{
  api[groupName] = groups[groupName];
}}

module.exports = new Proxy(api, {{
  get: function (target, prop) {{
    if (typeof prop !== "string") return target[prop];
    if (prop in target) return target[prop];
    if (!NAME_SET.has(prop)) {{
      logOnce("unknown:" + prop, "未知导出 " + prop + "（拼错? 或者这版库还没有）");
      return undefined;
    }}
    return bind(prop);
  }},
  has: function (target, prop) {{
    return typeof prop === "string" && (prop in target || NAME_SET.has(prop));
  }},
}});
'''


def render_dts(exports: list[dict], api_version: str) -> str:
    lines = [
        f"// HologramLib.d.ts —— 自动生成（tools/gen_lse_shim.py），API {api_version}",
        "// 放到脚本旁边, 编辑器就能补全 HL.xxx 的参数名与类型。",
        "",
        "declare namespace HologramLib {",
        "  interface Api {",
        '    /** 库的 API 版本，如 "1.32.0" */',
        "    readonly apiVersion: string;",
        "    /** 虚表布局戳（脚本侧无需关心，C++ 消费方用） */",
        "    readonly abiStamp: string;",
        "    /** 本版导出的全部函数名 */",
        "    list(): string[];",
        "    /** 有没有这个导出 */",
        "    has(name: string): boolean;",
        "    /** 显式绑定（等价于取属性） */",
        "    bind(name: string): (...args: any[]) => any;",
        "",
    ]
    for e in exports:
        params = ", ".join(f"{n}: {t}" for n, t in e["params"])
        ret = e["returns"]
        cast = f"    {e['name']}({params}): {ret};"
        doc = f'    /** 实现见 {e["file"]} */'
        lines.append(doc)
        lines.append(cast)
    lines += ["  }", "", "  // ── 按类分组（同 GMLIB-LegacyRemoteCallApi 的写法）──"]

    by_group: dict[str, list[dict]] = {}
    for e in exports:
        by_group.setdefault(e["group"], []).append(e)
    for group in sorted(by_group):
        lines.append(f"  interface {group} {{")
        for e in sorted(by_group[group], key=lambda x: x["method"]):
            params = ", ".join(f"{n}: {t}" for n, t in e["params"])
            lines.append(f"    {e['method']}({params}): {e['returns']};")
        lines.append("  }")
        lines.append("")
    lines.append("  interface Groups {")
    for group in sorted(by_group):
        lines.append(f"    {group}: {group};")
    lines.append("  }")
    lines += [
        "",
        "  const groups: Groups;",
        "}",
        "",
        "declare const HL: HologramLib.Api & HologramLib.Groups;",
        "export = HL;",
        "",
    ]
    return "\n".join(lines)


def render_json(exports: list[dict], api_version: str) -> str:
    payload = {
        "namespace": NAMESPACE,
        "apiVersion": api_version,
        "count": len(exports),
        "exports": exports,
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def read_versions() -> tuple[str, str, str]:
    header = io.open(
        os.path.join(ROOT, "include", "hologramlib", "HologramLib.h"), encoding="utf-8"
    ).read()
    ver = re.search(r"#define\s+HOLOGLIB_API_VERSION\s+(0x[0-9A-Fa-f]+)", header)
    stamp = re.search(r"HOLOGLIB_ABI_STAMP\s*=\s*(\d+)", header)
    if not ver or not stamp:
        sys.exit("[shim] 头文件里找不到 HOLOGLIB_API_VERSION / HOLOGLIB_ABI_STAMP")
    raw = int(ver.group(1), 16)
    # 0x012600 → 1.32.0：次版本号 = (raw >> 8) & 0xFF - 6（见 HologramLib.h 的注释与版本矩阵）
    semver = f"{raw >> 16}.{((raw >> 8) & 0xFF) - 6}.{raw & 0xFF}"
    return ver.group(1), semver, stamp.group(1)


def main() -> int:
    ap = argparse.ArgumentParser(description="生成 LSE 导入层")
    ap.add_argument("--check", action="store_true", help="只检查生成物是否最新")
    args = ap.parse_args()

    api_hex, api_semver, abi_stamp = read_versions()
    exports = collect_exports()
    if not exports:
        sys.exit("[shim] 一个 exportAs 都没找到，检查 src/ 路径")

    outputs = {
        os.path.join(OUT_DIR, "HologramLib.js"): render_js(exports, api_semver, abi_stamp),
        os.path.join(OUT_DIR, "HologramLib.d.ts"): render_dts(exports, api_semver),
        os.path.join(OUT_DIR, "exports.json"): render_json(exports, api_semver),
    }

    stale = []
    for path, content in outputs.items():
        old = io.open(path, encoding="utf-8", newline="").read() if os.path.exists(path) else None
        if old != content:
            stale.append(os.path.relpath(path, ROOT))
            if not args.check:
                os.makedirs(os.path.dirname(path), exist_ok=True)
                io.open(path, "w", encoding="utf-8", newline="\n").write(content)

    if args.check:
        if stale:
            print("[shim] 生成物过期: " + ", ".join(stale))
            print("[shim] 跑一次 python tools/gen_lse_shim.py 再提交")
            return 1
        print(f"[shim] 最新（{len(exports)} 个导出，API {api_semver}）")
        return 0

    print(f"[shim] 写出 {len(exports)} 个导出的导入层（API {api_semver} / {api_hex}）")
    for path in outputs:
        print("  " + os.path.relpath(path, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
