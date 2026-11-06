#pragma once

// 【重构来源标注 · LGPL-3.0】
// 本文件的「动态行」实现（FloatingTextLine 的内容池 / 轮播间隔 / 行级变量开关，以及
// FloatingTextManager 里按间隔取模轮播、0.5s 节流刷新与逐玩家重发的部分）重构自 Phantom：
//   项目: https://github.com/GroupMountain/Phantom
//   许可: GNU Lesser General Public License v3.0（LGPL-3.0）
//   版权: Phantom 贡献者（GroupMountain）
// 依 LGPL-3.0 §4 在此标注来源；对应许可全文见 LICENSES/LGPL-3.0.txt 与 LICENSES/GPL-3.0.txt，
// 该重构部分及其演绎作品按 LGPL-3.0 分发，本库其余部分仍为 MIT（见 LICENSE）。

#include "core/Registry.h"

class Player;

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace debugshape_export {

struct Color4f {
    float r, g, b, a;

    Color4f()
        : r(1.0f)
        , g(1.0f)
        , b(1.0f)
        , a(1.0f) {}
    Color4f(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_) {}
};

struct FloatingTextLine {
    std::string              text;
    std::vector<std::string> pool;
    int                      poolIntervalMs{0};
    bool                     parseVariables{true};
};

struct FloatingText {
    int64_t id;
    float   x, y, z;
    int     dimId = 0;

    std::vector<FloatingTextLine> lines;

    Color4f                color{1.0f, 1.0f, 1.0f, 1.0f};
    float                  scale = 1.0f;
    std::optional<Color4f> backgroundColor;
    bool                   depthTest   = false;
    bool                   useRotation = false;
    float                  rotPitch = 0.0f, rotYaw = 0.0f, rotRoll = 0.0f;

    std::string followPlayer;
    float       followOffsetY = 2.0f;

    enum class DrawTarget { None,
                            All,
                            Dimension,
                            Player };
    DrawTarget  drawTarget = DrawTarget::None;
    std::string targetPlayer;

    int64_t textShapeId = -1;
    bool    isDrawn     = false;

    std::unordered_map<std::string, int64_t> viewerShapes;
};

class FloatingTextManager {
public:
    static FloatingTextManager& getInstance();

    FloatingTextManager(const FloatingTextManager&)            = delete;
    FloatingTextManager& operator=(const FloatingTextManager&) = delete;

    int64_t create(float x, float y, float z);

    bool destroy(int64_t id);
    void destroyAll();

    bool addLine(int64_t id, const std::string& text);

    bool setLineText(int64_t id, int lineIndex, const std::string& text);

    bool removeLine(int64_t id, int lineIndex);

    bool clearLines(int64_t id);

    int getLineCount(int64_t id);

    bool setColor(int64_t id, float r, float g, float b, float a = 1.0f);

    bool setScale(int64_t id, float scale);

    bool setBackgroundColor(int64_t id, float r, float g, float b, float a = 1.0f);
    bool clearBackgroundColor(int64_t id);

    bool setDepthTest(int64_t id, bool enabled);

    bool setRotation(int64_t id, float pitch, float yaw, float roll);
    bool clearRotation(int64_t id);

    bool setLinePool(int64_t id, int lineIndex, const std::vector<std::string>& content, int intervalMs);

    bool setLineParseVariables(int64_t id, int lineIndex, bool enabled);

    bool setLocation(int64_t id, float x, float y, float z);

    bool setDimension(int64_t id, int dimId);

    bool setFollowPlayer(int64_t id, const std::string& playerName, float offsetY = 2.0f);

    bool clearFollowPlayer(int64_t id);

    bool draw(int64_t id);
    bool drawToDimension(int64_t id, int dimId);
    bool drawToPlayer(int64_t id, const std::string& playerName);

    bool remove(int64_t id);

    bool refresh(int64_t id);

    using VariableProvider = std::function<std::string(const std::string& playerName)>;
    void registerVariable(const std::string& name, VariableProvider provider);

    void registerBuiltinVariables();

    void onPlayerJoin(Player& player);
    void initDynamicDriver();
    void shutdownDynamicDriver();

private:
    FloatingTextManager();
    ~FloatingTextManager() = default;

    FloatingText* getFloatingText(int64_t id);

    void rebuildTextShape(FloatingText& ft);

    void destroyTextShape(FloatingText& ft);

    bool redrawTextShape(FloatingText& ft);

    [[nodiscard]] static bool textHasVariables(FloatingText const& ft);

    void rebuildTextShapesPerViewer(FloatingText& ft);

    void destroyViewerShapes(FloatingText& ft);

    void resolveFollowPosition(FloatingText& ft);

    void        applyBlockStyle(int64_t shapeId, FloatingText const& ft);
    std::string processVariables(const std::string& text, const std::string& playerContext);

    void tickDynamic();

    [[nodiscard]] static bool hasDynamicLines(FloatingText const& ft);

    std::string composeLine(FloatingTextLine const& line, const std::string& playerContext, std::uint64_t nowMs);

    std::string composeText(FloatingText const& ft, const std::string& playerContext, std::uint64_t nowMs);

    core::Registry<FloatingText, core::NoLock>        mFloatingTexts;
    std::unordered_map<std::string, VariableProvider> mVariables;

    std::unordered_map<int64_t, std::string> mLastSentText;
    std::uint64_t                            mDynamicTickCounter = 0;

    std::atomic<double>        mTps{20.0};
    std::atomic<std::uint64_t> mLastTickMs{0};
    void                       sampleTps();

    std::recursive_mutex mMutex;
};

} // namespace debugshape_export
