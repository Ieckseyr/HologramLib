// NpcDialogueManager.cpp - NPC 对话界面（协议层）实现
#include "npcdialog/NpcDialogueManager.h"

#include "DiagLog.h"

#include "customentity/CustomEntityManager.h"
#include "npcdialog/NpcCarrierPacket.h"

#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/NpcRequestPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

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
    if (!level) return nullptr;
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
    if (!level) return nullptr;
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
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

// 按钮 JSON —— 26.40 实测格式（客户端 SetActions 的原样载荷, 由本库 NpcRequest 钩子记录）:
//   [ { "button_name": "...", "data": [ { "cmd_line": "...", "cmd_ver": 50 } ],
//       "mode": 0, "text": "...\n", "type": 1 }, ... ]
// 注意: 顶层是**数组**（不是 {"buttons":[...]}）; cmd_ver 实测恒为 50。
// mode = 客户端动作类型（与参考实现 ActionType 一致）: 0=普通按钮(回传 ExecuteAction) 1=关闭 2=打开。
// 客户端读的是 AddActor 里的 Actions 元数据, 这份 JSON 同时也会放进 NpcDialoguePacket（冗余但一致）。
std::string buildActionJson(std::vector<hologramlib::NpcDialogButton> const& buttons) {
    std::string json{"[\n"};
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        auto const& btn = buttons[i];
        std::string text;
        json += "   {\n";
        json += "      \"button_name\" : \"" + jsonEscape(btn.label) + "\",\n";
        json += "      \"data\" : [\n";
        for (std::size_t k = 0; k < btn.commands.size(); ++k) {
            json += "         {\n            \"cmd_line\" : \"" + jsonEscape(btn.commands[k])
                    + "\",\n            \"cmd_ver\" : 50\n         }";
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

// 与交易菜单同款: 不做 BDS 回读硬校验（合成 JSON 可能被读后校验拒绝, 但不影响客户端收包）
template <typename PacketT>
bool sendPacket(Player& player, PacketT const& packet) {
    std::vector<std::byte>        body;
    sculk::protocol::BinaryStream bodyStream(body);
    packet.write(bodyStream);
    if (body.empty()) return false;

    BinaryStream out;
    out.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    out.mBuffer.append(reinterpret_cast<char const*>(body.data()), body.size());

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem) return false;
    auto* peer = networkSystem->getPeerForUser(player.getNetworkIdentifier());
    if (peer == nullptr) return false;
    peer->sendPacket(out.mBuffer, NetworkPeer::Reliability::Reliable, Compressibility::Compressible);
    return true;
}

// 合成载体 NPC 的生成包（纯协议实体, 不进 BDS 实体系统 —— BDS 侧没有它, 所以 NpcRequest 钩子恒调 origin）。
// 包结构见 NpcCarrierPacket.h（离线对拍: tests/check-npc-carrier.py）。
// 位置 = 玩家身后 5 格 + 世界下方 y=kCarrierY: 客户端看不到实体本身, 但界面里的头像照常渲染。
void sendCarrierSpawn(
    Player&                            player,
    std::uint64_t                      uniqueId,
    std::uint64_t                      runtimeId,
    hologramlib::NpcDialogSpec const&  spec,
    std::string const&                 actionJson
) {
    auto const& pos = player.getPosition();
    float const yaw = player.getRotation().y;
    float const rad = yaw * (std::numbers::pi_v<float> / 180.0f);
    auto const  position = sculk::protocol::Vec3{
        pos.x + std::sin(rad) * 5.0f, // 前向 = (-sin, cos) → 身后 = (sin, -cos)
        npcdialog::kCarrierY,
        pos.z - std::cos(rad) * 5.0f
    };

    sendPacket(
        player,
        npcdialog::buildCarrierAddActor(
            uniqueId,
            runtimeId,
            spec.carrierIdentifier,
            spec.npcName,
            actionJson,
            position,
            yaw
        )
    );
}

} // namespace

namespace {
// 载体 uniqueId 取负段（与真实 NPC 同风格, 且避开真实 id）, runtimeId 用高位段避免冲突
std::atomic<std::int64_t>   gNextCarrierUid{-850403524570LL};
std::atomic<std::uint64_t>  gNextCarrierRid{0x6E200000ULL};
} // namespace

