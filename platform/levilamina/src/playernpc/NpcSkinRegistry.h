#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <sculk/protocol/codec/actor/player/SerializedSkin.hpp>

#include "hologramlib/HologramLib.h"

class Player;

namespace debugshape_export {

// 皮肤缓存（捕获/序列化）
class NpcSkinRegistry {
public:
    static NpcSkinRegistry& getInstance();

    void init();
    void shutdown();

    bool registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error, bool forceOpaque = false, int alphaMode = 0);

    int importSkinsFromDir(std::string const& dirPath, std::string& error);

    bool captureSkin(std::string const& skinId, std::string const& playerName);

    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    bool registerSkinFromBlob(std::string const& blob, std::string& error);

    bool                     hasSkin(std::string const& skinId) const;
    bool                     unregisterSkin(std::string const& skinId);
    std::vector<std::string> getSkinIds() const;

    bool getSkin(std::string const& skinId, sculk::protocol::SerializedSkin& out) const;

private:
    NpcSkinRegistry()  = default;
    ~NpcSkinRegistry() = default;

    mutable std::mutex                                     mMutex;
    std::map<std::string, sculk::protocol::SerializedSkin> mSkins;
    void*                                                  mGdiplusToken{nullptr};
};

} // namespace debugshape_export
