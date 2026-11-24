# 参与开发

纯前置库：只提供机制，不做业务。动手前先看 [docs/design-contracts.md](docs/design-contracts.md)。

## 构建环境

- Visual Studio 2022（MSVC 19.44+）
- xmake 2.9+
- BedrockProtocol 的 `install/` 目录（自行构建，不在本仓库）

```powershell
$env:HOLO_PROTOCOL_DIR = 'D:\deps\BedrockProtocol\install'   # 协议库
$env:HOLO_REPO_DIR     = "$env:LOCALAPPDATA\.xmake\repositories\liteldev-repo"  # 可选
```

```powershell
xmake f -c --holo_werror=y -y
xmake build -y
```

## CI 变量

`build` / `release` 两个 job 都靠仓库 Variables 提供外部依赖（设置 → Actions → Variables）：

| 变量 | 内容 |
|---|---|
| `HOLO_PROTOCOL_URL` | BedrockProtocol `install/` 的 zip 直链 |
| `HOLO_LL_SDK_URL` | 离线 LL SDK 包的 zip 直链，由 `python tools/pack_llsdk.py` 产出 |

`pack_llsdk.py` 从本机 xmake 缓存里抓 `levilamina 1.9.7` 的头/库，连同 `prelink.exe`、
`bedrock_runtime_data` 合并成一份 `include/ lib/ tools/`，CI 解包后设 `HOLO_LL_SDK_DIR`，
`xmake.lua` 见到它就跳过 `add_requires("levilamina")`（不再联网现编 LL 和 bedrockdata）。
两个变量缺任一，编译/发布整关跳过。

## 提交前

```powershell
xmake f -c --holo_werror=y -y ; xmake build -y      # 零警告
cd tests ; .\check-trade-offers.bat                 # 其余见 tests/README.md
python tools/abi_tool.py check
clang-format -i <改过的文件>
```

## 提交粒度

一次提交只做一件事，能单独 review、单独 revert。

单独成提交：
- 纯格式化、改名、文件搬移（不夹带逻辑）
- 生成产物（abi 基线、schema 快照）与产生它的代码
- 版本号与 changelog（发版提交）

第一行写做了什么，正文写为什么；涉及协议结论时贴证据来源。

## 代码约定

- 异常用标准 C++（`set_exceptions("cxx")`）。不要加 `/EHa`，会和它冲突。
- 只在类尾追加虚函数，中段插入会被 `abi_tool.py check` 挡下。
- `HOLOGLIB_API_VERSION` 只在正式发布新能力时推高。
- 正式产物不带日志；排障时 `xmake f --holo_diag=y`，发布前复位。
- 锁内不调游戏 API（发包、查玩家、取 level）。回环回调会自死锁。
- 修 bug 时先补一条会失败的对拍，再改实现。
- **严禁资源包**：不往库里放 RP/BP，不让库依赖或代发资源包；效果只能用协议包 + 客户端原生资源。

## 发版

1. `manifest.json` 的 `version` = tag 名
2. 需要时推高 `HOLOGLIB_API_VERSION`
3. `docs/versions/changelog.md` + `.en.md` 写条目
4. `docs/versions/api-version-matrix.md` + `.en.md` 补行
5. 跑完上面的检查，出包，打 tag
