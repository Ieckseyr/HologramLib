#pragma once

namespace debugshape_export {

/**
 * RemoteCallExporter - 导出形状渲染 API 给 LSE（协议层）
 *
 * 经 LseBridge 在统一命名空间 "HologramLib" 下导出（shape* 前缀域）,
 * LSE 脚本经 ll.import("HologramLib", "shapeCreateText") 等调用。
 *
 * 所有渲染走协议层发包（BinaryStream 序列化）。
 */
class RemoteCallExporter {
public:
    /**
     * Export all functions to LegacyRemoteCall
     * Should be called when plugin enables
     */
    static void exportAll();

private:
    static void exportCreateFunctions();
    static void exportPropertyFunctions();
    static void exportDisplayFunctions();
    static void exportLifecycleFunctions();
};

} // namespace debugshape_export
