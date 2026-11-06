#pragma once

namespace debugshape_export {

class RemoteCallExporter {
public:
    static void exportAll();

private:
    static void exportCreateFunctions();
    static void exportPropertyFunctions();
    static void exportDisplayFunctions();
    static void exportLifecycleFunctions();
};

} // namespace debugshape_export
