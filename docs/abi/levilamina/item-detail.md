# 1.4 IItemDetail · 物品详情

完整签名（`#include "HologramLib/HologramLib.h"`）：

```cpp
// 在 (x,y,z) 显示 "本地化物品名 xN"（自动翻译; count<=1 无数量后缀）
// customText 非空则完全替代自动文本（支持 § 颜色码与 {变量}）
// 返回悬浮字 ID（可用 holograms() 继续精修）
int64_t show(int dimId, float x, float y, float z,
             std::string const            std::string const& itemId, int aux, int count, itemId, int aux, int count,
             std::string const            std::string const& customText = ""); customText = "");
bool hide(int64_t id);
```

物品名解析链：`ItemRegistry（HashedString）→ ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → 兜底原样显示 ID（不阻断）。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
