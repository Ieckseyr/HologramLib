#include "npcdialog/NpcDialogueManager.h"
#include "kernel/EventIds.h"
#include "core/HookRegistry.h"

#include "DiagLog.h"
#include "kernel/PacketSink.h"

#include "customentity/CustomEntityManager.h"
#include "npcdialog/NpcCarrierPacket.h"
#include "view/ViewOverrideExporter.h"
#include "view/ViewOverrideLogic.h"

#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/NpcRequestPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <format>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/math/Vec2.hpp>
#include <sculk/protocol/codec/math/Vec3.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/NpcDialoguePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <vector>

namespace debugshape_export {

namespace {

Player* findPlayerByName(std::string const& name) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getRealName() == name) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

Player* findPlayerByNetworkId(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getNetworkIdentifier() == source) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

std::string jsonEscape(std::string const& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char const c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

std::string buildActionJson(std::vector<hologramlib::NpcDialogButton> const& buttons) {
    std::string json{"[\n"};
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        auto const& btn = buttons[i];
        std::string text;
        json += "   {\n";
        json += "      \"button_name\" : \"" + jsonEscape(btn.label) + "\",\n";
        json += "      \"data\" : [\n";
        for (std::size_t k = 0; k < btn.commands.size(); ++k) {
            json += "         {\n            \"cmd_line\" : \"" + jsonEscape(btn.commands[k]) + "\",\n            \"cmd_ver\" : 50\n         }";
            json += (k + 1 < btn.commands.size()) ? ",\n" : "\n";
            text += btn.commands[k] + "\n";
        }
        json += "      ],\n";
        json += "      \"mode\" : " + std::to_string(btn.mode) + ",\n";
        json += "      \"text\" : \"" + jsonEscape(text) + "\",\n";
        json += "      \"type\" : 1\n";
        json += (i + 1 < buttons.size()) ? "   },\n" : "   }\n";
    }
    json += "]\n";
    return json;
}

std::string resolveActionJson(hologramlib::NpcDialogSpec const& spec) {
    return spec.rawActionJson.empty() ? buildActionJson(spec.buttons) : spec.rawActionJson;
}

template <typename PacketT>
bool sendPacket(Player& player, PacketT const& packet) {

    return wire::sendBody(
        player,
        static_cast<std::uint32_t>(packet.getId()),
        wire26::body26(packet),
        wire::Reliability::Reliable,
        packet.getName(),
        false);
}

std::pair<std::uint64_t, std::uint64_t> sendCarrierSpawn(
    Player&                           player,
    std::uint64_t                     uniqueId,
    std::uint64_t                     runtimeId,
    hologramlib::NpcDialogSpec const& spec,
    std::string const&                actionJson) {
    auto const& pos      = player.getPosition();
    float const yaw      = player.getRotation().y;
    float const rad      = yaw * (std::numbers::pi_v<float> / 180.0f);
    auto const  position = sculk::protocol::Vec3{
        pos.x + std::sin(rad) * 5.0f,
        npcdialog::kCarrierY,
        pos.z - std::cos(rad) * 5.0f};

    std::string identifier = spec.carrierIdentifier;
    std::string title      = spec.npcName;
    if (!spec.avatarViewSpec.empty()) {
        auto const view = parseEntitySpec(spec.avatarViewSpec);
        if (!view.identifier.empty()) {
            auto const normalised = view::logic::normaliseType(view.identifier);
            if (!normalised.empty())
                identifier = normalised;
        }
        if (view.hasNametag && !view.nametag.empty())
            title = view.nametag;
    }

    sendPacket(
        player,
        npcdialog::buildCarrierAddActor(
            uniqueId,
            runtimeId,
            identifier,
            title,
            actionJson,
            position,
            yaw,
            spec.avatarSkinVariant));
    return {uniqueId, runtimeId};
}

} // namespace

namespace {

std::atomic<std::int64_t>  gNextCarrierUid{-850403524570LL};
std::atomic<std::uint64_t> gNextCarrierRid{0x6E200000ULL};
} // namespace

