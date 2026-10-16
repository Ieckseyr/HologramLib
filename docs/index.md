<div align="center">
  <img src="assets/HologramLib.svg" alt="HologramLib — 基岩版协议层统一悬浮显示库" width="100%">
</div>

# HologramLib 文档


**HologramLib** 是基岩版服务端（LeviLamina / BDS）的协议层统一悬浮显示库：把 18 个「往客户端塞东西」的能力域收进一个纯前置插件，同时供 C++ 插件与 LSE 脚本调用。

> 当前：插件 26.51.3 · API 1.29.0（`0x012300`）· LeviLamina 26.51 / BDS 1.26.51 / 协议 2193

---

## 先定位你是哪种读者

| 你是 | 从这开始 | 然后 |
|---|---|---|
| 第一次用这个库 | [快速开始](getting-started.md)：构建、部署、跑通第一个功能 | [能力域总览](domains/README.md) 挑你要的能力 |
| 要把某个能力接进自己的插件 | [能力域指南](domains/README.md)：每个域怎么用、有哪些实测坑 | 对应域的 [C++ 接口](reference/cpp-api.md) / [LSE 接口](reference/lse-api.md) 查签名 |
| 决定自己该写在哪一侧 | [设计说明](design-contracts.md)：接口演进、库与消费方的边界 | [架构总览](architecture/overview.md) 看这套约定的落地方式 |
| 写 LSE 脚本 | [LSE 接口参考](reference/lse-api.md)：16 个前缀域的完整函数清单 | [能力域指南](domains/README.md) 里的脚本侧示例 |
| 查 LSE 标准接口 | [LSE 文档站](https://lse.levimc.org/zh/apis/)：`mc` / `ll` / `logger` / 事件 / NBT / 表单 / 数据库… | 本库只提供 `HologramLib` 命名空间下的导出 |
| 遇到渲染不对 / 包发不出去 | [协议层实现笔记](architecture/protocol-notes.md)：线格式、tick、抓包验证 | [能力域指南](domains/README.md) 里该域的「实测」小节 |
| 排查版本 / ABI 崩溃 | [版本与 ABI 对照](versions/api-version-matrix.md) | [架构总览](architecture/overview.md) 的 ABI 约束 |
| 维护这个库 / 提交代码 | [设计说明](design-contracts.md) | [设计说明](https://github.com/Ieckseyr/HologramLib/blob/main/README.md#设计说明) · [架构总览](architecture/overview.md) · [协议层实现笔记](architecture/protocol-notes.md) |

---

## 目录结构

```
docs/
├── index.md                       本页：文档导航中心
├── getting-started.md             构建 / 部署 / 第一个功能
├── design-contracts.md            设计说明：什么能进库、什么留在消费方
├── domains/                       按能力域讲「怎么用 + 实测坑」
│   ├── README.md                    域总览与选型决策
│   ├── rendering.md                 形状 / 悬浮字 / 渐变线 / 物品详情 / 物品悬浮 / 粒子
│   ├── entities-npc.md              自定义实体 / Ghost / 假玩家 NPC / 硫磺立方体
│   ├── interfaces.md                交易菜单 / 虚拟容器 / 背包虚容器 / NPC 对话框
│   └── view-sensing.md              客户端视图覆盖 / 感知域 / 逐客户端渲染
├── reference/                     查签名：接口兼容性的逐方法说明
│   ├── cpp-api.md                   C++ 侧接口的完整签名
│   └── lse-api.md                   LSE 侧 16 个前缀域的完整函数清单
├── architecture/                  理解内部：为什么这么设计
│   ├── overview.md                  分层、目录、重发策略、自驱盘点
│   └── protocol-notes.md            线格式、tick、发包原语、抓包验证
├── versions/
│   ├── changelog.md                 逐版本改了什么
│   └── api-version-matrix.md        三套编号对照 + ABI 事故记录
├── internal/                      ⚠ 维护者本地文档，不入库、不上站
│   ├── design-conventions.md        库的边界：什么能进库（原名 设计约定.md）
│   ├── architecture-cost.md         架构与开销评估（原名 架构与开销评估.md）
│   └── lse-exports.md               由源码统计出的权威导出清单（原名 LSE导出清单.md）
└── assets/HologramLib.svg         头图
```

`docs/internal/` 是本地文档，禁止提交 GitHub。它被 `.gitignore` 排除、被 `mkdocs.yml` 的 `exclude_docs` 挡在站点之外，`.github/workflows/docs.yml` 里还有一道 CI 守卫会在它被误提交时让构建失败。因此上面表格里没有指向 `internal/` 的链接：那些页面在公开仓库里并不存在。

---

## 读文档前该知道的事

1. 「能力域」= 一个可独立启用的能力面。每个域在 C++ 侧是一个接口（`IShapeDrawer`、`IPlayerNpc`…），在 LSE 侧是一个函数名前缀（`shape*`、`playerNpc*`…）。域之间可以独立使用，只有少数有依赖（如硫磺立方体完全委托给 `ICustomEntity` 实现）。

2. 描述里的版本号是「该能力出现的最低 API 版本」，用来写 `static_assert` 门。三套编号（发布版本 / API 版本 / `HOLOGLIB_API_VERSION`）的关系见 [版本与 ABI 对照](versions/api-version-matrix.md)。

3. 「消费方」= 调用这个库的插件。本库不落盘、不做业务、不替消费方决定何时重发；数据持久化、命令、权限、表单都在消费方。这条边界决定了遇到「库为什么不帮我存这个」时的答案，见 [设计说明](design-contracts.md) 与 [README · 设计说明](https://github.com/Ieckseyr/HologramLib/blob/main/README.md#设计说明)。

---

## 术语表

| 术语 | 含义 |
|---|---|
| 能力域 / 域 | 一组相关 API 的集合（如「视图覆盖」域）。C++ 侧一个接口，LSE 侧一个前缀 |
| 消费方 | 调用 HologramLib 的插件（C++ 或 LSE），如 MeowHolographicRenderer、MSkinventory、ZXPanel |
| 前置库 | 本库的定位：只提供机制，不含业务；消费方 `enable()` 时直接 `getInstance()`，无需声明依赖 |
| 只拦不发 | 视图覆盖域的限制：出站钩子只决定「这一包发不发」，从不改引擎包字段 |
| 时机通知 | 库发给消费方的回调（如实体对玩家 spawn 完成），由消费方决定补发什么 |
| 接口兼容性 | `IHologramLib` 及各域接口只在类尾部追加虚函数，既有签名与语义永不变 |
| 虚表槽位 | 纯虚函数在 C++ 虚表中的位置。中段插入/删除会让预编译消费方静默错位 |
| ABI 布局戳 | 头文件携带的虚表布局描述符 + 消费方编译期自检，库侧运行期拦截布局不一致的消费方 |
| 明记收缩 | 已发布 API 的删除（按约定该走大版本），在版本表里逐条写清原因与迁移方式 |
| 载体实体 | 界面域用到的隐身实体（NPC 对话框 / 交易菜单），界面关闭即删除，不落存档 |

---

## 文档的版本真值来源

本套文档里的版本号与函数计数以代码为准，不以后人补写的散文为准：

| 事实 | 真值来源 |
|---|---|
| 插件发布版本 | `manifest.json` 的 `version` |
| API 版本 / `HOLOGLIB_API_VERSION` | `include/hologramlib/HologramLib.h` |
| LSE 导出函数清单与计数 | 源码里的 `exportAs(NAMESPACE, ...)` 调用 —— 逐文件统计见 `docs/internal/lse-exports.md`（本地文档） |
| C++ 接口签名 | `include/hologramlib/HologramLib.h` |

发现文档与代码不一致时，以代码为准并提一个 issue。本套文档在整理时已修正若干历史遗留的版本号与函数计数错误（API 版本停留在 1.27.0、`view*` 计数少 2、`trade*` / `npcDialog*` 清单漏列等），差异记录见[更新日志](versions/changelog.md)。
