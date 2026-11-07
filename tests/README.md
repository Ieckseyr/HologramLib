# 字节级对拍

C++ fixture 构造包，Python 逐字段和 BDS 抓包比对。

## 怎么跑

协议迁移（sculk → EndstoneMC/bedrock-protocol）后，旧的 C++ 字节对拍夹具已全部删除：
它们钉住的是旧线格式，而 1.26.30+ 多个包的 wire 已变化（PlayerList/皮肤/BossEvent 等），
继续保留只会误导。字节一致性现在由 `core/` 的编码器对拍保证（见 `core/proto/Version.h` 注释与发布记录）。

下面只剩纯标准库单测。

## 不依赖 BDS 的测试

四个纯标准库的单测，CI 里用 `g++` 直接跑（Windows 本地用 clang++ 或 g++ 都行）：

```powershell
g++ -std=c++20 -pthread -O1 -I src tests/registry_test.cpp        -o registry_test        && ./registry_test
g++ -std=c++20 -O1          -I src tests/click_event_test.cpp     -o click_event_test     && ./click_event_test
g++ -std=c++20 -O1          -I src tests/shape_id_map_test.cpp    -o shape_id_map_test    && ./shape_id_map_test
g++ -std=c++20 -O1          -I src tests/visibility_logic_test.cpp -o visibility_logic_test && ./visibility_logic_test
```

| 测试 | 钉住什么 |
|---|---|
| `registry_test.cpp` | `core::Registry` 的 id 分配、索引清理、脏集、快照遍历、锁策略、并发 |
| `click_event_test.cpp` | 点击回传行的版式（`player=… menuId=… slot=… closed=…`）与解析往返。`menuId` 必须是打开容器时拿到的那个 id —— 26.51.6 漏回填时它恒为 0，就是这条断言能挡住的错 |
| `shape_id_map_test.cpp` | 脚本形状编号 0..10 与协议编号的双向映射（6 是历史空洞），两套编号不许混 |
| `visibility_logic_test.cpp` | `core::decideVisibility` 的语义：`R` 内生成、`R..R+4` 迟滞带保持、`R+4` 外移除；禁用/换维度/白名单外立刻移除不受迟滞；`hysteresis==0` 无迟滞带；`viewDistance<=0` 同维度全可见；生成预算不透支 |


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
