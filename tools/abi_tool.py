#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""abi_tool.py —— HologramLib 的 ABI 契约工具

背景（为什么需要它）
--------------------------------------------------------------------------------
HologramLib.h 是**冻结契约**：虚函数的声明顺序就是它在虚表里的槽位号。
消费方（MeowHolographicRenderer / MeowMenu / Mtps ...）把这份顺序在编译期固化成机器人指令，
运行时才去 DLL 虚表里取。库里在中段插入/删除/重排一个虚函数，消费方就会静默跳到别的方法上
—— 2026-10-09 / 10-10 两次 0xC0000005 启动崩溃就是这么来的（栈全在消费方代码里，
看起来像"两个插件互相冲突"）。

在冻结契约面前，"别在中段插入"靠人记、靠注释、靠 code review 都不够。本工具把它变成
**一条命令的退出码**：

    python tools/abi_tool.py check          # 漂移 = 非 0，可直接进 CI

子命令
--------------------------------------------------------------------------------
extract   从当前头文件抽出契约快照（JSON），可另出 Markdown 槽位表
check     与 abi/ 下的基线比对；尾部追加放行，中段插入/删除/改签名/改结构体 = 失败
diff      打印两份快照的差异（人工看用）

退出码：0 = 通过，1 = 漂移，2 = 用法错误。
"""

import argparse
import json
import os
import re
import sys

# Windows 控制台默认 cp936: 直接 print 中文或 —/→ 这类字符会 UnicodeEncodeError，
# 让 check 在输出阶段假失败（退出码 1，契约其实没动）。固定成 UTF-8。
for _stream in (sys.stdout, sys.stderr):
    try:
        _stream.reconfigure(encoding="utf-8", errors="replace")
    except Exception:  # noqa: BLE001 - 老 Python / 非标准流: 保持原样
        pass

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = os.path.join(ROOT, "include", "hologramlib", "HologramLib.h")
ABI_DIR = os.path.join(ROOT, "abi")

# 参数名剥离时需要保留的类型关键字（这些出现在末尾时是类型的一部分，不是名字）
TYPE_KEYWORDS = {
    "const", "volatile", "unsigned", "signed", "long", "short",
    "int", "char", "float", "double", "bool", "void", "auto",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "size_t",
}

IDENT_RE = re.compile(r"^[A-Za-z_]\w*$")


# ─────────────────────────────────────────────────────────────── 词法辅助

def strip_comments(text):
    """去掉 // 与 /* */ 注释，保留换行（行号不漂）。"""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                if text[i] == "\n":
                    out.append("\n")
                i += 1
            i += 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def find_block(text, start):
    """从 text[start] == '{' 开始，返回配对的 '}' 的下标。"""
    assert text[start] == "{", "find_block expects '{'"
    depth = 0
    i = start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced braces")


def split_top_level(s, sep=","):
    """按顶层分隔符切分（忽略 <> () [] 内部的 sep）。"""
    parts, depth, cur = [], 0, []
    for c in s:
        if c in "(<[{":
            depth += 1
        elif c in ")>]}":
            depth -= 1
        if c == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    if "".join(cur).strip():
        parts.append("".join(cur))
    return parts


def collapse(s):
    return re.sub(r"\s+", " ", s).strip()


def strip_param_name(param):
    """把 `std::string const& text` 归一成 `std::string const&`。

    只依赖一个可靠事实：参数名总是**末尾的那个标识符**，且它前面是空白，
    而类型侧的末尾 token 只可能是 * & > 或类型关键字。
    """
    p = collapse(param)
    if not p:
        return p
    tokens = p.split(" ")
    if len(tokens) < 2:
        return p
    last = tokens[-1]
    if not IDENT_RE.match(last) or last in TYPE_KEYWORDS:
        return p
    return " ".join(tokens[:-1])


def normalize_signature(decl, class_name):
    """decl 形如 'virtual int64_t createText(float x, ...) = 0' —— 去掉 virtual/默认值/函数体，
    抹平参数名与空白，得到可跨版本比较的签名。"""
    d = collapse(decl)
    d = re.sub(r"^virtual\s+", "", d)
    d = re.sub(r"\s*=\s*(0|default|delete)\s*$", "", d)
    d = d.strip()

    paren = d.find("(")
    if paren < 0:  # 不是函数（理论上不会出现）
        return d
    # 找到与第一个 '(' 匹配的 ')'
    depth, close = 0, None
    for idx in range(paren, len(d)):
        if d[idx] == "(":
            depth += 1
        elif d[idx] == ")":
            depth -= 1
            if depth == 0:
                close = idx
                break
    if close is None:
        return d
    head = d[:paren].strip()
    params = d[paren + 1:close]
    normalized = ", ".join(strip_param_name(p) for p in split_top_level(params))
    return f"{head}({normalized})"


