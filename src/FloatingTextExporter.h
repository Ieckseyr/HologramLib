#pragma once

namespace debugshape_export {

/**
 * FloatingTextExporter - 导出高级悬浮字 API 给 LSE
 *
 * 统一命名空间 "HologramLib"（holo* 前缀域）
 * 
 * 功能:
 * - 多行文本，每行独立配置
 * - 颜色渐变/彩虹效果
 * - 滚动/弹跳动画
 * - 跟随玩家
 * - 动态变量
 */
class FloatingTextExporter {
public:
    static void exportAll();
    
private:
    static void exportCreateFunctions();
    static void exportLineFunctions();
    static void exportColorFunctions();
    static void exportAnimationFunctions();
    static void exportDisplayFunctions();
};

} // namespace debugshape_export
