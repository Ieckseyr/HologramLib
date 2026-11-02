// BossBarManager.cpp - Boss 条
#include "BossBarManager.h"

#include "../anchor/AnchorPool.h"
#include "core/PacketSink.h"
#include "core/VirtualActorIds.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/packet/BossEventPacket.hpp>

#include <format>

namespace debugshape_export {

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

} // namespace

BossBarManager& BossBarManager::getInstance() {
    static BossBarManager instance;
    return instance;
}

std::int64_t BossBarManager::nextTargetId() {
    // 虚拟号段里自增（不参与 ghost 路由，服务端没有对应实体）
    static std::atomic<std::uint64_t> counter{core::actorIds::bossBar.first};
    return static_cast<std::int64_t>(counter.fetch_add(1, std::memory_order_relaxed));
}

bool BossBarManager::send(::Player& player, Bar const& bar, int eventType, std::int64_t /*playerId*/) {
    // 手写线格式（26.51 BDS 的 BossEventPacketPayload）:
    //   varint64 bossId | byte eventType | string name | string filteredName | float percent | byte color | byte overlay
    // 规矩：本库**一律手写线格式**，不构造 BDS 的 Packet 对象（项目约定）。
    // 不用 sculk 的 BossEventPacket 的原因: 它按老格式多写一个 varint64 playerID，
    // 26.51 的 payload 里没有这个字段 → 字节错位，客户端不认。
    ::BinaryStream stream;
    stream.writeVarInt64(bar.targetId, nullptr, nullptr);
    stream.writeByte(static_cast<uchar>(eventType), nullptr, nullptr);
    stream.writeString(bar.title, nullptr, nullptr);
    stream.writeString(bar.title, nullptr, nullptr); // filtered name：没有过滤就用同一份
    stream.writeFloat(bar.percent, nullptr, nullptr);
    stream.writeByte(static_cast<uchar>(static_cast<int>(bar.color)), nullptr, nullptr);
    stream.writeByte(static_cast<uchar>(static_cast<int>(bar.overlay)), nullptr, nullptr);

    // 补包头（与 wire::encodeChecked 同一套：包 id | 标志位）
    ::BinaryStream sendStream;
    sendStream.writeUnsignedVarInt((74 & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12), nullptr, nullptr);
    sendStream.mBuffer.append(stream.mBuffer.data(), stream.mBuffer.size());

    return wire::sendRaw(player, sendStream.mBuffer);
}

int64_t BossBarManager::add(
    std::string const&          playerName,
    std::string const&          title,
    float                       healthPercent,
    hologramlib::BossBarColor   color,
    hologramlib::BossBarOverlay overlay) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return -1;

    // 锚点实体：客户端认的 boss 条要对应一个它知道的实体，
    // 所以在虚空里放一只"没有模型的掉落物"当锚点（和 NPC 对话菜单的载体同一思路）。
    std::int64_t const anchor = AnchorPool::getInstance().acquire(static_cast<int>(player->getDimensionId()));
    if (anchor < 0)
        return -1;

    Bar bar;
    bar.playerName = playerName;
    bar.targetId   = anchor;
    bar.title      = title;
    bar.percent    = healthPercent < 0.0f ? 0.0f : (healthPercent > 1.0f ? 1.0f : healthPercent);
    bar.color      = color;
    bar.overlay    = overlay;

    auto const playerUid = static_cast<std::int64_t>(player->getOrCreateUniqueID().rawID);

    // 原版是两步：Add 建条（发给所有人），PlayerAdded 把某名玩家加进这条（发给他）。
    // 只发 Add 时客户端可能不建条 —— 实测没显示就是这个原因。
    if (!send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Add), 0)) {
        return -1;
    }
    // 原版流程: Add 建条 → PlayerAdded 把收件人加进这条。少第二步时客户端可能不显示。
    send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::PlayerAdded), 0);

    std::lock_guard lock(mMutex);
    bar.id = static_cast<int64_t>(bar.targetId); // 对外 id 直接用虚拟 actor id，简且唯一
    mBars.emplace(bar.id, bar);
    return bar.id;
}

