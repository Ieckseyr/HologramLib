"""用仓库里缓存的 GitHub 凭据发布 Release（不打印 token）。

    python tools/publish_release.py                    # 默认读 release/ 下的资产，版本取 manifest.json
    python tools/publish_release.py --tag v26.51.8 --version 26.51.8
    python tools/publish_release.py --draft             # 先传成草稿，人工检查后点发布

资产（按版本号命名，放 release/ 目录）：
  LeviLamina-HologramLib-<v>.zip
  Endstone-HologramLib-<v>-py3-none-win_amd64.whl
  HologramLib-pdb.zip

token 来源优先级：环境变量 GH_TOKEN / GITHUB_TOKEN → git 凭据助手
（manager / manager-core / Windows 版 GCM 的绝对路径）。
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
RELEASE = ROOT / "release"
OWNER, REPO = "Ieckseyr", "HologramLib"

# git credential helper 候选；带 ! 前缀的是「按 shell 命令执行」，用于带空格的绝对路径
HELPERS = (
    None,
    "manager",
    "manager-core",
    '!"C:/Program Files/Git/mingw64/bin/git-credential-manager.exe"',
    '!"C:/Program Files (x86)/Git Credential Manager/git-credential-manager.exe"',
)


def token() -> str:
    env = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if env:
        return env
    for helper in HELPERS:
        cmd = ["git"]
        if helper:
            cmd += ["-c", f"credential.helper={helper}"]
        cmd += ["credential", "fill"]
        proc = subprocess.run(
            cmd,
            input="protocol=https\nhost=github.com\n\n",
            capture_output=True,
            text=True,
        )
        if proc.returncode != 0:
            continue
        for line in proc.stdout.splitlines():
            if line.startswith("password="):
                return line.split("=", 1)[1]
    raise SystemExit("no credential（配 GH_TOKEN，或登录一次 GitHub 让凭据助手缓存）")


def api(method: str, url: str, tok: str, data: bytes | None = None, ctype: str = "application/json"):
    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("Authorization", f"Bearer {tok}")
    req.add_header("Accept", "application/vnd.github+json")
    if data is not None:
        req.add_header("Content-Type", ctype)
    with urllib.request.urlopen(req, timeout=180) as resp:
        body = resp.read()
        return json.loads(body) if body else {}


def main() -> int:
    manifest = json.loads((ROOT / "manifest.json").read_text(encoding="utf-8"))
    parser = argparse.ArgumentParser(description="发布 HologramLib Release")
    parser.add_argument("--version", default=manifest["version"], help="发布版本（默认取 manifest.json）")
    parser.add_argument("--tag", default=None, help="git tag（默认 v<version>）")
    parser.add_argument("--notes", default=str(RELEASE / "RELEASE_NOTES.md"), help="Release 正文文件")
    parser.add_argument("--draft", action="store_true", help="发成草稿")
    args = parser.parse_args()

    version = args.version
    tag = args.tag or f"v{version}"
    notes_path = pathlib.Path(args.notes)
    body = notes_path.read_text(encoding="utf-8") if notes_path.is_file() else f"HologramLib {version}"

    assets = [
        f"LeviLamina-HologramLib-{version}.zip",
        f"Endstone-HologramLib-{version}-py3-none-win_amd64.whl",
        "HologramLib-pdb.zip",
    ]

    tok = token()
    payload = json.dumps(
        {"tag_name": tag, "name": version, "body": body, "draft": bool(args.draft)}
    ).encode()
    existing = None
    try:
        existing = api("GET", f"https://api.github.com/repos/{OWNER}/{REPO}/releases/tags/{tag}", tok)
    except urllib.error.HTTPError as exc:
        if exc.code != 404:
            print(f"[release] 查询失败 HTTP {exc.code}")
            return 1
    if existing:
        rel = api(
            "PATCH",
            f"https://api.github.com/repos/{OWNER}/{REPO}/releases/{existing['id']}",
            tok,
            json.dumps({"name": version, "body": body, "draft": bool(args.draft)}).encode(),
        )
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

    for name in assets:
        path = RELEASE / name
        if not path.is_file():
            print(f"[release] 缺文件 {name}")
            continue
        data = path.read_bytes()
        try:
            info = api(
                "POST",
                f"{upload_url}?name={urllib.parse.quote(name)}",
                tok,
                data,
                "application/octet-stream",
            )
        except urllib.error.HTTPError as exc:
            print(f"[release] 上传 {name} 失败 HTTP {exc.code}: {exc.read().decode('utf-8', 'replace')[:200]}")
            continue
        print(f"[release] 上传 {name} -> {info.get('size', 0)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
