# 表单控制（IFormControl）

关闭客户端当前打开的表单。表单的创建由 LSE、LeviLamina Form API 或 MeowMenu 负责。

协议：`ClientboundCloseForm`。

脚本用法见[表单控制](../lse/form.md)。

## 接口

```cpp
IFormControl& formControl();

bool closeForm(std::string const& playerName); // 玩家不在线返回 false
int  closeAllForms();                          // 返回发送数量
```

## 说明

- 不区分表单来源，客户端当前打开的表单都会被关闭。
- 玩家未打开表单时调用也返回 `true`。
- 只影响客户端显示，服务端不记录表单状态。
