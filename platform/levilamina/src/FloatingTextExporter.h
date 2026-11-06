#pragma once

namespace debugshape_export {

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
