#pragma once

namespace debugshape_export {

/**
 * FloatingTextExporter - 导出高级悬浮字 API 给 LSE
 *
 * 统一命名空间 "HologramLib"（holo* 前缀域）
 *
 * 功能:
 * - 多行文本（整块合并为单一文本形状）
 * - 整块样式: 颜色 / 缩放 / 背景框颜色 / 穿墙开关 / 三轴旋转
 * - 跟随玩家（位置在调用时就地解析, 无库内自驱）
 * - 动态变量
 */
class FloatingTextExporter {
public:
    static void exportAll();
    
private:
    static void exportCreateFunctions();
    static void exportLineFunctions();
    static void exportColorFunctions();
    static void exportStyleFunctions();
    static void exportDisplayFunctions();
};

} // namespace debugshape_export