std::uint64_t NpcDialogueManager::nextCarrierUniqueId() {
    return static_cast<std::uint64_t>(gNextCarrierUid.fetch_sub(1));
}
std::uint64_t NpcDialogueManager::nextCarrierRuntimeId() {
    return gNextCarrierRid.fetch_add(1);
}

NpcDialogueManager& NpcDialogueManager::getInstance() {
    static NpcDialogueManager instance;
    return instance;
}

void NpcDialogueManager::removeCarrierEntity(Player& player, std::uint64_t uniqueId) {
    if (uniqueId == 0)
        return;
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
    sendPacket(player, packet);
}

std::uint64_t NpcDialogueManager::ensureCarrier(
    Player&                           player,
    std::string const&                playerName,
    hologramlib::NpcDialogSpec const& spec,
    std::string const&                actionJson) {
    bool         hadPrevious = false;
    std::int64_t previousUid = 0;
    {
        std::lock_guard lock(mMutex);
        auto            it = mCarriers.find(playerName);
        if (it != mCarriers.end()) {
            hadPrevious = true;
            previousUid = it->second.uniqueId;
            mCarriers.erase(it);
        }
    }
    // 拆旧载体要发包，必须在锁外
    if (hadPrevious)
        removeCarrierEntity(player, previousUid);

    auto const uid = nextCarrierUniqueId();
    auto const rid = nextCarrierRuntimeId();

    auto const actual = sendCarrierSpawn(player, uid, rid, spec, actionJson);

    {
        std::lock_guard lock(mMutex);
        mCarriers[playerName] = Carrier{actual.first, actual.second};
    }
    HLIB_LOG_INFO(
        "[NpcDialog] 载体已生成: player={} uid={} rid={} (合成载体, y=-66)",
        playerName,
        actual.first,
        actual.second);
    return actual.first;
}

void NpcDialogueManager::sendDialog(Player& player, Dialog const& dialog, bool open) {
    sculk::protocol::NpcDialoguePacket packet;
    packet.mNpcId      = dialog.carrierUniqueId;
    packet.mActionType = open ? sculk::protocol::NpcDialoguePacket::ActionType::Open
                              : sculk::protocol::NpcDialoguePacket::ActionType::Close;
    packet.mDialogue   = dialog.spec.dialogue;
    packet.mSceneName  = dialog.spec.sceneName;
    packet.mNpcName    = dialog.spec.npcName;
    packet.mActionJSON = resolveActionJson(dialog.spec);
    sendPacket(player, packet);
}

int64_t NpcDialogueManager::open(std::string const& playerName, hologramlib::NpcDialogSpec const& spec) {
    std::uint64_t dialog0Uid     = 0;
    std::uint64_t carrierRuntime = 0;
    auto*         player         = findPlayerByName(playerName);
    if (player == nullptr) {
        HLIB_LOG_WARN("[NpcDialog] 打开失败: 玩家 {} 不在线", playerName);
        return -1;
    }

    auto const actionJson = resolveActionJson(spec);
    HLIB_LOG_INFO("[NpcDialog] open 步骤1/5: 生成载体（合成 NPC·y=-66·仅该玩家可见）");
    dialog0Uid = ensureCarrier(*player, playerName, spec, actionJson);
    if (dialog0Uid == 0)
        return -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mCarriers.find(playerName);
        if (it == mCarriers.end())
            return -1;
        carrierRuntime = it->second.runtimeId;
    }
    HLIB_LOG_INFO("[NpcDialog] open 步骤2/5: 载体就绪 uid={} rid={}", dialog0Uid, carrierRuntime);

    std::uint64_t uniqueId  = dialog0Uid;
    std::uint64_t runtimeId = carrierRuntime;
    HLIB_LOG_INFO("[NpcDialog] open 步骤3/5: id pair uid={} rid={}", uniqueId, runtimeId);

    Dialog dialog;
    dialog.playerName = playerName;

    dialog.carrierUniqueId  = spec.npcUniqueIdOverride != 0
                                  ? static_cast<std::uint64_t>(spec.npcUniqueIdOverride)
                                  : uniqueId;
    dialog.carrierRuntimeId = runtimeId;
    dialog.spec             = spec;

    int64_t dialogId = 0;
    {
        std::lock_guard lock(mMutex);

        if (auto prev = mByPlayer.find(playerName); prev != mByPlayer.end()) {
            mDialogs.erase(prev->second);
        }
        dialogId = mDialogs.insert(std::make_unique<Dialog>(dialog));
        mDialogs.withObject(dialogId, [&](Dialog& d) {
            d.dialogId = dialogId;
        });
        mByPlayer[playerName] = dialogId;
    }

    HLIB_LOG_INFO("[NpcDialog] open 步骤4/5: 载体已定位（生成时已摆在身后 5 格）");
    HLIB_LOG_INFO("[NpcDialog] open 步骤5/5: 发包中 (buttons={})", spec.buttons.size());
    sendDialog(*player, dialog, true);
    if (spec.npcUniqueIdOverride != 0) {
        HLIB_LOG_INFO("[NpcDialog] 使用覆盖的 NPC uniqueId={}（诊断模式）", spec.npcUniqueIdOverride);
    }
    HLIB_LOG_INFO(
        "[NpcDialog] 已打开: player={} dialogId={} scene='{}' buttons={} npcName='{}'",
        playerName,
        dialogId,
        spec.sceneName,
        spec.buttons.size(),
        spec.npcName);
    return dialogId;
}

