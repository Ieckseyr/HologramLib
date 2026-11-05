#pragma once

#include "core/EventIds.h"
#include "trade/TradeMenuManager.h"

#include "DiagLog.h"
#include "core/PacketSink.h"

#include "container/ContainerResponse.h"
#include "customentity/CustomEntityManager.h"
#include "trade/TradeOfferNbt.h"
#include <mc/network/packet/InventorySlotPacket.h>

#include <ll/api/chrono/GameChrono.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>

#include <mc/entity/components_json_legacy/EconomyTradeableComponent.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/trading/MerchantRecipe.h>
#include <mc/world/item/trading/MerchantRecipeList.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerEnumName.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ContainerClosePacket.hpp>
#include <sculk/protocol/codec/packet/ItemStackResponsePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateTradePacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <variant>
#include <vector>

namespace debugshape_export {
namespace trade_ops {

inline Player* findPlayerByName(std::string const& name) {
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

template <typename PacketT>
inline bool sendTradePacket(Player& player, PacketT const& packet) {

    return wire::sendBody(
        player,
        static_cast<std::uint32_t>(packet.getId()),
        wire26::body26(packet),
        wire::Reliability::Reliable,
        packet.getName(),
        false);
}

inline int clampTier(int tier) {
    return std::clamp(tier, hologramlib::kTradeTierNovice, hologramlib::kTradeTierMaster);
}
inline int toWireTier(int tier) {
    return clampTier(tier) - 1;
}

constexpr int kOfferNetIdBase = 3676;

inline std::vector<trade::TradeOffer> toOffers(
    std::vector<hologramlib::TradeMenuOffer> const& in,
    int                                             menuWireTier) {
    std::vector<trade::TradeOffer> out;
    out.reserve(in.size());
    for (auto const& o : in) {
        trade::TradeOffer e;
        auto const        item = [](hologramlib::TradeMenuItem const& i) {
            return trade::TradeItem{i.type, i.count, i.damage, i.name, i.lore, i.isBlock, i.blockVersion, {}};
        };
        e.buyA      = item(o.buyA);
        e.buyB      = item(o.buyB);
        e.sell      = item(o.sell);
        e.tier      = o.locked ? menuWireTier + 1 : toWireTier(o.tier);
        e.maxUses   = o.maxUses;
        e.traderExp = o.traderExp;
        out.push_back(std::move(e));
    }
    return out;
}

template <typename T>
inline constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return value.get();
    } else {
        return (value);
    }
}

inline int takeFromSlot(::Player& player, int slot, int count);
inline int giveToInventory(::Player& player, ::ItemStack const& stack, int preferSlot);

inline int takeFromSlot(::Player& player, int slot, int count) {
    auto&     inv  = player.getInventory();
    int const size = inv.getContainerSize();
    if (slot < 0 || slot >= size)
        return -1;
    ::ItemStack cur = inv.getItem(slot);
    if (cur.isNull())
        return -1;
    int const have = static_cast<int>(cur.mCount);
    int const take = std::min(count < 1 ? have : count, have);
    if (take <= 0)
        return -1;
    if (take >= have) {
        inv.setItem(slot, ::ItemStack{});
    } else {
        cur.mCount = static_cast<unsigned char>(have - take);
        inv.setItem(slot, cur);
    }
    return take;
}

inline int giveToInventory(::Player& player, ::ItemStack const& stack, int preferSlot) {
    if (stack.isNull())
        return -1;
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    auto      trySlot = [&](int slot) -> int {
        if (slot < 0 || slot >= size)
            return -1;
        ::ItemStack cur = inv.getItem(slot);
        if (cur.isNull()) {
            inv.setItem(slot, stack);
            return slot;
        }
        if (cur.getTypeName() == stack.getTypeName() && cur.getDamageValue() == stack.getDamageValue() && cur.hasSameUserData(stack)) {
            // 合并上限是物品自己的堆叠上限，不是 127：127 会合出 64 上限物品的 127 个一叠
            int const maxStack = std::max<int>(1, static_cast<int>(stack.getMaxStackSize()));
            int const total    = static_cast<int>(cur.mCount) + static_cast<int>(stack.mCount);
            if (total <= maxStack) {
                cur.mCount = static_cast<unsigned char>(total);
                inv.setItem(slot, cur);
                return slot;
            }
        }
        return -1;
    };
    if (int const slot = trySlot(preferSlot); slot >= 0)
        return slot;
    for (int slot = 0; slot < size; ++slot) {
        ::ItemStack cur = inv.getItem(slot);
        if (!cur.isNull() && cur.getTypeName() == stack.getTypeName()) {
            if (int const s = trySlot(slot); s >= 0)
                return s;
        }
    }
    for (int slot = 0; slot < size; ++slot) {
        if (inv.getItem(slot).isNull()) {
            inv.setItem(slot, stack);
            return slot;
        }
    }
    return -1;
}

inline std::string snbtEscape(std::string const& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char const c : in) {
        if (c == '"' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

inline std::string itemSnbt(hologramlib::TradeMenuItem const& item) {
    if (item.type.empty())
        return {};
    std::string snbt = std::format(
        "{{\"Count\":{}s,\"Damage\":{}s,\"Name\":\"{}\",\"WasPickedUp\":0b",
        item.count,
        item.damage,
        snbtEscape(item.type));
    if (!item.name.empty() || !item.lore.empty()) {
        snbt += ",\"tag\":{\"display\":{";
        bool first = true;
        if (!item.name.empty()) {
            snbt += std::format("\"Name\":\"{}\"", snbtEscape(item.name));
            first = false;
        }
        if (!item.lore.empty()) {
            if (!first)
                snbt += ',';
            snbt += "\"Lore\":[";
            for (std::size_t i = 0; i < item.lore.size(); ++i) {
                if (i)
                    snbt += ',';
                snbt += std::format("\"{}\"", snbtEscape(item.lore[i]));
            }
            snbt += ']';
        }
        snbt += "}}";
    }
    snbt += '}';
    return snbt;
}

inline ::ItemStack makeItemStackOf(hologramlib::TradeMenuItem const& item) {
    auto const snbt = itemSnbt(item);
    if (snbt.empty())
        return ::ItemStack{};
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value())
        return ::ItemStack{};
    return ::ItemStack::fromTag(tag.value());
}

} // namespace trade_ops
} // namespace debugshape_export