# ─────────────────────────────────────────────────────────────── 提取

def parse_methods(body, class_name):
    """按出现顺序返回 [{index, kind, sig}]；index 就是虚表槽位号。"""
    slots = []
    i, n = 0, len(body)
    while True:
        j = body.find("virtual", i)
        if j < 0:
            break
        if j > 0 and (body[j - 1].isalnum() or body[j - 1] == "_"):
            i = j + len("virtual")
            continue

        p = j + len("virtual")
        paren = 0
        decl_end = None
        kind = "pure"
        while p < n:
            c = body[p]
            if c == "(":
                paren += 1
            elif c == ")":
                paren -= 1
            elif paren == 0 and c == ";":
                decl_end = p
                break
            elif paren == 0 and c == "{":
                decl_end = p
                kind = "body"
                # 跳过函数体
                depth, q = 0, p
                while q < n:
                    if body[q] == "{":
                        depth += 1
                    elif body[q] == "}":
                        depth -= 1
                        if depth == 0:
                            break
                    q += 1
                p = q
                break
            p += 1
        if decl_end is None:
            break

        decl = body[j:decl_end]
        if re.search(r"=\s*default", decl):
            kind = "default"
        sig = normalize_signature(decl, class_name)
        slots.append({"index": len(slots), "kind": kind, "sig": sig})
        i = p + 1
    return slots


def parse_struct_fields(body):
    fields = []
    for raw in split_top_level(body, ";"):
        line = collapse(raw)
        if not line:
            continue
        if line.startswith(("using ", "friend ", "typedef ", "static ")):
            continue
        # 去掉成员初始化器：`bool visible{false}` / `int x = 0` / `float x{0}, y{0}, z{0}`
        line = re.sub(r"\{[^{}]*\}", "", line)
        line = re.sub(r"\s*=\s*[^,;]+$", "", line)
        if not line:
            continue
        fields.append(collapse(line))
    return fields


def parse_enumerators(body):
    values, next_value = {}, 0
    for raw in split_top_level(body, ","):
        item = collapse(raw)
        if not item:
            continue
        m = re.match(r"^(\w+)\s*=\s*(.+)$", item)
        if m:
            name, expr = m.group(1), collapse(m.group(2))
            try:
                next_value = int(expr, 0)
            except ValueError:
                values[name] = expr  # 非常量表达式：原样记录
                continue
        else:
            name, expr = item, None
        values[name] = next_value
        next_value = next_value + 1 if expr is None else next_value + 1
    return values


def extract():
    global HEADER
    with open(HEADER, "r", encoding="utf-8") as f:
        raw = f.read()
    text = strip_comments(raw)

    m = re.search(r"#define\s+HOLOGLIB_API_VERSION\s+(0x[0-9A-Fa-f]+|\d+)", text)
    if not m:
        raise SystemExit("ABI-ERROR: HOLOGLIB_API_VERSION not found")
    api_version = m.group(1)
    api_version_num = int(api_version, 0)

    m = re.search(r"HOLOGLIB_ABI_STAMP\s*=\s*(\d+)", text)
    if not m:
        raise SystemExit("ABI-ERROR: HOLOGLIB_ABI_STAMP not found")
    abi_stamp = int(m.group(1))

    # 人类可读版本: 头文件顶部有一行 "0x012500（26.40.12 / 26.51.5，API 1.31.0）：..."
    # 中间字节只是十六进制计数器, 与 semver 不是简单换算（0x1A 对应的是 1.20.0），所以只认这一处注释。
    m = re.search(r"API\s+(\d+\.\d+\.\d+)", text)
    api_semver = m.group(1) if m else None

    interfaces, structs, enums = {}, {}, {}

    # 接口类：class IXxx { ... };
    for m in re.finditer(r"\bclass\s+(I[A-Za-z_]\w*)\s*(?::[^{]+)?\{", text):
        name = m.group(1)
        open_brace = text.index("{", m.start())
        close = find_block(text, open_brace)
        body = text[open_brace + 1:close]
        interfaces[name] = parse_methods(body, name)

    # 结构体 / 联合体
    for m in re.finditer(r"\b(struct|union)\s+([A-Za-z_]\w*)\s*\{", text):
        kind, name = m.group(1), m.group(2)
        open_brace = text.index("{", m.start())
        close = find_block(text, open_brace)
        body = text[open_brace + 1:close]
        structs[name] = {"kind": kind, "fields": parse_struct_fields(body)}

    # 枚举（含 enum class X : int）
    for m in re.finditer(r"\benum\s+(?:class\s+|struct\s+)?([A-Za-z_]\w*)\s*(?::\s*([\w:]+))?\s*\{", text):
        name = m.group(1)
        underlying = m.group(2) or "int"
        open_brace = text.index("{", m.start())
        close = find_block(text, open_brace)
        body = text[open_brace + 1:close]
        enums[name] = {"underlying": collapse(underlying), "values": parse_enumerators(body)}

    try:
        header_rel = os.path.relpath(HEADER, ROOT).replace("\\", "/")
    except ValueError:  # 跨盘符（-H 指向别处时）
        header_rel = HEADER.replace("\\", "/")

    return {
        "header": header_rel,
        "apiVersion": api_version,
        "apiVersionNumber": api_version_num,
        "apiVersionSemver": api_semver,
        "abiStamp": abi_stamp,
        "interfaces": interfaces,
        "structs": structs,
        "enums": enums,
    }