bool NpcDialogueManager::setAvatar(int64_t dialogId, int skinVariant, std::string const& avatarViewSpec) {
    std::string                playerName;
    hologramlib::NpcDialogSpec spec;
    {
        std::lock_guard lock(mMutex);
        if (!mDialogs.withObject(dialogId, [&](Dialog const& d) {
                playerName = d.playerName;
                spec       = d.spec;
            })) {
            return false;
        }
    }
    spec.avatarSkinVariant = skinVariant;
    spec.avatarViewSpec    = avatarViewSpec;
    return open(playerName, spec) >= 0;
}

bool NpcDialogueManager::update(int64_t dialogId, hologramlib::NpcDialogSpec const& spec) {
    Dialog copy;
    {
        std::lock_guard lock(mMutex);
        if (!mDialogs.withObject(dialogId, [&](Dialog& d) {
                d.spec    = spec;
                d.touched = true;
                copy      = d;
            })) {
            return false;
        }
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr)
        return false;

    sendDialog(*player, copy, true);
    HLIB_LOG_INFO(
        "[NpcDialog] 就地换内容: player={} dialogId={} scene={} 按钮={}",
        copy.playerName,
        dialogId,
        spec.sceneName,
        spec.buttons.size());
    return true;
}

bool NpcDialogueManager::getSpec(int64_t dialogId, hologramlib::NpcDialogSpec& out) const {
    std::lock_guard lock(mMutex);
    return mDialogs.withObject(dialogId, [&](Dialog const& d) {
        out = d.spec;
    });
}

bool NpcDialogueManager::close(int64_t dialogId) {
    Dialog copy;
    {
        std::lock_guard lock(mMutex);
        if (!mDialogs.withObject(dialogId, [&](Dialog const& d) {
                copy = d;
            }))
            return false;
        mDialogs.erase(dialogId);
        if (auto byPlayer = mByPlayer.find(copy.playerName);
            byPlayer != mByPlayer.end() && byPlayer->second == dialogId) {
            mByPlayer.erase(byPlayer);
        }
    }
    if (auto* player = findPlayerByName(copy.playerName)) {
        sendDialog(*player, copy, false);

        std::uint64_t carrierUid = 0;
        {
            std::lock_guard lock(mMutex);
            auto            it = mCarriers.find(copy.playerName);
            if (it != mCarriers.end()) {
                carrierUid = it->second.uniqueId;
                mCarriers.erase(it);
            }
        }
        removeCarrierEntity(*player, carrierUid);
    }
    return true;
}

void NpcDialogueManager::closeAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        ids = mDialogs.snapshotAll();
    }
    // close() 自己加锁，不能持锁遍历
    for (auto const id : ids)
        close(id);
}

bool NpcDialogueManager::isOpen(int64_t dialogId) const {
    std::lock_guard lock(mMutex);
    return mDialogs.exists(dialogId);
}

std::vector<int64_t> NpcDialogueManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mDialogs.snapshotAll();
}

