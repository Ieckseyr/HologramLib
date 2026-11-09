#pragma once

#include <mc/world/level/block/Block.h>

#include <cstdint>
#include <string>

namespace debugshape_export::blockstate {

// "minecraft:oak_stairs[waterlogged=true]" -> (name, states)
bool parseBlockState(std::string const& text, std::string& outName, ::Block::BlockStatesType& outStates);

// 状态串 -> 网络 id（26.60 是哈希值）
bool resolveNetworkId(std::string const& stateText, std::uint32_t& outId);

// SNBT（getNbt().toSNBT() 的形态）→ network id；同时把解析出的 NBT 交给调用方
bool resolveNetworkIdFromSnbt(std::string const& snbt, std::uint32_t& outId);

} // namespace debugshape_export::blockstate