# ─────────────────────────────────────────────────────────────── 比对

def load_baseline(path=None):
    if path:
        with open(path, encoding="utf-8") as f:
            return json.load(f), path
    if not os.path.isdir(ABI_DIR):
        return None, None
    candidates = sorted(
        (os.path.join(ABI_DIR, n) for n in os.listdir(ABI_DIR) if re.match(r"^v[\d.]+\.json$", n)),
        key=lambda p: [int(x) for x in re.findall(r"\d+", os.path.basename(p))],
    )
    if not candidates:
        return None, None
    path = candidates[-1]
    with open(path, encoding="utf-8") as f:
        return json.load(f), path


def compare(base, cur):
    """返回 (errors, warnings, summary_lines)。"""
    errors, warnings, lines = [], [], []

    for iface, base_slots in base["interfaces"].items():
        cur_slots = cur["interfaces"].get(iface)
        if cur_slots is None:
            errors.append(f"接口消失: {iface}（删类 = 破坏契约）")
            continue

        for i, b in enumerate(base_slots):
            if i >= len(cur_slots):
                errors.append(f"{iface}: 基线槽 #{i} '{b['sig']}' 被删除或该接口被截短")
                continue
            c = cur_slots[i]
            if c["sig"] != b["sig"]:
                where = "中段插入/重排/改签名" if c["sig"] not in {
                    s["sig"] for s in cur_slots[i + 1:]
                } else "重排"
                errors.append(
                    f"{iface}: 槽 #{i} 变了（{where}）\n"
                    f"      基线: {b['sig']}\n"
                    f"      当前: {c['sig']}"
                )
        added = cur_slots[len(base_slots):]
        if added:
            lines.append(f"{iface}: +{len(added)} 个尾部槽" + "".join(f"\n      #{s['index']} {s['sig']}" for s in added))

    for iface, cur_slots in cur["interfaces"].items():
        if iface not in base["interfaces"]:
            lines.append(f"新接口: {iface}（{len(cur_slots)} 槽; 消费方可用 API 版本门住）")

    for name, b in base["structs"].items():
        c = cur["structs"].get(name)
        if c is None:
            errors.append(f"结构体消失: {name}")
            continue
        if b["fields"] != c["fields"]:
            detail = []
            for i, bf in enumerate(b["fields"]):
                cf = c["fields"][i] if i < len(c["fields"]) else "<删除>"
                if bf != cf:
                    detail.append(f"      字段 #{i}: 基线 '{bf}' → 当前 '{cf}'")
            if len(c["fields"]) > len(b["fields"]):
                for extra in c["fields"][len(b["fields"]):]:
                    detail.append(f"      尾部新增字段: '{extra}'")
            errors.append(
                f"结构体布局变了: {name}（按值/引用跨 DLL 传递 → 消费方必须重编译）\n" + "\n".join(detail)
            )

    for name, c in cur["structs"].items():
        if name not in base["structs"]:
            lines.append(f"新结构体: {name}（{len(c['fields'])} 字段）")

    for name, b in base["enums"].items():
        c = cur["enums"].get(name)
        if c is None:
            errors.append(f"枚举消失: {name}")
            continue
        if b["values"] != c["values"]:
            diff = [
                f"      {k}: {b['values'].get(k)} → {c['values'].get(k)}"
                for k in sorted(set(b["values"]) | set(c["values"]))
                if b["values"].get(k) != c["values"].get(k)
            ]
            errors.append(f"枚举值变了: {name}（线上/ABI 数值变了）\n" + "\n".join(diff))

    # 版本与布局戳的一致性
    if cur["apiVersionNumber"] < base["apiVersionNumber"]:
        errors.append(
            f"API 版本回退: {base['apiVersion']} → {cur['apiVersion']}"
        )
    elif cur["apiVersionNumber"] == base["apiVersionNumber"] and any(l.startswith(("IShape", "IHologram", "IItem")) or " 个尾部槽" in l for l in lines):
        warnings.append(f"有新增槽位但 HOLOGLIB_API_VERSION 未抬（当前 {cur['apiVersion']}）")

    breaking = bool(errors)
    if breaking:
        if cur["abiStamp"] <= base["abiStamp"]:
            errors.append(
                f"HOLOGLIB_ABI_STAMP 未抬（基线 {base['abiStamp']}，当前 {cur['abiStamp']}）"
                f" —— 破坏槽位的改动必须 +1，否则消费方的运行期自检拦不住"
            )
    else:
        if cur["abiStamp"] != base["abiStamp"]:
            errors.append(
                f"HOLOGLIB_ABI_STAMP 从 {base['abiStamp']} 变成 {cur['abiStamp']}，但槽位序没变"
                f" —— 追加不应动布局戳（布局戳只在破坏槽位时 +1）"
            )

    return errors, warnings, lines


