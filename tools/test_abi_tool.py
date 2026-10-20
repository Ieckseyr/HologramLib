#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_abi_tool.py —— abi_tool.py 的自测（不需要 BDS、不需要编译器）

跑法:  python tools/test_abi_tool.py
覆盖:  通过 / 中段插入 / 尾部追加(抬版本) / 尾部追加(未抬版本) / 结构体变更 /
       枚举值变更 / 破坏槽位但布局戳没抬 / 消费方镜像生成与自证
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "abi_tool.py")
HEADER = os.path.join(ROOT, "include", "hologramlib", "HologramLib.h")

failures = []


def run(*args):
    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    return subprocess.run(
        [sys.executable, TOOL, *args], capture_output=True, text=True, encoding="utf-8", env=env
    )


def expect(name, cond, detail=""):
    if cond:
        print(f"  PASS  {name}")
    else:
        print(f"  FAIL  {name}  {detail}")
        failures.append(name)


def write_variant(tmp, name, text):
    path = os.path.join(tmp, name)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    return path


def main():
    with open(HEADER, encoding="utf-8", newline="") as f:
        original = f.read()

    print("[1] 当前头文件 vs 基线")
    r = run("check")
    expect("基线通过", r.returncode == 0, r.stdout + r.stderr)

    tmp = tempfile.mkdtemp(prefix="holo-abi-test-")

    print("[2] 中段插入虚函数 → 必须失败")
    anchor = "    virtual bool setScale(int64_t id, float scale)                        = 0;"
    assert anchor in original
    mid = original.replace(anchor, anchor + "\n    virtual bool evilMidInsert(int64_t id) = 0;", 1)
    r = run("check", "-H", write_variant(tmp, "mid.h", mid))
    expect("退出码 1", r.returncode == 1, f"got {r.returncode}")
    expect("指出槽号", re.search(r"槽 #\d+ 变了", r.stdout) is not None, r.stdout)

    print("[3] 尾部追加 + 抬版本 → 通过并列出槽位")
    last = "    virtual bool     removeActorInteractListener(uint64_t token) = 0;"
    assert last in original
    appended = original.replace("#define HOLOGLIB_API_VERSION 0x012500", "#define HOLOGLIB_API_VERSION 0x012600", 1)
    appended = appended.replace(last, last + "\n    virtual bool appendOnlyProbe(int64_t id) = 0;", 1)
    r = run("check", "-H", write_variant(tmp, "append.h", appended))
    expect("退出码 0", r.returncode == 0, r.stdout + r.stderr)
    expect("报告新增槽", "+1 个尾部槽" in r.stdout, r.stdout)

    print("[4] 尾部追加但没抬 API 版本 → 通过 + 警告")
    no_bump = original.replace(last, last + "\n    virtual bool appendOnlyProbe(int64_t id) = 0;", 1)
    r = run("check", "-H", write_variant(tmp, "append_nobump.h", no_bump))
    expect("退出码 0", r.returncode == 0, r.stdout + r.stderr)
    expect("提示未抬版本", "HOLOGLIB_API_VERSION 未抬" in r.stdout, r.stdout)

    print("[5] 结构体布局变更 → 必须失败")
    field = "    std::string playerName;"
    assert field in original
    struct_changed = original.replace(field, field + "\n    int extraField;", 1)
    r = run("check", "-H", write_variant(tmp, "struct.h", struct_changed))
    expect("退出码 1", r.returncode == 1, f"got {r.returncode}")
    expect("指出结构体", "结构体布局变了" in r.stdout, r.stdout)

    print("[6] 枚举值变更 → 必须失败")
    m = re.search(r"enum class ShapeType[^{]*\{([^}]*)\}", original, re.S)
    assert m
    body = m.group(1)
    first = body.strip().split(",")[0]
    enum_changed = original.replace(first, "Text = 99" if "Text" in first else first, 1)
    r = run("check", "-H", write_variant(tmp, "enum.h", enum_changed))
    expect("退出码 1", r.returncode == 1, f"got {r.returncode}")
    expect("指出枚举", "枚举值变了" in r.stdout, r.stdout)

    print("[7] 破坏槽位 + 布局戳 +1 → 仍然失败（破坏改动要走大版本流程）")
    breaking_stamped = mid.replace("HOLOGLIB_ABI_STAMP = 1", "HOLOGLIB_ABI_STAMP = 2", 1)
    r = run("check", "-H", write_variant(tmp, "mid_stamped.h", breaking_stamped))
    expect("退出码 1", r.returncode == 1, f"got {r.returncode}")
    expect("不再报戳未抬", "HOLOGLIB_ABI_STAMP 未抬" not in r.stdout, r.stdout)

    print("[8] 消费方镜像生成 + 槽位自证")
    mirror_out = os.path.join(tmp, "HologramLibAbi.h")
    r = run("mirror", "--namespace-macro", "TEST_NS", "--out", mirror_out)
    expect("退出码 0", r.returncode == 0, r.stdout + r.stderr)
    expect("自证一致", "槽位表自证一致" in r.stdout, r.stdout)
    with open(mirror_out, encoding="utf-8") as f:
        mirror_text = f.read()
    expect("命名空间已宏化", "namespace TEST_NS {" in mirror_text, "")
    expect("没有 dllimport", "__declspec(dllimport)" not in mirror_text, "")

    print()
    if failures:
        print(f"ABI-TOOL-TEST: {len(failures)} 项失败: {', '.join(failures)}")
        return 1
    print("ABI-TOOL-TEST: 全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
