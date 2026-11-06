"""用仓库里缓存的 GitHub 凭据发布 Release（不打印 token）。

资产：
  LeviLamina-HologramLib-<v>.zip
  Endstone-HologramLib-<v>-py3-none-win_amd64.whl
  HologramLib-pdb.zip
"""

from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
RELEASE = ROOT / "release"
OWNER, REPO = "Ieckseyr", "HologramLib"
TAG = "v26.60.2"
VERSION = "26.60.2"

ASSETS = [
    f"LeviLamina-HologramLib-{VERSION}.zip",
    f"Endstone-HologramLib-{VERSION}-py3-none-win_amd64.whl",
    "HologramLib-pdb.zip",
]


def token() -> str:
    out = subprocess.run(
        ["git", "credential", "fill"],
        input="protocol=https\nhost=github.com\n\n",
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    for line in out.splitlines():
        if line.startswith("password="):
            return line.split("=", 1)[1]
    raise SystemExit("no credential")


def api(method: str, url: str, tok: str, data: bytes | None = None, ctype: str = "application/json"):
    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("Authorization", f"Bearer {tok}")
    req.add_header("Accept", "application/vnd.github+json")
    if data is not None:
        req.add_header("Content-Type", ctype)
    with urllib.request.urlopen(req, timeout=120) as resp:
        body = resp.read()
        return json.loads(body) if body else {}


def main() -> int:
    tok = token()
    body = (RELEASE / "RELEASE_NOTES.md").read_text(encoding="utf-8")

    payload = json.dumps({"tag_name": TAG, "name": VERSION, "body": body, "draft": False}).encode()
    existing = None
    try:
        existing = api("GET", f"https://api.github.com/repos/{OWNER}/{REPO}/releases/tags/{TAG}", tok)
    except urllib.error.HTTPError as exc:
        if exc.code != 404:
            print(f"[release] 查询失败 HTTP {exc.code}")
            return 1
    if existing:
        rel = api("PATCH", f"https://api.github.com/repos/{OWNER}/{REPO}/releases/{existing['id']}",
                  tok, json.dumps({"name": VERSION, "body": body, "draft": False}).encode())
        for asset in rel.get("assets", []):
            api("DELETE", f"https://api.github.com/repos/{OWNER}/{REPO}/releases/assets/{asset['id']}", tok)
        print(f"[release] 已更新既有 Release {rel['html_url']}")
    else:
        try:
            rel = api("POST", f"https://api.github.com/repos/{OWNER}/{REPO}/releases", tok, payload)
        except urllib.error.HTTPError as exc:
            detail = exc.read().decode("utf-8", "replace")[:300]
            print(f"[release] 创建失败 HTTP {exc.code}: {detail}")
            return 1
        print(f"[release] 已创建 {rel['html_url']}")

    upload_url = rel["upload_url"].split("{")[0]

    for name in ASSETS:
        path = RELEASE / name
        if not path.is_file():
            print(f"[release] 缺文件 {name}")
            continue
        data = path.read_bytes()
        try:
            info = api("POST", f"{upload_url}?name={urllib.parse.quote(name)}", tok, data,
                       "application/octet-stream")
        except urllib.error.HTTPError as exc:
            print(f"[release] 上传 {name} 失败 HTTP {exc.code}: {exc.read().decode('utf-8', 'replace')[:200]}")
            continue
        print(f"[release] 上传 {name} -> {info.get('size', 0)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