# ─────────────────────────────────────────────────────────────── 输出

def to_markdown(snap):
    out = [
        f"# ABI 槽位表（{snap['apiVersion']} / ABI 戳 {snap['abiStamp']}）",
        "",
        "> 本文件由 `python tools/abi_tool.py extract --md-out abi/slots.md` 生成，不要手改。",
        "> 虚函数声明顺序 = 虚表槽位号。**只能在尾部追加**。",
        "",
    ]
    for iface, slots in snap["interfaces"].items():
        out += [f"## {iface}", "", "| # | 种类 | 签名 |", "|---|---|---|"]
        for s in slots:
            out.append(f"| {s['index']} | {s['kind']} | `{s['sig']}` |")
        out.append("")
    if snap["structs"]:
        out += ["## 结构体布局", "", "| 结构体 | 字段（按最终布局顺序） |", "|---|---|"]
        for name, info in snap["structs"].items():
            out.append(f"| {name} | `{'` `'.join(info['fields'])}` |")
        out.append("")
    return "\n".join(out)


def cmd_extract(args):
    snap = extract()
    if args.json_out:
        os.makedirs(os.path.dirname(os.path.abspath(args.json_out)), exist_ok=True)
        with open(args.json_out, "w", encoding="utf-8", newline="\n") as f:
            json.dump(snap, f, indent=2, ensure_ascii=False, sort_keys=False)
            f.write("\n")
        print(f"[ok] 写出 {args.json_out}")
    if args.md_out:
        with open(args.md_out, "w", encoding="utf-8", newline="\n") as f:
            f.write(to_markdown(snap))
        print(f"[ok] 写出 {args.md_out}")
    if not args.json_out and not args.md_out:
        json.dump(snap, sys.stdout, indent=2, ensure_ascii=False)
        print()
    return 0


def cmd_diff(args):
    a, _ = load_baseline(args.a)
    b = extract() if not args.b else json.load(open(args.b, encoding="utf-8"))
    errors, warnings, lines = compare(a, b)
    for l in lines:
        print(f"  ~ {l}")
    for w in warnings:
        print(f"  ! {w}")
    for e in errors:
        print(f"  x {e}")
    return 0


MANIFEST = os.path.join(ROOT, "manifest.json")