int64_t BossBarManager::addOn(
    std::string const&          playerName,
    std::int64_t                targetId,
    std::string const&          title,
    float                       healthPercent,
    hologramlib::BossBarColor   color,
    hologramlib::BossBarOverlay overlay) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return -1;

    Bar bar;
    bar.id         = targetId;
    bar.playerName = playerName;
    bar.targetId   = targetId;
    bar.title      = title;
    bar.percent    = healthPercent < 0.0f ? 0.0f : (healthPercent > 1.0f ? 1.0f : healthPercent);
    bar.color      = color;
    bar.overlay    = overlay;

    if (!send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Add), 0))
        return -1;
    send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::PlayerAdded), 0);

    std::lock_guard lock(mMutex);
    mBars[bar.id] = bar;
    return bar.id;
}

bool BossBarManager::setTitle(int64_t id, std::string const& title) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.title = title;
        copy             = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdateName), 0);
}

bool BossBarManager::setHealth(int64_t id, float healthPercent) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.percent = healthPercent < 0.0f ? 0.0f : (healthPercent > 1.0f ? 1.0f : healthPercent);
        copy               = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdatePercent), 0);
}

bool BossBarManager::setStyle(int64_t id, hologramlib::BossBarColor color, hologramlib::BossBarOverlay overlay) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.color   = color;
        it->second.overlay = overlay;
        copy               = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdateStyle), 0);
}

bool BossBarManager::remove(int64_t id) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        copy = it->second;
        mBars.erase(it);
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return true; // 人已下线，客户端那边早没了

    // 锚点实体跟着这条 Boss 条一起回收
    AnchorPool::getInstance().release(copy.targetId);
    send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::PlayerRemoved), 0);
    send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Remove), 0);
    return true;
}

void BossBarManager::removeAll(std::string const& playerName) {
    std::vector<Bar> victims;
    {
        std::lock_guard lock(mMutex);
        for (auto it = mBars.begin(); it != mBars.end();) {
            if (it->second.playerName == playerName) {
                victims.push_back(it->second);
                it = mBars.erase(it);
            } else {
                ++it;
            }
        }
    }
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return;
    for (auto const& bar : victims) {
        send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Remove), 0);
    }
}

int BossBarManager::count(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    int             n = 0;
    for (auto const& [id, bar] : mBars) {
        if (bar.playerName == playerName)
            ++n;
    }
    return n;
}

std::string BossBarManager::probeWire() const {
    auto build = [](bool optionalFiltered) {
        ::BinaryStream body;
        body.writeVarInt64(0x71000001, nullptr, nullptr);
        body.writeByte(static_cast<uchar>(0), nullptr, nullptr); // Add
        body.writeString("Probe", nullptr, nullptr);
        if (optionalFiltered) {
            // 布局 B: Name 是 RedactableString → string + optional<string>
            body.writeBool(true, nullptr, nullptr);
            body.writeString("Probe", nullptr, nullptr);
        } else {
            // 布局 A: Name + FilteredName 两个平铺字符串（协议文档的写法）
            body.writeString("Probe", nullptr, nullptr);
        }
        body.writeFloat(0.5f, nullptr, nullptr);
        body.writeByte(static_cast<uchar>(5), nullptr, nullptr); // Purple
        body.writeByte(static_cast<uchar>(0), nullptr, nullptr); // Progress
        return body.mBuffer;
    };

    std::string const a = build(false);
    std::string const b = build(true);
    return std::format(
        "A(两个平铺字符串) valid={} bytes={} | B(string+optional) valid={} bytes={}",
        wire::validateBody(74, a) ? 1 : 0,
        a.size(),
        wire::validateBody(74, b) ? 1 : 0,
        b.size());
}

std::string BossBarManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto            it = mBars.find(id);
    if (it == mBars.end())
        return "not_found";
    auto const& bar = it->second;
    return std::format(
        "id={} player={} target={} title='{}' pct={:.3f} color={} overlay={}",
        bar.id,
        bar.playerName,
        bar.targetId,
        bar.title,
        bar.percent,
        static_cast<int>(bar.color),
        static_cast<int>(bar.overlay));
}

void BossBarManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    for (auto it = mBars.begin(); it != mBars.end();) {
        if (it->second.playerName == playerName)
            it = mBars.erase(it);
        else
            ++it;
    }
}

void BossBarManager::shutdown() {
    std::lock_guard lock(mMutex);
    mBars.clear();
}

} // namespace debugshape_export
