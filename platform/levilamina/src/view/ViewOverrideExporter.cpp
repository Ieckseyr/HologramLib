#include "ViewOverrideExporter.h"

#include "ViewOverrideManager.h"

#include "container/ContainerPackets.h"
#include "core/ItemNbt.h"
#include "playernpc/NpcProtocol.h"
#include "view/ViewPackets.h"

#include <mc/deps/nbt/CompoundTag.h>

#include <cctype>
#include <vector>
#include "ViewPackets.h"

#include "lse/LseBridge.h"

#include <ll/api/service/Bedrock.h>
#include <exception>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <string>


namespace {

// 调用方自己把 SNBT 转成 NBT 字节再传进来；这里只做传输解码（hex 或 base64），不解析 SNBT。
std::optional<std::vector<std::uint8_t>> decodeNbtPayload(std::string const& text) {
    auto const isHex = [&] {
        if (text.size() < 2 || text.size() % 2 != 0)
            return false;
        for (char c : text) {
            if (!std::isxdigit(static_cast<unsigned char>(c)))
                return false;
        }
        return true;
    }();

    std::vector<std::uint8_t> out;
    if (isHex) {
        auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return c - 'A' + 10;
        };
        out.reserve(text.size() / 2);
        for (std::size_t i = 0; i + 1 < text.size(); i += 2)
            out.push_back(static_cast<std::uint8_t>((nibble(text[i]) << 4) | nibble(text[i + 1])));
        return out;
    }

    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int    bits = 0;
    int    acc  = 0;
    for (char c : text) {
        if (c == '=' || std::isspace(static_cast<unsigned char>(c)))
            continue;
        auto const pos = std::string_view{kAlphabet}.find(c);
        if (pos == std::string_view::npos)
            return std::nullopt;
        acc  = (acc << 6) | static_cast<int>(pos);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((acc >> bits) & 0xFF));
        }
    }
    return out;
}

} // namespace

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string trim(std::string const& text) {
    auto const begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    auto const end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool truthy(std::string const& value) {
    auto const v = lower(trim(value));
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

bool intOf(std::string const& value, int& out) {
    try {
        std::size_t used = 0;
        auto const  v    = std::stoi(trim(value), &used);
        if (used == 0)
            return false;
        out = v;
        return true;
    } catch (std::exception const&) {
        return false;
    }
}

bool floatOf(std::string const& value, float& out) {
    try {
        std::size_t used = 0;
        auto const  v    = std::stof(trim(value), &used);
        if (used == 0)
            return false;
        out = v;
        return true;
    } catch (std::exception const&) {
        return false;
    }
}

hologramlib::EntityView parseEntitySpec(std::string const& spec) {
    hologramlib::EntityView view;
    std::string             part;
    auto                    flush = [&] {
        auto const eq = part.find('=');
        if (eq != std::string::npos) {
            auto const key   = lower(trim(part.substr(0, eq)));
            auto const value = trim(part.substr(eq + 1));
            if (key == "type") {
                view.identifier = value;
            } else if (key == "as") {
                view.asPlayer = value;
            } else if (key == "hidden") {
                view.hidden = truthy(value);
            } else if (key == "name") {
                view.hasNametag = true;
                view.nametag    = value;
            } else if (key == "always") {
                view.nametagAlwaysShow = truthy(value);
            } else if (key == "skin") {

                view.skinId = value;
            }

            else if (key == "pose") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasPose = true;
                    view.pose    = v;
                }
            } else if (key == "scale") {

                float v = 1.0f;
                if (floatOf(value, v)) {
                    view.hasMetaScale = true;
                    view.metaScale    = v;
                }
            } else if (key == "variant") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasVariant = true;
                    view.variant    = v;
                }
            } else if (key == "mark" || key == "markvariant") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasMarkVariant = true;
                    view.markVariant    = v;
                }
            } else if (key == "color" || key == "colorindex") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasColorIndex = true;
                    view.colorIndex    = v;
                }
            } else if (key == "color2") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasColor2 = true;
                    view.color2    = v;
                }
            }
        }
        part.clear();
    };
    for (char const c : spec) {
        if (c == ';' || c == '\n') {
            flush();
        } else {
            part += c;
        }
    }
    flush();
    return view;
}

