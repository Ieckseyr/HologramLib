# 表单控制（IFormControl）

库不生成表单 —— 只发 `ClientboundCloseForm` 把客户端当前打开的表单收起来。

脚本侧见 [表单控制](../lse/form.md)。

## 接口

```cpp
IFormControl& formControl();

bool closeForm(std::string const& playerName); // 玩家不在线 = false
int  closeAllForms();                          // 返回发给了几个在线玩家
```

## 协议

`ClientboundCloseForm`（走 `player->sendNetworkPacket`，BDS 的包类只能用这条通道）。

## 边界

- **不检查表单是谁开的**：客户端开着就关，不管是 LSE、别的插件还是原版界面。
- 玩家没开表单时调用也返回 `true`，客户端不报错，只是没东西可关。
- 纯客户端表现：库不记录"谁打开了什么"，也不改动服务端状态。
- 表单本身仍归 LSE / LeviLamina Form API / MeowMenu —— 这里只有"关"这一个动作。
