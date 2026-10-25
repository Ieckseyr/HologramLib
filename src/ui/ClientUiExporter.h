// ClientUiExporter.h - 表单控制 / 方位条 的 LSE 导出 + 公开接口适配
#pragma once

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

class FormControlExporter {
public:
    static void exportAll();
};

class LocatorBarExporter {
public:
    static void exportAll();
};

hologramlib::IFormControl& formControlAdapter();
hologramlib::ILocatorBar&  locatorBarAdapter();

} // namespace debugshape_export