std::uint64_t NpcDialogueManager::nextCarrierUniqueId() {
    return static_cast<std::uint64_t>(gNextCarrierUid.fetch_sub(1));
}
std::uint64_t NpcDialogueManager::nextCarrierRuntimeId() { return gNextCarrierRid.fetch_add(1); }

NpcDialogueManager& NpcDialogueManager::getInstance() {
    static NpcDialogueManager instance;
    return instance;
}

void NpcDialogueManager::removeCarrierEntity(Player& player, std::uint64_t uniqueId) {
    if (uniqueId == 0) return;
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
    sendPacket(player, packet);
}

// 生成（或重建）该玩家的对话载体: 合成 NPC、只发给该玩家、摆在身后 5 格的世界下方。
// 每次打开都重建 —— 按钮 JSON 随 AddActor 的 ActorData 下发, 且旧的客户端实体要先删掉不留残影。
std::uint64_t NpcDialogueManager::ensureCarrier(
    Player&                            player,
    std::string const&                 playerName,
    hologramlib::NpcDialogSpec const&  spec,
    std::string const&                 actionJson
) {
    {
        std::lock_guard lock(mMutex);
        auto            it = mCarriers.find(playerName);
        if (it != mCarriers.end()) {
            removeCarrierEntity(player, it->second.uniqueId);
            mCarriers.erase(it);
        }
    }

    auto const uid = nextCarrierUniqueId();
    auto const rid = nextCarrierRuntimeId();
    sendCarrierSpawn(player, uid, rid, spec, actionJson);

    {
        std::lock_guard lock(mMutex);
        mCarriers[playerName] = Carrier{uid, rid};
    }
    HLIB_LOG_INFO("[NpcDialog] 载体已生成: player={} uid={} rid={} (合成 NPC, y=-66)", playerName, uid, rid);
    return uid;
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
    if (dialog0Uid == 0) return -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mCarriers.find(playerName);
        if (it == mCarriers.end()) return -1;
        carrierRuntime = it->second.runtimeId;
    }
    HLIB_LOG_INFO("[NpcDialog] open 步骤2/5: 载体就绪 uid={} rid={}", dialog0Uid, carrierRuntime);

    std::uint64_t uniqueId  = dialog0Uid;
    std::uint64_t runtimeId = carrierRuntime;
    HLIB_LOG_INFO("[NpcDialog] open 步骤3/5: id pair uid={} rid={}", uniqueId, runtimeId);

    Dialog dialog;
    dialog.playerName       = playerName;
    // 诊断覆盖: 指定了就用它当目标 NPC（不再依赖自建载体被识别为 NPC）
    dialog.carrierUniqueId  = spec.npcUniqueIdOverride != 0
                                  ? static_cast<std::uint64_t>(spec.npcUniqueIdOverride)
                                  : uniqueId;
    dialog.carrierRuntimeId = runtimeId;
    dialog.spec             = spec;

    int64_t dialogId = 0;
    {
        std::lock_guard lock(mMutex);
        // 同一玩家只保留一个对话记录: 重开时把上一条清掉, 否则 mDialogs 里会留陈旧条目
        // （旧 id 还会被 isOpen/getAllIds 报出来, 客户端回传旧 id 时匹配语义也不清晰）
        if (auto prev = mByPlayer.find(playerName); prev != mByPlayer.end()) {
            mDialogs.erase(prev->second);
        }
        dialogId        = mNextDialogId++;
        dialog.dialogId = dialogId;
        mDialogs[dialogId]    = dialog;
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
        spec.npcName
    );
    return dialogId;
}

bool NpcDialogueManager::update(int64_t dialogId, hologramlib::NpcDialogSpec const& spec) {
    Dialog copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mDialogs.find(dialogId);
        if (it == mDialogs.end()) return false;
        it->second.spec    = spec;
        it->second.touched = true; // 本次点击已由调用方接管: 库里不再自动拆这个对话
        copy               = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr) return false;

    sendDialog(*player, copy, true); // 同一载体, 只重发对话包
    HLIB_LOG_INFO(
        "[NpcDialog] 就地换内容: player={} dialogId={} scene={} 按钮={}",
        copy.playerName,
        dialogId,
        spec.sceneName,
        spec.buttons.size()
    );
    return true;
}

