# 字节级对拍

C++ fixture 构造包，Python 逐字段和 BDS 抓包比对。

## 怎么跑

```powershell
cd tests
.\check-trade-offers.bat        # offers 与抓包逐字节一致
.\check-trade-packet.bat        # 13 项
.\check-trade-settlement.bat    # 43 项
.\check-container-packets.bat
.\check-view-override.bat       # 59 项
.\check-npc-carrier.bat         # 32 项
.\check-fake-inventory.bat
.\check-no-diagnostics.bat      # 正式产物不含日志字符串
```

退出码 0 = 通过。

## LSE 冒烟测试

`tests/lse/HoloBoxUITest/` 是虚拟容器（箱子界面）的脚本侧测试，指令版：

- `/boxui` 跑 22 条断言，然后开一个 54 格界面供手动点
- `/boxtest` 只跑断言
- `/boxstop` 关闭全部界面并清空点击队列

装法：把 `HoloBoxUITest.js` 与 `manifest.json` 放进服务器的 `plugins/HoloBoxUITest/`，重启后进服敲指令。
结果看控制台的 `[PASS]` / `[FAIL]`。

不连服务器也能验脚本逻辑：

```powershell
node tests/lse/HoloBoxUITest/mock_run.js
```

`mock_run.js` 用假的 `ll` / `mc` / `logger` 跑整份脚本，`mc.newCommand` 也一起模拟，覆盖指令注册、
清理、点击分发、`menuId` 对不上、缺前置等 9 个场景。

## 需要

- MSVC（bat 里用 vswhere 找，自动 vcvars64）
- BedrockProtocol 的 `install/`（fixture 要链 `Protocol.lib`）
- Python 3.12+
- `check-authinput.bat` 要本机抓包 `logs/authinput.hex`，没有就跳过

产物落在 `work/`，被 .gitignore 忽略。

## 加检查的规矩

- 一条断言只测一件事，失败信息带期望和实际
- 期望值优先取真实抓包；抓包不在仓库就把关键字节写进 `.py` 并注明来源
- 只在本机能跑的检查，文件头注明缺什么就跳过
- 修 bug 先加会失败的断言