uint64_t NpcDialogueManager::addClickListener(
    std::function<void(hologramlib::NpcDialogClickEvent const&)> listener) {
    if (!listener)
        return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mListeners.emplace(token, std::move(listener));
    return token;
}

bool NpcDialogueManager::removeClickListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mListeners.erase(token) > 0;
}

void NpcDialogueManager::dispatch(hologramlib::NpcDialogClickEvent const& event) {
    std::vector<std::function<void(hologramlib::NpcDialogClickEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);

        if (mClickQueue.size() >= kClickQueueCap) {
            mClickQueue.pop_front();
            mDroppedClicks.fetch_add(1, std::memory_order_relaxed);
        }
        mClickQueue.push_back(event);
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners)
            snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn)
            fn(event);
    }
}

std::vector<hologramlib::NpcDialogClickEvent> NpcDialogueManager::pollClicks() {
    std::vector<hologramlib::NpcDialogClickEvent> out;
    std::lock_guard                               lock(mMutex);
    out.reserve(mClickQueue.size());
    while (!mClickQueue.empty()) {
        out.push_back(std::move(mClickQueue.front()));
        mClickQueue.pop_front();
    }
    return out;
}

void NpcDialogueManager::clearClicks() {
    std::lock_guard lock(mMutex);
    mClickQueue.clear();
}

std::string NpcDialogueManager::formatClick(hologramlib::NpcDialogClickEvent const& event) {
    std::string commands;
    for (std::size_t i = 0; i < event.commands.size(); ++i) {
        if (i)
            commands += ',';
        for (char const c : event.commands[i])
            commands += (c == ',' || c == ' ') ? '+' : c;
    }
    return std::format(
        "player={} dialogId={} scene={} button={} actionId={} closed={} commands={}",
        event.playerName,
        event.dialogId,
        event.sceneName,
        event.buttonIndex,
        event.actionId,
        event.closed ? 1 : 0,
        commands);
}

bool NpcDialogueManager::handleNpcRequest(
    std::string const& playerName,
    std::uint64_t      npcId,
    int                requestType,
    int                actionIndex,
    std::string const& sceneName) {
    hologramlib::NpcDialogClickEvent event;
    bool                             finished = false;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end())
            return false;
        bool const hit = mDialogs.withObject(byPlayer->second, [&](Dialog& dialog) -> bool {
            dialog.touched = false;

            if (npcId != dialog.carrierUniqueId && npcId != dialog.carrierRuntimeId)
                return false;

            event.playerName  = playerName;
            event.dialogId    = dialog.dialogId;
            event.sceneName   = sceneName.empty() ? dialog.spec.sceneName : sceneName;
            event.buttonIndex = -1;
            if (requestType == static_cast<int>(::NpcRequestPacket::RequestType::ExecuteAction)) {
                event.buttonIndex = actionIndex;
                if (actionIndex >= 0 && static_cast<std::size_t>(actionIndex) < dialog.spec.buttons.size()) {
                    auto const& btn = dialog.spec.buttons[actionIndex];
                    event.actionId  = btn.actionId;
                    event.commands  = btn.commands;
                }
                finished = true;
            } else if (requestType == static_cast<int>(::NpcRequestPacket::RequestType::ExecuteClosingCommands)) {
                event.closed = true;
                finished     = true;
            }

            return true;
        });
        if (!hit)
            return false;
    }

    std::uint64_t preDispatchCarrierUid = 0;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mCarriers.find(playerName); it != mCarriers.end())
            preDispatchCarrierUid = it->second.uniqueId;
    }

    if (!event.closed && event.buttonIndex < 0) {
        return false;
    }
    dispatch(event);

    if (finished) {

        std::uint64_t removeCarrierUid = 0;
        {
            std::lock_guard lock(mMutex);
            bool            tookOver = false;
            mDialogs.withObject(event.dialogId, [&](Dialog const& d) {
                tookOver = d.touched;
            });
            auto       byPlayer     = mByPlayer.find(playerName);
            bool const stillCurrent = (byPlayer != mByPlayer.end() && byPlayer->second == event.dialogId);

            if (tookOver) {

            } else if (stillCurrent) {
                if (auto car = mCarriers.find(playerName); car != mCarriers.end()) {
                    removeCarrierUid = car->second.uniqueId;
                    mCarriers.erase(car);
                }
                mDialogs.erase(event.dialogId);
                mByPlayer.erase(byPlayer);
            } else {

                removeCarrierUid = preDispatchCarrierUid;
            }
        }
        if (removeCarrierUid != 0) {
            if (auto* player = findPlayerByName(playerName))
                removeCarrierEntity(*player, removeCarrierUid);
        }
    }
    return true;
}