bool NpcDialogueManager::close(int64_t dialogId) {
    Dialog copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mDialogs.find(dialogId);
        if (it == mDialogs.end()) return false;
        copy = it->second;
        mDialogs.erase(it);
        if (auto byPlayer = mByPlayer.find(copy.playerName);
            byPlayer != mByPlayer.end() && byPlayer->second == dialogId) {
            mByPlayer.erase(byPlayer);
        }
    }
    if (auto* player = findPlayerByName(copy.playerName)) {
        sendDialog(*player, copy, false); // ActionType::Close
        // 关闭即删除客户端上的载体实体（不留残影）; 下次打开会重新生成
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
        for (auto const& [id, dialog] : mDialogs) ids.push_back(id);
    }
    for (auto const id : ids) close(id);
}

bool NpcDialogueManager::isOpen(int64_t dialogId) const {
    std::lock_guard lock(mMutex);
    return mDialogs.contains(dialogId);
}

std::vector<int64_t> NpcDialogueManager::getAllIds() const {
    std::lock_guard      lock(mMutex);
    std::vector<int64_t> out;
    out.reserve(mDialogs.size());
    for (auto const& [id, dialog] : mDialogs) out.push_back(id);
    return out;
}

uint64_t NpcDialogueManager::addClickListener(
    std::function<void(hologramlib::NpcDialogClickEvent const&)> listener
) {
    if (!listener) return 0;
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
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(event);
    }
}

bool NpcDialogueManager::handleNpcRequest(
    std::string const& playerName,
    std::uint64_t      npcId,
    int                requestType,
    int                actionIndex,
    std::string const& sceneName
) {
    hologramlib::NpcDialogClickEvent event;
    bool                             finished = false;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto dialogIt = mDialogs.find(byPlayer->second);
        if (dialogIt == mDialogs.end()) return false;
        auto& dialog = dialogIt->second;
        dialog.touched = false; // 本次回传开始; 调用方在回调里 update 会重新置位
        // 回传的 mId 是 ActorUniqueID —— 两个 id 都比一遍（客户端回传的不一定用哪一个）
        if (npcId != dialog.carrierUniqueId && npcId != dialog.carrierRuntimeId) return false;

        event.playerName  = playerName;
        event.dialogId    = dialog.dialogId;
        event.sceneName   = sceneName.empty() ? dialog.spec.sceneName : sceneName;
        event.buttonIndex = -1;
        if (requestType == static_cast<int>(::NpcRequestPacket::RequestType::ExecuteAction)) {
            event.buttonIndex = actionIndex;
            if (actionIndex >= 0 && static_cast<std::size_t>(actionIndex) < dialog.spec.buttons.size()) {
                auto const& btn = dialog.spec.buttons[actionIndex];
                event.actionId  = btn.actionId; // 回填调用方的标识
                event.commands  = btn.commands; // 合成 NPC 不由 BDS 代跑命令, 交给调用方处理
            }
            finished = true;
        } else if (requestType
                   == static_cast<int>(::NpcRequestPacket::RequestType::ExecuteClosingCommands)) {
            event.closed = true;
            finished     = true;
        }
        // 其余回传类型（SetActions/SetName/SetSkin/SetInteractText…）是客户端编辑器回写,
        // 对合成 NPC 无意义, 不结束对话。
    }

    // dispatch 之前先记下当前载体: 回调里若换成另一个对话（open）, 旧载体实体要由这里收尾
    std::uint64_t preDispatchCarrierUid = 0;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mCarriers.find(playerName); it != mCarriers.end()) preDispatchCarrierUid = it->second.uniqueId;
    }

    dispatch(event);

    if (finished) {
        // 点按钮/关闭后客户端已自行收起界面, 正常情况下这里收尾（删载体 + 结束记录）。
        // 但**回调里可能已经接管了这次点击**:
        //   · 调了 update()（就地换页/进子菜单）→ 界面要继续留着, 不能拆
        //   · 调了 open()（换成另一个对话）→ 当前对话已换成新的, 不能按玩家无脑拆
        // 所以只处理"本次事件对应的那个对话", 且它没有被接管过。
        std::uint64_t removeCarrierUid = 0;
        {
            std::lock_guard lock(mMutex);
            auto            it           = mDialogs.find(event.dialogId);
            bool const      tookOver     = (it != mDialogs.end() && it->second.touched);
            auto            byPlayer     = mByPlayer.find(playerName);
            bool const      stillCurrent = (byPlayer != mByPlayer.end() && byPlayer->second == event.dialogId);

            if (tookOver) {
                // 调用方接管（就地换内容）: 保持对话与载体
            } else if (stillCurrent) {
                if (auto car = mCarriers.find(playerName); car != mCarriers.end()) {
                    removeCarrierUid = car->second.uniqueId;
                    mCarriers.erase(car);
                }
                if (it != mDialogs.end()) mDialogs.erase(it);
                mByPlayer.erase(byPlayer);
            } else {
                // 回调里开了新对话: 只清掉旧载体实体, 新对话保持不动
                removeCarrierUid = preDispatchCarrierUid;
            }
        }
        if (removeCarrierUid != 0) {
            if (auto* player = findPlayerByName(playerName)) removeCarrierEntity(*player, removeCarrierUid);
        }
    }
    return true;
}

