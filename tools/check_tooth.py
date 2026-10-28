#!/usr/bin/env python3
"""check_tooth.py —— 校验 tooth.json（Lip 包清单）与仓库实际产物一致。

`format_uuid` 是**格式标记，不是每个包各自的 UUID**。官方 schema 写的是常量：

    "format_uuid": { "type": "string", "const": "289f771f-2c9a-4d73-9f3f-8492495a924d" }

（`tools/schema/tooth.v3.schema.json`，取自 LiteLDev/lip 的 schemas/manifest.schema.json）
所以每个格式 3 的包都必须是这个值，随机一个会让清单直接无效。包的身份由 `tooth` 字段
（仓库路径）标识，不是 UUID。

Lip 按 tooth.json 下载发布包并解压到目标目录。这几处一旦对不上，用户 `lip install` 会装上
残缺的包（缺 lib/ 或落错目录），所以放进 CI：

  1. format_version / format_uuid 与官方 schema 的 const 一致
  2. 版本号与 manifest.json 一致
  3. 发布包文件名与 tools/pack_release.py 的产物一致
  4. 解压后的根目录、目标目录与实际布局一致（HologramLib/ → plugins/HologramLib）

用法：
  python tools/check_tooth.py
"""
from __future__ import annotations

import io
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOTH = os.path.join(ROOT, "tooth.json")
MANIFEST = os.path.join(ROOT, "manifest.json")
PACK = os.path.join(ROOT, "tools", "pack_release.py")

SCHEMA = os.path.join(ROOT, "tools", "schema", "tooth.v3.schema.json")

# 格式标记的合法值只认官方 schema 里的 const —— 不在这里手写一份，避免两处漂移
try:
    _schema = json.loads(io.open(SCHEMA, encoding="utf-8").read())
    _fmt_version = _schema["properties"]["format_version"]["const"]
    _fmt_uuid = _schema["properties"]["format_uuid"]["const"]
except Exception as _err:  # noqa: BLE001
    raise SystemExit(f"[tooth] 读不到 {SCHEMA}: {_err}")

EXPECTED = {
    "format_version": _fmt_version,
    "format_uuid": _fmt_uuid,
    "tooth": "github.com/Ieckseyr/HologramLib",
    "zip_name": "HologramLib.zip",
    "zip_root": "HologramLib/",
    "dest": "plugins/HologramLib",
    "platform": "win-x64",
}


def fail(msg: str) -> int:
    print("[tooth] " + msg)
    return 1


def main() -> int:
    if not os.path.isfile(TOOTH):
        return fail("缺少 tooth.json")

    try:
        tooth = json.loads(io.open(TOOTH, encoding="utf-8").read())
    except Exception as err:  # noqa: BLE001 - 逐条报出便于定位
        return fail(f"tooth.json 不是合法 JSON: {err}")

    manifest = json.loads(io.open(MANIFEST, encoding="utf-8").read())

    checks = [
        ("format_version", tooth.get("format_version"), EXPECTED["format_version"]),
        ("format_uuid", tooth.get("format_uuid"), EXPECTED["format_uuid"]),
        ("tooth", tooth.get("tooth"), EXPECTED["tooth"]),
        ("version（应与 manifest.json 一致）", tooth.get("version"), manifest.get("version")),
        ("info.name", (tooth.get("info") or {}).get("name"), manifest.get("name")),
    ]
    for label, got, want in checks:
        if got != want:
            return fail(f"{label} 不一致：tooth={got!r}，应为 {want!r}")

    variants = tooth.get("variants") or []
    if len(variants) != 1:
        return fail(f"variants 应为 1 个（仅服务端 win-x64），当前 {len(variants)} 个")

    variant = variants[0]
    if variant.get("label"):
        return fail("服务端变体不应带 label（带 label 的变体需要手动选择）")
    if variant.get("platform") != EXPECTED["platform"]:
        return fail(f"platform 应为 {EXPECTED['platform']}，当前 {variant.get('platform')!r}")

    deps = variant.get("dependencies") or {}
    if "github.com/LiteLDev/LeviLamina" not in deps:
        return fail("缺少 LeviLamina 依赖")

    assets = variant.get("assets") or []
    if len(assets) != 1 or assets[0].get("type") != "zip":
        return fail("assets 应为 1 个 zip")

    urls = assets[0].get("urls") or []
    if len(urls) != 1:
        return fail("assets.urls 应为 1 条")
    if not urls[0].endswith("/" + EXPECTED["zip_name"]):
        return fail(f"发布包文件名应为 {EXPECTED['zip_name']}，当前 {urls[0]!r}")

    placements = assets[0].get("placements") or []
    if len(placements) != 1:
        return fail("placements 应为 1 条")
    place = placements[0]
    if place.get("type") != "dir":
        return fail("placements.type 应为 dir")
    if place.get("src") != EXPECTED["zip_root"]:
        return fail(f"placements.src 应为 {EXPECTED['zip_root']}，当前 {place.get('src')!r}")
    if (place.get("dest") or "").replace("\\", "/").rstrip("/") != EXPECTED["dest"]:
        return fail(f"placements.dest 应为 {EXPECTED['dest']}，当前 {place.get('dest')!r}")

    # 打包脚本必须真的产出同名 zip、且根目录就是这个文件夹
    pack = io.open(PACK, encoding="utf-8").read()
    if f'MOD + ".zip"' not in pack and EXPECTED["zip_name"] not in pack:
        return fail("tools/pack_release.py 里找不到发布包文件名，打包脚本与 tooth.json 可能已脱节")
    if 'MOD + "/"' not in pack and EXPECTED["zip_root"] not in pack:
        return fail("tools/pack_release.py 里找不到压缩包根目录，打包脚本与 tooth.json 可能已脱节")

    print(
        "[tooth] 通过："
        f"{tooth['tooth']} {tooth['version']} → {EXPECTED['dest']}"
        f"（包里 {EXPECTED['zip_root']}，格式 {tooth['format_version']} 标记 {tooth['format_uuid']}）"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
