// PlayerVisibilityManager.h - 按玩家屏蔽出站包（26.51.6）
//
// 按被隐藏的玩家屏蔽出站数据包：出生、Tab、移动、状态、皮肤，或整体隐身。
//
// 判定顺序（一次发送只做一次）:
//   1. 没有任何条目 → 直接放行（零开销快路径）；
//   2. 认出这条包的"主体"（运行时 id 或 uuid 反查在线玩家）；
//   3. 主体没有条目 / 收件人就是主体本人（且没开 alsoSelf）→ 放行；
//   4. 按包的分类查掩码，命中就丢。
//
// 无法识别类型的数据包直接放行。
#pragma once

#include "hologramlib/HologramLib.h" // PlayerPacketMask / IPlayerVisibility

#include <mutex>
#include <string>
#include <unordered_map>

class Packet;
class Player;

namespace debugshape_export {

class PlayerVisibilityManager {
public:
    static PlayerVisibilityManager& getInstance();

    bool setMask(std::string const& playerName, hologramlib::PlayerPacketMask const& mask, bool alsoSelf);
    bool setHidden(std::string const& playerName, bool hidden);
    bool restore(std::string const& playerName);

    [[nodiscard]] hologramlib::PlayerPacketMask maskOf(std::string const& playerName) const;
    [[nodiscard]] bool                          isHidden(std::string const& playerName) const;

    // 出站钩子调用：这条包要不要因为"主体被屏蔽"而丢掉
    bool shouldDrop(::Player& recipient, ::Packet const& packet);

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    PlayerVisibilityManager() = default;

    struct Entry {
        hologramlib::PlayerPacketMask mask;
        bool                          alsoSelf{false};
        bool                          hidden{false};
    };

    [[nodiscard]] bool anyActive() const;
    // 借视图覆盖域的"撤销即补发真身"流程，把出生包重新发给所有观看者
    static void forceRespawn(std::string const& playerName);

    mutable std::mutex                     mMutex;
    std::unordered_map<std::string, Entry> mEntries;
};

} // namespace debugshape_export