static bool playerIdOf(std::string const& playerName, std::int64_t& uniqueId) {
    return view::ViewOverrideManager::getInstance().uniqueIdOfPlayer(playerName, uniqueId);
}

void ViewOverrideExporter::exportAll() {
    auto& manager = view::ViewOverrideManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "viewEntity",
                               [&manager](std::string const& playerName, int64_t uniqueId, std::string const& spec) -> bool {
                                   return manager.overrideEntity(playerName, uniqueId, parseEntitySpec(spec));
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewBlock",
                               [&manager](
                                   std::string const& playerName,
                                   int                x,
                                   int                y,
                                   int                z,
                                   std::string const& type) -> bool {
                                   hologramlib::BlockView view;
                                   view.type = type;
                                   return manager.overrideBlock(playerName, x, y, z, view);
                               });

    // A：调用方自己算 network id（状态由调用方负责）
    hologramlib::lse::exportAs(NAMESPACE, "viewBlockById",
                               [&manager](
                                   std::string const& playerName,
                                   int                x,
                                   int                y,
                                   int                z,
                                   int64_t            networkId) -> bool {
                                   if (networkId <= 0)
                                       return false;
                                   hologramlib::BlockView view;
                                   view.type = "#" + std::to_string(networkId);
                                   return manager.overrideBlock(playerName, x, y, z, view);
                               });

    // C：方块实体预览。nbT 由调用方用 SNBT 转好（hex 或 base64），这里只解码 + 补 id/坐标。
    hologramlib::lse::exportAs(NAMESPACE, "viewBlockEntity",
                               [](std::string const& playerName,
                                  int                x,
                                  int                y,
                                  int                z,
                                  std::string const& type,
                                  std::string const& nbt) -> bool {
                                   auto bytes = decodeNbtPayload(nbt);
                                   if (!bytes || bytes->empty())
                                       return false;

                                   std::string_view raw{reinterpret_cast<char const*>(bytes->data()), bytes->size()};
                                   core::proto::bp::BinaryReader reader{raw};
                                   auto named = core::proto::bp::nbt::readNamedTag(reader);
                                   if (!named)
                                       return false;
                                   auto* compound = named->second.template get_if<core::proto::bp::CompoundTag>();
                                   if (compound == nullptr)
                                       return false;

                                   core::proto::bp::CompoundTag tag = std::move(*compound);
                                   tag["id"] = trade::makeTag(core::proto::bp::StringTag{type});
                                   tag["x"]  = trade::makeTag(core::proto::bp::IntTag{x});
                                   tag["y"]  = trade::makeTag(core::proto::bp::IntTag{y});
                                   tag["z"]  = trade::makeTag(core::proto::bp::IntTag{z});

                                   auto packet = container::makeBlockActorData(x, y, z, std::move(tag));
                                   auto level  = ::ll::service::getLevel();
                                   if (!level)
                                       return false;

                                   bool any = false;
                                   level->forEachPlayer([&](::Player& player) -> bool {
                                       if (!playerName.empty() && player.getRealName() != playerName)
                                           return true;
                                       any |= npc_protocol::sendToPlayer(player, packet, wire::Reliability::Reliable);
                                       return true;
                                   });
                                   return any;
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearEntity",
                               [&manager](std::string const& playerName, int64_t uniqueId) -> bool {
                                   return manager.clearEntity(playerName, uniqueId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearBlock",
                               [&manager](std::string const& playerName, int x, int y, int z) -> bool {
                                   return manager.clearBlock(playerName, x, y, z);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearAll",
                               [&manager](std::string const& playerName) -> void {
                                   manager.clearAll(playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewUniqueIdOf",
                               [](std::string const& playerName) -> int64_t {
                                   auto level = ::ll::service::getLevel();
                                   if (!level)
                                       return -1;
                                   auto* player = level->getPlayer(playerName);
                                   if (player == nullptr)
                                       return -1;
                                   return static_cast<std::int64_t>(player->getOrCreateUniqueID().rawID);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewSelf",
                               [&manager](std::string const& playerName, std::string const& spec) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   return manager.overrideEntity({}, id, parseEntitySpec(spec));
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfClear",
                               [](std::string const& playerName) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   return view::ViewOverrideManager::getInstance().clearEntity({}, id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewSelfType",
                               [&manager](std::string const& playerName, std::string const& type) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   hologramlib::EntityView view;
                                   view.identifier = type;
                                   return manager.overrideEntity({}, id, view);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfSkin",
                               [&manager](std::string const& playerName, std::string const& skinOfPlayer) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   hologramlib::EntityView view;
                                   view.asPlayer = skinOfPlayer;
                                   return manager.overrideEntity({}, id, view);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfName",
                               [&manager](std::string const& playerName, std::string const& nametag, bool alwaysShow) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   hologramlib::EntityView view;
                                   view.hasNametag        = true;
                                   view.nametag           = nametag;
                                   view.nametagAlwaysShow = alwaysShow;
                                   return manager.overrideEntity({}, id, view);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfHide",
                               [&manager](std::string const& playerName, bool hidden) -> bool {
                                   std::int64_t id = 0;
                                   if (!playerIdOf(playerName, id))
                                       return false;
                                   hologramlib::EntityView view;
                                   view.hidden = hidden;
                                   return manager.overrideEntity({}, id, view);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewTargetType",
                               [&manager](std::int64_t uniqueId, std::string const& type) -> bool {
                                   hologramlib::EntityView view;
                                   view.identifier = type;
                                   return manager.overrideEntity({}, uniqueId, view);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "viewClearId",
                               [](std::int64_t uniqueId) -> bool {
                                   return view::ViewOverrideManager::getInstance().clearEntity({}, uniqueId);
                               });

    // 1.36.0：读真实实体信息
    hologramlib::lse::exportAs(NAMESPACE, "viewEntityInfo", [](int64_t uniqueId) -> std::string {
        auto const info = view::ViewOverrideManager::getInstance().queryEntity(uniqueId);
        if (info.identifier.empty())
            return std::string{};
        return std::format(
            "identifier={} nameTag='{}' hasNameTag={} isPlayer={} health={:.1f}",
            info.identifier,
            info.nameTag,
            info.hasNameTag,
            info.isPlayer,
            info.health
        );
    });

    // 1.36.0：把覆盖参数对齐到真实实体（类型 + 名字）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "viewAlign",
        [](std::string const& playerName, int64_t uniqueId) -> bool {
            return view::ViewOverrideManager::getInstance().alignEntity(playerName, uniqueId);
        }
    );

    hologramlib::lse::exportAs(NAMESPACE, "viewMetaProbe",
                               [](std::string const& spec) -> std::string {
                                   return view::describeMetaItems(parseEntitySpec(spec));
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewEcho",
                               [](std::string const& text) -> std::string {
                                   std::string hex;
                                   for (std::size_t i = 0; i < text.size() && i < 16; ++i) {
                                       hex += std::format("{:02x}", static_cast<unsigned char>(text[i]));
                                   }
                                   return std::format("len={} hex={} text=[{}]", text.size(), hex, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewDescribe",
                               [&manager](std::string const& playerName) -> std::string {
                                   return manager.describeFor(playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewPollSpawnEvents",
                               []() -> std::vector<std::string> {
                                   return debugshape_export::view::drainSpawnEvents();
                               });

    hologramlib::lse::exportAs(NAMESPACE, "viewItemSpawnHooked",
                               []() -> bool {
                                   return true;
                               });
}

} // namespace debugshape_export
