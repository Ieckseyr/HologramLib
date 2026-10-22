// diag_marker.cpp - 诊断开关极性检查用的小翻译单元（只编译成 .obj, 不链接）
//
// 校验: tests/check-no-diagnostics.bat + check-no-diagnostics.py
//   关闭诊断（默认）时 "[HOLOGLIB-DIAG-MARKER]" 不得出现在目标文件里;
//   显式 -DHOLOGLIB_DIAG_LOG=1 时必须出现。
#include "DiagLog.h"

int main() {
    HLIB_LOG_INFO("[HOLOGLIB-DIAG-MARKER] info {}", 1);
    HLIB_LOG_WARN("[HOLOGLIB-DIAG-MARKER] warn {}", 2);
    HLIB_LOG_ERROR("[HOLOGLIB-DIAG-MARKER] error {}", 3);
    HLIB_LOG_DEBUG("[HOLOGLIB-DIAG-MARKER] debug {}", 4);
    return 0;
}
