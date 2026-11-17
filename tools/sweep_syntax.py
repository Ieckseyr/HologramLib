#!/usr/bin/env python3
"""全量语法检查：用 xmake 的真实命令行（去掉 -c/-Fo/-Fd/-MD）把每个 .cpp 过一遍，
一次拿到完整错误面，避免 xmake 在第一批错误就停下。

用法: python .cache/sweep.py [输出日志路径] [--only 关键字]
"""
import glob
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLANG = r"C:\Program Files\LLVM\bin\clang-cl"
PKGS = r"C:\Users\MCMin\AppData\Local\.xmake\packages"

FLAGS = [
    "--target=x86_64-pc-windows-msvc",
    "-O0", "-std:c++latest", "-fsyntax-only",
    f"-I{PKGS}\\r\\rapidjson\\2025.02.05\\5aec2755925a45d9974d95d2cb7038b1\\include",
    "-Iplatform/levilamina/src/compat",
    "-Iplatform/levilamina/src",
    "-I.", "-Iinclude",
    "-DNOMINMAX", "-DUNICODE", "-DHOLOGLIB_EXPORTS", "-D_CRT_SECURE_NO_WARNINGS",
    "-DENTT_PACKED_PAGE=128", "-DENTT_SPARSE_PAGE=2048", "-DENTT_NO_MIXIN", "-DLL_PLAT_S",
    "/EHsc", "-external:W0",
    f"-external:I{os.path.join(os.path.dirname(ROOT), '..', '.llsdk', 'endstone-proto', 'install', 'include')}",
    f"-external:I{PKGS}\\l\\levilamina\\1.9.7\\e163be9f486448b785e9a2c72b5144b7\\include",
    f"-external:I{PKGS}\\e\\entt\\v3.15.0\\18a7c66ceb6a4245aa7ca5b637cbf536\\include",
    f"-external:I{PKGS}\\e\\expected-lite\\v0.8.0\\36ca3aab48a04473a0b2d34e01875b34\\include",
    f"-external:I{PKGS}\\f\\fmt\\11.2.0\\ee1902dd7cff4195a287949f80dbce70\\include",
    f"-external:I{PKGS}\\g\\gsl\\v4.2.0\\cdcbe579518f475dbbe3954d5b394a71\\include",
    f"-external:I{PKGS}\\g\\glm\\1.0.1\\18615f0387cd4e2ab2e9ea9965afc784\\include",
    f"-external:I{PKGS}\\l\\leveldb\\1.23\\2532d000dc9d47cbb618c80ba0e6d806\\include",
    f"-external:I{PKGS}\\s\\snappy\\1.2.2\\b3244272357b4994abb43dc0aa939d28\\include",
    f"-external:I{PKGS}\\m\\magic_enum\\v0.9.7\\03e2990032db4ea08102d17ff8a57da7\\include",
    f"-external:I{PKGS}\\m\\magic_enum\\v0.9.7\\03e2990032db4ea08102d17ff8a57da7\\include\\magic_enum",
    f"-external:I{PKGS}\\n\\nlohmann_json\\v3.11.3\\826582eb66794908a16a49dbdd33bec4\\include",
    f"-external:I{PKGS}\\r\\rapidjson\\v1.1.0\\541d06a92bbb4f8fac523353c2426c15\\include",
    f"-external:I{PKGS}\\t\\type_safe\\v0.2.4\\4b33a9a5c8154bd58b7d4007150f50b7\\include",
    f"-external:I{PKGS}\\d\\debug_assert\\v1.3.4\\e3515434506a445792c1c83600bcf899\\include",
    f"-external:I{PKGS}\\p\\pcg_cpp\\v1.0.0\\6c550fbfc1bb49fbbed872829f6090b0\\include",
    f"-external:I{PKGS}\\p\\pfr\\2.1.1\\45e5102591a649fda279e67eaf92482a\\include",
    f"-external:I{PKGS}\\p\\parallel-hashmap\\v1.3.12\\47b4dd3f84f34e9d98b4cde1eedee54d\\include",
    f"-external:I{PKGS}\\c\\concurrentqueue\\v1.0.4\\0cd923cdfb6a4256954a994422b17de9\\include",
    f"-external:I{PKGS}\\c\\concurrentqueue\\v1.0.4\\0cd923cdfb6a4256954a994422b17de9\\include\\concurrentqueue\\moodycamel",
    "/utf-8", "/W4", "-DNDEBUG",
]


def run(rel: str) -> str:
    proc = subprocess.run([CLANG, *FLAGS, rel], cwd=ROOT,
                          capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = (proc.stdout or "") + (proc.stderr or "")
    if not out.strip():
        return ""
    lines = []
    for line in out.splitlines():
        lines.append(f"{rel}: {line}")
    return "\n".join(lines)


def main() -> int:
    out_path = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else os.path.join(ROOT, ".cache", "syntax.log")
    only = ""
    for i, a in enumerate(sys.argv):
        if a == "--only" and i + 1 < len(sys.argv):
            only = sys.argv[i + 1]

    os.chdir(ROOT)
    files = sorted(glob.glob("core/**/*.cpp", recursive=True) + glob.glob("platform/levilamina/src/**/*.cpp", recursive=True))
    if only:
        files = [f for f in files if only in f]

    with ThreadPoolExecutor(max_workers=12) as pool:
        results = list(pool.map(run, files))

    text = "\n".join(r for r in results if r)
    with open(out_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)

    errs = text.count("error:")
    failed_files = len({r.split(":", 1)[0] for r in results if "error:" in r})
    print(f"files={len(files)} failed_files={failed_files} errors={errs} log={out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
