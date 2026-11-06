# 1.3 IHologramText · 悬浮字

完整签名（`#include "HologramLib/HologramLib.h"`）：

整块多行文本单一背景框渲染（`\n` 合并所有行）；变更后调用 `refresh` 原地重绘（同 networkId 覆盖，无闪烁）。行索引 0 起。
**1.26.0 收缩后口径**：行只负责文本；颜色/缩放/背景框/穿墙/旋转都是**整块**属性 —— 行级样式差异请在文本内嵌 § 颜色代码。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create | `(float x, float y, float z) -> int64_t` | 创建 |
| | destroy | `(int64_t id) -> bool` | 销毁 |
| | destroyAll | `() -> void` | 全部销毁 |
| 行管理 | addLine | `(int64_t id, std::string const& text) -> bool` | 追加行 |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | 行文本 |
| | setLinePool | `(int64_t id, int line, std::vector<std::string> const& content, int intervalMs) -> bool` | 动态行（1.26.0）: 内容池按 `intervalMs` **时间取模轮播**（无状态, 多个浮字同池同相）; 池 ≤1 项 / 间隔≤0 = 不轮播; 空 = 清除池 |
| | setLineParseVariables | `(int64_t id, int line, bool enabled) -> bool` | 该行是否解析变量（默认 true; 关 = `{}` 按字面量显示）（1.26.0） |
| | removeLine | `(int64_t id, int line) -> bool` | 移除行 |
| | clearLines | `(int64_t id) -> bool` | 清空行 |
| | getLineCount | `(int64_t id) -> int` | 行数 |
| 颜色 | setColor | `(int64_t id, r, g, b, a: float) -> bool` | 整块文字颜色 |
| | setBackgroundColor | `(int64_t id, r, g, b, a: float) -> bool` | 背景框颜色（不设 = 客户端默认色; 1.26.0） |
| | clearBackgroundColor | `(int64_t id) -> bool` | 清除背景框颜色（1.26.0） |
| 样式 | setScale | `(int64_t id, float scale) -> bool` | 整块缩放（1.26.0; 替代原行级 setLineScale） |
| | setDepthTest | `(int64_t id, bool enabled) -> bool` | 穿墙可见性（1.26.0; true = 被方块/实体遮挡） |
| | setRotation | `(int64_t id, pitch, yaw, roll: float) -> bool` | 三轴固定朝向（度; 不再面向相机; 1.26.0） |
| | clearRotation | `(int64_t id) -> bool` | 恢复面向相机（1.26.0） |
| 位置 | setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | 跟随玩家（位置在调用时就地解析, 无自驱） |
| | clearFollowPlayer | `(int64_t id) -> bool` | 取消跟随 |
| | setDimension | `(int64_t id, int dimId) -> bool` | 迁移维度：已绘制时同步底层形状维度并按原绘制目标原地重发（无闪烁） |
| 显示 | draw / drawToDimension / drawToPlayer / remove | 同形状语义 | |
| | refresh | `(int64_t id) -> bool` | 重解析变量/跟随坐标并原地重发 |

> **1.26.0 收缩（明记）**：`setLineScale` / `setLineColor` / `setLineGradient` / `setLineRainbow` /
> `setLineScroll` / `setVerticalAnimation` / `setLineSpacing` 与 `tick` 已移除 —— 这些在"整块单形状"
> 模型下不可实现或空转。原因与替代（§ 颜色代码 / setScale）见 [更新日志](../../versions/changelog.md)
> 与 [版本与 ABI 对照](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)。
> **C++ ABI**：它们保留为**废弃空槽**（非纯虚 + 内联空实现 + `[[deprecated]]`，调用返回 `false` / 无动作），
> 虚表槽位与 1.25.0 一致 —— 未重编译的旧消费方二进制按旧槽位调用不会错位；LSE 侧导出照旧删除。

> **动态行（1.26.0；重构自 [Phantom](https://github.com/GroupMountain/Phantom)，LGPL-3.0，已按其许可标注来源）**：`setLinePool` = 内容池按 `intervalMs` **时间取模轮播**
> （无状态，多个浮字同池同相）；`setLineParseVariables` = 行级变量开关（默认开）。轮播/变量刷新由库内
> **0.5s 节流**驱动、**内容变了才重发**（区别于本版移除的自驱"偏移"——那时每拍都在动、但驱出来的东西
> 没有消费方）。变量集：`{player}` `{online}` `{time}` `{tps}` `{dimension}` `{x}` `{y}` `{z}`。**含 `{var}` 的文本按观看者各一份形状**（逐玩家解析），其刷新会逐玩家
> 原地重发（同 networkId，无闪烁）。

内置变量：`{time}` `{online}` `{tps}` `{player}` `{dimension}` `{x}` `{y}` `{z}`，全部在库内解析。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
