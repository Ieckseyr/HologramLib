#pragma once

namespace debugshape_export {

/**
 * RemoteCallExporter - Export DebugShape functions to LSE (Protocol Layer)
 * 
 * Uses LegacyRemoteCall mechanism to export all functions under "DebugShape" namespace,
 * enabling LSE script plugins to call these functions via ll.import().
 * 
 * All rendering is done via protocol layer packet sending (BinaryStream serialization).
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
