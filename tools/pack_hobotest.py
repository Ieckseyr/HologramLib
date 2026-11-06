"""把 HoloTestEndstone 打成 wheel 并可选部署（仅本地测试用，不发布）。

用法：
    python tools/pack_hobotest.py                     # 只打包
    python tools/pack_hobotest.py --deploy <plugins>  # 打包并装到服务端
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import pathlib
import shutil
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "platform" / "endstone" / "tests" / "hobotest"
DIST = ROOT / "dist"
NAME = "endstone_hobotest"
VERSION = "1.2.0"
TAG = "py3-none-win_amd64"


def record(digest: bytes, size: int) -> str:
    return f"{NAME}-{VERSION}.dist-info/RECORD,," if False else f"{base64.urlsafe_b64encode(digest).rstrip(b'=').decode()},{size}"


def build() -> pathlib.Path:
    DIST.mkdir(exist_ok=True)
    wheel = DIST / f"{NAME}-{VERSION}-{TAG}.whl"
    dist_info = f"{NAME}-{VERSION}.dist-info"
    files = ["__init__.py", "plugin.py", "cases.py"]

    with zipfile.ZipFile(wheel, "w", zipfile.ZIP_DEFLATED) as zf:
        for name in files:
            zf.write(SRC / name, f"hobotest/{name}")

        metadata = (
            "Metadata-Version: 2.1\n"
            f"Name: {NAME}\n"
            f"Version: {VERSION}\n"
            "Summary: HologramLib endstone 全量调用测试（仅本地）\n"
            "Requires-Python: >=3.11\n"
        )
        zf.writestr(f"{dist_info}/METADATA", metadata)
        zf.writestr(f"{dist_info}/WHEEL", "Wheel-Version: 1.0\nGenerator: holotest-packer\nRoot-Is-Purelib: true\nTag: py3-none-win_amd64\n")
        zf.writestr(f"{dist_info}/entry_points.txt", "[endstone]\nhobotest = hobotest:HoloTestPlugin\n")

        record_lines = []
        for name in files:
            data = (SRC / name).read_bytes()
            digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode()
            record_lines.append(f"hobotest/{name},sha256={digest},{len(data)}")
        for extra in ("METADATA", "WHEEL", "entry_points.txt"):
            record_lines.append(f"{dist_info}/{extra},,")
        record_lines.append(f"{dist_info}/RECORD,,")
        zf.writestr(f"{dist_info}/RECORD", "\n".join(record_lines) + "\n")
    return wheel


def deploy(wheel: pathlib.Path, plugins: pathlib.Path) -> None:
    site = plugins / ".local" / "Lib" / "site-packages"
    target = site / "hobotest"
    if target.exists():
        shutil.rmtree(target)
    for stale in site.glob("endstone_hobotest*"):
        shutil.rmtree(stale, ignore_errors=True) if stale.is_dir() else stale.unlink(missing_ok=True)
    with zipfile.ZipFile(wheel) as zf:
        for item in zf.namelist():
            if item.startswith("hobotest/"):
                dest = site / item
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(zf.read(item))
    print(f"[hobotest] 已部署到 {target}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--deploy", default="")
    args = ap.parse_args()
    wheel = build()
    print(f"[hobotest] wheel: {wheel}  ({wheel.stat().st_size} bytes)")
    if args.deploy:
        deploy(wheel, pathlib.Path(args.deploy))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
