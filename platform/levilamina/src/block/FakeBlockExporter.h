#pragma once

namespace debugshape_export {

// 假方块（只发给客户端，不动服务端世界）
class FakeBlockExporter {
public:
    static void exportAll();
};

} // namespace debugshape_export
