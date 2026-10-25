// FormControlManager.h - 客户端表单控制（26.51.6）
//
// 只做一件事: 发一条 ClientboundCloseForm 让客户端把当前打开的表单收起来。
// 库不生成表单 —— 表单归 LSE / LeviLamina Form API / MeowMenu 管。
#pragma once

#include "hologramlib/HologramLib.h" // IFormControl

#include <string>

namespace debugshape_export {

class FormControlManager {
public:
    static FormControlManager& getInstance();

    bool closeForm(std::string const& playerName);
    int  closeAllForms();

private:
    FormControlManager() = default;
};

} // namespace debugshape_export