void NpcDialogueManager::onPlayerLeave(std::string const& playerName) {
    int64_t dialogId = -1;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end()) dialogId = it->second;
        mCarriers.erase(playerName); // 玩家已离线, 客户端实体随之消失, 只需清记录
    }
    if (dialogId >= 0) close(dialogId);
}

void NpcDialogueManager::shutdown() {
    closeAll();
    std::lock_guard lock(mMutex);
    mCarriers.clear(); // 载体只在客户端存在, 库卸载时无需再发包
}

// ── NpcRequestPacket 钩子: 点按钮 / 关闭对话的回传 ──
// 我们的载体是纯协议实体, BDS 侧找不到它 → 恒调 origin 不改变原版行为
LL_TYPE_INSTANCE_HOOK(
    NpcRequestHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const& source,
    NpcRequestPacket const&  packet
) {
    origin(source, packet);

    // NpcRequestPacket 继承自 ll::PayloadPacket<NpcRequestPacketPayload> —— 字段名用 payload 的
    // （mId/mType/mActionIndex/mSceneName）, 且都在 TypedStorage 里, 需 .get()
    // 全量诊断: 记录每一个 NpcRequest 的原始内容。
    // 关键点: 对话/按钮 JSON 是客户端通过 SetActions(0) 发给服务端的（schema:
    // NpcRequestPacketPayload.Actions 是 string）—— 所以在游戏里编辑一次对话,
    // 那个 JSON 就会出现在这里, 这就是按钮格式的真值来源, 不用抓包。
    if (auto* player = findPlayerByNetworkId(source)) {
        // TypedStorage 字段不能直接进 fmt, 先落成局部值
        std::string const sceneName = packet.mSceneName;
        std::string const actions   = packet.mActions;
        HLIB_LOG_INFO(
            "[NpcDialog] 收到 NpcRequest: player={} rid={} reqType={} actionIndex={} scene='{}' actions='{}'",
            player->getRealName(),
            static_cast<std::uint64_t>(packet.mId.get().rawID),
            static_cast<int>(packet.mType),
            static_cast<int>(packet.mActionIndex),
            sceneName,
            actions
        );
    }

    if (auto* player = findPlayerByNetworkId(source)) {
        NpcDialogueManager::getInstance().handleNpcRequest(
            player->getRealName(),
            static_cast<std::uint64_t>(packet.mId.get().rawID),
            static_cast<int>(packet.mType),
            static_cast<int>(packet.mActionIndex),
            packet.mSceneName
        );
    }
}

static ll::memory::HookRegistrar<NpcRequestHook> gNpcRequestHookRegistrar;

namespace {
class NpcDialogueAdapter final : public hologramlib::INpcDialogue {
public:
    int64_t open(std::string const& playerName, hologramlib::NpcDialogSpec const& spec) override {
        return NpcDialogueManager::getInstance().open(playerName, spec);
    }
    bool close(int64_t dialogId) override { return NpcDialogueManager::getInstance().close(dialogId); }
    void closeAll() override { NpcDialogueManager::getInstance().closeAll(); }
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