def verify_manifest(snapshot, manifest_path=None):
    """manifest.json 的 extraInfo 必须与头文件一致。

    LeviLamina 的 Manifest.extraInfo 是自由字符串表（SmallStringMap<std::string>），所以能放
    自定义元数据而不影响加载; 这里只保证它不会和头文件漂移。
    """
    manifest_path = manifest_path or MANIFEST
    if not os.path.isfile(manifest_path):
        return ["manifest.json 不存在（应带 extraInfo: apiVersion / apiVersionSemver / abiStamp）"]
    try:
        with open(manifest_path, encoding="utf-8-sig") as f:
            manifest = json.load(f)
    except Exception as exc:  # noqa: BLE001 - 坏 JSON 就是错误
        return [f"manifest.json 解析失败: {exc}"]

    info = manifest.get("extraInfo")
    if not isinstance(info, dict):
        suggestion = (
            '"extraInfo": {'
            f'"apiVersion": "{snapshot["apiVersion"]}", '
            f'"apiVersionSemver": "{snapshot.get("apiVersionSemver") or "?"}", '
            f'"abiStamp": "{snapshot["abiStamp"]}"'
            "}"
        )
        return [f"manifest.json 缺 extraInfo（ABI 版本展示），应加: {suggestion}"]

    want = {
        "apiVersion": snapshot["apiVersion"],
        "abiStamp": str(snapshot["abiStamp"]),
    }
    if snapshot.get("apiVersionSemver"):
        want["apiVersionSemver"] = snapshot["apiVersionSemver"]

    errors = []
    for key, expected in want.items():
        actual = info.get(key)
        if actual is None:
            errors.append(f'manifest.json extraInfo 缺 "{key}"（应为 "{expected}"）')
        elif str(actual) != expected:
            errors.append(f'manifest.json extraInfo.{key} = "{actual}"，与头文件 "{expected}" 不一致')
    return errors


def cmd_check(args):
    base, base_path = load_baseline(args.baseline)
    cur = extract()
    if base is None:
        print("[abi] 没有基线，先把当前头文件固化为基线：")
        print("      python tools/abi_tool.py extract --json-out abi/v%s.json" % cur["apiVersion"])
        return 0

    errors, warnings, lines = compare(base, cur)
    # manifest 描述的是**本仓库这次构建**：用 -H 检查别的头文件（单测夹具/消费方镜像）时不比它
    if not getattr(args, "header", None):
        errors += verify_manifest(cur, getattr(args, "manifest", None))
    print(f"[abi] 基线 {os.path.relpath(base_path, ROOT)}  ({base['apiVersion']}, 戳 {base['abiStamp']})")
    print(f"[abi] 当前      {cur['apiVersion']}, 戳 {cur['abiStamp']}")
    for l in lines:
        print(f"  ~ {l}")
    for w in warnings:
        print(f"  ! {w}")

    if errors:
        print("\n[abi] 失败 —— 冻结契约被破坏：")
        for e in errors:
            print(f"  x {e}")
        print(
            "\n处理方式:\n"
            "  1. 能改回尾部追加就改回去（旧二进制才不会跳到错槽）；\n"
            "  2. 确实要破坏槽位 → HOLOGLIB_ABI_STAMP +1、HOLOGLIB_API_VERSION 抬次版本、\n"
            "     在 CHANGELOG 记明，并同步通知所有消费方**重新编译**；\n"
            "  3. 大版本（2.0.0）再清理废弃空槽。"
        )
        return 1

    if args.write_snapshot and (lines or warnings):
        if cur["apiVersionNumber"] > base["apiVersionNumber"]:
            out = os.path.join(ABI_DIR, f"v{args.release or cur['apiVersion']}.json")
        else:
            out = os.path.join(ABI_DIR, f"v{args.release or base['apiVersion']}.json")
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            json.dump(cur, f, indent=2, ensure_ascii=False)
            f.write("\n")
        print(f"[abi] 基线已更新 → {os.path.relpath(out, ROOT)}")

    print("[abi] 通过：槽位序是基线的超集（只在尾部追加）")
    return 0


# ─────────────────────────────────────────────────────────────── 消费方镜像

MIRROR_BANNER = """// {out} —— 由 tools/abi_tool.py mirror 生成，**不要手改**
//
// 用途: 消费方需要"同一份虚表布局、但不要 dllimport"的声明（否则 HologramLib 会变成
// 加载期硬前置）。这份镜像由真头文件逐行转换而来，槽位序因此不可能漂移:
//   python tools/abi_tool.py mirror --namespace-macro MEOW_HOLOGRAM_ABI_NAMESPACE --out HologramLibAbi.h
//   python tools/abi_tool.py check -H HologramLibAbi.h    # 与基线对拍，槽位序应完全一致
//
// 转换只做两件事: 命名空间换成宏、HOLOGLIB_API 置空。其余逐字保留。
#pragma once
"""

