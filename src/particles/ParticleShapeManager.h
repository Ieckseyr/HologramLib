// ParticleShapeManager.h - 通用协议层粒子形状系统
#pragma once

#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <mc/platform/UUID.h>

namespace debugshape_export {

class ParticleShapeManager {
public:
    enum class Kind : int {
        Point = 0,   // 单点
        Line,        // 线段（两端点, step 采样）
        Rect,        // 矩形环线（边框）
        Plane,       // 填充平面网格
        Box,         // 长方体线框（12 边）
        BoxFaces,    // 长方体六面填充
        Poly,        // 自定义多面体（顶点 + 边）
    };

    // 形状会话（id 为句柄; 全部字段可运行时变更）
    struct Shape {
        int64_t     id{};
        Kind        kind{};
        std::string playerName;    // 创建者（日志用）

        // 几何定义（局部坐标, 锚点为原点）
        // Line: a-b 两端点; Rect/Plane: w×h 平面（axis 决定法向, w/h 沿平面两轴）
        // Box/BoxFaces: half 三轴半尺寸; Poly: verts + edges
        float ax{}, ay{}, az{}, bx{}, by{}, bz{}; // 局部端点（Line）
        float w{}, h{};                            // Rect/Plane 尺寸
        int   axis{};                              // 平面法向轴: 0=XY 1=YZ 2=XZ
        float hx{}, hy{}, hz{};                    // Box 半尺寸
        std::vector<float>        polyVerts;       // Poly 顶点 (x,y,z)×N
        std::vector<std::int32_t> polyEdges;       // Poly 边 (i,j)×M（顶点索引对）
        float step{1.0f};                          // 采样步长（格）

        // 变换（世界系）
        float px{}, py{}, pz{};                    // 锚点（世界坐标）
        float rotX{}, rotY{}, rotZ{};              // 欧拉角（度, ZYX 序, 绕锚点）
        float scale{1.0f};
        float spinX{}, spinY{}, spinZ{};           // 自旋速率（度/tick）

        // 跟随（最高优先级: 生效时锚点 = 玩家位置 + offset, setPos 被覆盖）
        mce::UUID followUuid;
        std::string followUuidStr;                 // 空 = 不跟随
        float offX{}, offY{}, offZ{};

        // moveTo 平滑移动（点对点动画; 锚点每 tick easeOutCubic 插值逼近目标）
        bool          animActive{};
        float         animFromX{}, animFromY{}, animFromZ{};
        float         animToX{},   animToY{},   animToZ{};
        std::uint64_t animStartTick{};
        int           animDurationTicks{};        // >0; 到达后 animActive=false

        // 渲染
        std::string effect{"minecraft:endrod"};
        std::set<std::string> visiblePlayers;      // 白名单（uuid 字符串; 空 = 全员）
        int   viewDistance{96};                    // 格; 0 = 不裁剪
        int   intervalTicks{4};                    // 重发周期
        int   dimId{};                             // 创建时维度（跟随/全员可见按此过滤）

        // 生命周期
        std::uint64_t endTick{};                   // 0 = 永久
        std::uint64_t nextEmitTick{};
        bool visibleAll{true};                     // 白名单空 = true

        // 发送帧模板缓存（预序列化静态部分; 坐标 12B 运行期补丁）
        // frameTpl = [varuint 包头][包体(marker 坐标)]; framePosOff = 坐标偏移
        std::string  frameTpl;
        std::size_t  framePosOff{static_cast<std::size_t>(-1)}; // -1 = 需重建

        // 发射缓存（采样结果, 几何/step 未变时复用）
        bool          cacheValid{false};
        std::vector<float> localPts;               // 局部采样点 (x,y,z)×N
    };

    static ParticleShapeManager& getInstance();

    void init();
    void shutdown();

    // ── 创建（世界坐标定义, 返回 id; <0 = 失败）──
    int64_t createPoint(
        std::string const& playerName, int dimId,
        float x, float y, float z,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createLine(
        std::string const& playerName, int dimId,
        float x1, float y1, float z1, float x2, float y2, float z2, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createRect( // 矩形环线: 中心 + 尺寸 + 平面法向轴
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createPlane( // 填充平面: 同 rect 参数
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float w, float h, int axis, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createBox( // 长方体线框: 中心 + 半尺寸
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createBoxFaces(
        std::string const& playerName, int dimId,
        float cx, float cy, float cz, float hx, float hy, float hz, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    int64_t createPoly( // 多面体: 顶点串 "x,y,z;x,y,z;..." + 边串 "i-j;i-j..."
        std::string const& playerName, int dimId,
        std::string const& vertsCsv, std::string const& edgesCsv, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );
    // vector 版（C++ ABI 路径; verts=(x,y,z)*N 本地拷贝, edges=(i,j)*M）
    int64_t createPoly(
        std::string const& playerName, int dimId,
        std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step,
        std::string const& effect, int intervalTicks, int lifetimeTicks
    );

    // ── 运行时控制（id 无效返回 false; 采样缓存按需失效）──
    bool setPos(int64_t id, float x, float y, float z);       // 平移（粒子移动）
    bool moveBy(int64_t id, float dx, float dy, float dz);
    bool moveTo(int64_t id, float x, float y, float z, int durationTicks); // 平滑点对点移动
    bool setRot(int64_t id, float rx, float ry, float rz);     // 欧拉角（度）
    bool spin(int64_t id, float sx, float sy, float sz);       // 自旋（度/tick; 0,0,0 停止）
    bool setScale(int64_t id, float s);
    bool follow(int64_t id, std::string const& uuid, float offX, float offY, float offZ);
    bool unfollow(int64_t id);
    bool setEffect(int64_t id, std::string const& effect);
    bool setVisible(int64_t id, std::string const& playersCsv); // 逗号分隔 uuid; 空 = 全员
    // vector 白名单（C++ ABI 路径; 空列表 = 形状所在维度全员）
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids);
    bool clearVisiblePlayers(int64_t id);
    bool setInterval(int64_t id, int ticks);
    bool setViewDistance(int64_t id, int blocks);
    bool setLifetime(int64_t id, int ticks);                   // 从现在起延长/缩短

    bool destroy(int64_t id);
    void destroyAll();                                          // 停服清理
    bool exists(int64_t id) const;                              // 查询 id 是否在用
    std::vector<int64_t> getAllIds() const;                     // 全部形状 id
    std::string getInfo(int64_t id) const;                      // 诊断探针

    // tick 驱动（Level::$tick hook 调用; 无形状时空转）
    void tick();

private:
    int64_t insertShape(Shape&& s);
    static void sampleLocal(Shape& s);                          // 局部点采样（写入 localPts）
    bool emitShape(Shape& s, std::uint64_t now);                // 单形状发射; false = 应移除
    bool buildFrameTemplate(Shape& s);                          // 预序列化帧模板（marker 定位坐标偏移）

    mutable std::mutex                       mMutex;
    std::unordered_map<int64_t, Shape>       mShapes;
    int64_t                                  mNextId{1};
    bool                                     mInitialized{false};
};

} // namespace debugshape_export
