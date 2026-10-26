// InfoExporter.h - 库自身的元信息导出（26.51.6）
//
// 只回答"这个 DLL 是哪一版、虚表戳是多少、脚本侧该怎么查"。
// 脚本导入层（lse/HologramLib.js）拿它做运行时版本核对，消费方拿它做兼容判断。
#pragma once

namespace debugshape_export {

class InfoExporter {
public:
    static void exportAll();
};

} // namespace debugshape_export