EXPORT_BLOCK_RE = re.compile(
    r"#ifdef\s+HOLOGLIB_EXPORTS\s*"
    r"\n#define\s+HOLOGLIB_API\s+__declspec\(dllexport\)\s*"
    r"\n#else\s*"
    r"\n#define\s+HOLOGLIB_API\s+__declspec\(dllimport\)\s*"
    r"\n#endif"
)

MIRROR_EXPORT_REPLACEMENT = (
    "// 导出宏置空: 消费方只想要布局，不想留下导入项（否则库变成加载期硬前置）\n"
    "#ifndef HOLOGLIB_API\n#define HOLOGLIB_API\n#endif"
)


def make_mirror(text, namespace_macro, out_name):
    """真头文件 -> 消费方镜像。只换命名空间与导出宏，其余逐字保留。"""
    mirror, n_ns = re.subn(
        r"^namespace\s+hologramlib\s*\{",
        "namespace " + namespace_macro + " {",
        text,
        count=1,
        flags=re.M,
    )
    if n_ns != 1:
        raise SystemExit("MIRROR-ERROR: 找不到 `namespace hologramlib {`")

    mirror, n_exp = EXPORT_BLOCK_RE.subn(MIRROR_EXPORT_REPLACEMENT, mirror, count=1)
    if n_exp != 1:
        raise SystemExit("MIRROR-ERROR: 找不到 HOLOGLIB_API 的 dllexport/dllimport 分支")

    return MIRROR_BANNER.format(out=out_name) + mirror


def cmd_mirror(args):
    global HEADER
    with open(HEADER, "r", encoding="utf-8") as f:
        text = f.read()
    out = make_mirror(text, args.namespace_macro, os.path.basename(args.out))
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    print(f"[ok] 写出镜像 {args.out}（命名空间宏 {args.namespace_macro}）")

    # 自证: 镜像的槽位表必须与真头文件逐槽相同
    real = extract()
    saved, HEADER = HEADER, os.path.abspath(args.out)
    try:
        mirrored = extract()
    finally:
        HEADER = saved
    a = json.dumps(real["interfaces"], sort_keys=True)
    b = json.dumps(mirrored["interfaces"], sort_keys=True)
    if a != b:
        print("[FAIL] 镜像与真头文件的槽位表不一致")
        return 1
    total = sum(len(v) for v in real["interfaces"].values())
    print(f"[ok] 槽位表自证一致（{total} 槽 / {len(real['interfaces'])} 接口）")
    return 0


def main():
    ap = argparse.ArgumentParser(description="HologramLib ABI 契约工具")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("extract", help="抽出当前头文件的契约快照")
    p.add_argument("-H", "--header", help="待解析的头文件（默认 include/hologramlib/HologramLib.h）")
    p.add_argument("--json-out")
    p.add_argument("--md-out")
    p.set_defaults(func=cmd_extract)

    p = sub.add_parser("check", help="与 abi/ 基线比对（CI 用）")
    p.add_argument("-H", "--header", help="待解析的头文件（默认 include/hologramlib/HologramLib.h）")
    p.add_argument("--manifest", help="要校验的 manifest（默认仓库根 manifest.json）")
    p.add_argument("--baseline")
    p.add_argument("--write-snapshot", action="store_true", help="通过后把当前快照写成新基线")
    p.add_argument("--release", help="新基线文件名里的版本号（默认沿用）")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("mirror", help="生成消费方镜像头（同布局、无 dllimport）")
    p.add_argument("-H", "--header", help="源头文件（默认 include/hologramlib/HologramLib.h）")
    p.add_argument(
        "--namespace-macro",
        default="MEOW_HOLOGRAM_ABI_NAMESPACE",
        help="替代 hologramlib 的宏名（消费方自用改名）",
    )
    p.add_argument("--out", required=True, help="输出路径")
    p.set_defaults(func=cmd_mirror)

    p = sub.add_parser("diff", help="打印两份快照的差异")
    p.add_argument("-H", "--header", help="待解析的头文件")
    p.add_argument("--a")
    p.add_argument("--b")
    p.set_defaults(func=cmd_diff)

    args = ap.parse_args()
    if getattr(args, "header", None):
        globals()["HEADER"] = os.path.abspath(args.header)
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()