void NpcDialogueManager::onPlayerLeave(std::string const& playerName) {
    int64_t dialogId = -1;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end())
            dialogId = it->second;
        mCarriers.erase(playerName);
    }
    if (dialogId >= 0)
        close(dialogId);
}

std::uint64_t NpcDialogueManager::droppedClicks() const {
    return mDroppedClicks.load(std::memory_order_relaxed);
}

void NpcDialogueManager::shutdown() {
    closeAll();
    std::lock_guard lock(mMutex);
    mCarriers.clear();
}

LL_TYPE_INSTANCE_HOOK(
    NpcRequestHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const& source,
    NpcRequestPacket const&  packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);

    if (auto* player = findPlayerByNetworkId(source)) {

        std::string const sceneName = packet.mSceneName;
        std::string const actions   = packet.mActions;
        HLIB_LOG_INFO(
            "[NpcDialog] 收到 NpcRequest: player={} rid={} reqType={} actionIndex={} scene='{}' actions='{}'",
            player->getRealName(),
            static_cast<std::uint64_t>(packet.mId.get().rawID),
            static_cast<int>(packet.mType),
            static_cast<int>(packet.mActionIndex),
            sceneName,
            actions);
    }

    // 只认按钮点击和关界面：SetActions/SetName/SetSkin 这类不该产出点击事件
    if (packet.mType != ::NpcRequestPacket::RequestType::ExecuteAction && packet.mType != ::NpcRequestPacket::RequestType::ExecuteClosingCommands) {
        return;
    }

    if (auto* player = findPlayerByNetworkId(source)) {
        NpcDialogueManager::getInstance().handleNpcRequest(
            player->getRealName(),
            static_cast<std::uint64_t>(packet.mId.get().rawID),
            static_cast<int>(packet.mType),
            static_cast<int>(packet.mActionIndex),
            packet.mSceneName);
    }
}

static ll::memory::HookRegistrar<NpcRequestHook> gNpcRequestHookRegistrar;
static debugshape_export::core::HookRegistrar    gHookReg_npcdialog_request{
    "npcdialog-request",
    [] {
        decltype(gNpcRequestHookRegistrar)::hook();
    },
    [] {
        decltype(gNpcRequestHookRegistrar)::unhook();
    },
};

namespace {
class NpcDialogueAdapter final : public hologramlib::INpcDialogue {
public:
    int64_t open(std::string const& playerName, hologramlib::NpcDialogSpec const& spec) override {
        return NpcDialogueManager::getInstance().open(playerName, spec);
    }
    bool               close(int64_t dialogId) override { return NpcDialogueManager::getInstance().close(dialogId); }
    void               closeAll() override { NpcDialogueManager::getInstance().closeAll(); }
    [[nodiscard]] bool isOpen(int64_t dialogId) const override {
        return NpcDialogueManager::getInstance().isOpen(dialogId);
    }
    [[nodiscard]] std::vector<int64_t> getAllIds() const override {
        return NpcDialogueManager::getInstance().getAllIds();
    }
    uint64_t addClickListener(std::function<void(hologramlib::NpcDialogClickEvent const&)> listener) override {
        return NpcDialogueManager::getInstance().addClickListener(std::move(listener));
    }
    bool removeClickListener(uint64_t token) override {
        return NpcDialogueManager::getInstance().removeClickListener(token);
    }
    bool update(int64_t dialogId, hologramlib::NpcDialogSpec const& spec) override {
        return NpcDialogueManager::getInstance().update(dialogId, spec);
    }
};
} // namespace

hologramlib::INpcDialogue& npcDialogueAdapter() {
    static NpcDialogueAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
