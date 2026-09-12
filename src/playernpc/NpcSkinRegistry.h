// NpcSkinRegistry.h - 假玩家皮肤注册表（纯内存；持久化由消费方用 blob API 自理）
#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <sculk/protocol/codec/actor/player/SerializedSkin.hpp>

#include "hologramlib/HologramLib.h" // hologramlib::PlayerNpcSkin

class Player; // ::Player

namespace debugshape_export {

class NpcSkinRegistry {
public:
    static NpcSkinRegistry& getInstance();

    // GDI+ 生命周期（ModEntry enable/disable 调用，可重复调用）
    void init();
    void shutdown();

    // 用 PNG 注册皮肤；geometryData 非空则启用自定义模型。重复 skinId 覆盖。
    bool registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error);

    // 批量导入目录：一个子文件夹 = 一套皮肤（PNG 必需，.json 几何可选）。
    // skinId 取子文件夹名。返回导入数量，目录无效返回 -1。
    int importSkinsFromDir(std::string const& dirPath, std::string& error);

    // 采集在线玩家的皮肤（玩家不在线返回 false）
    bool captureSkin(std::string const& skinId, std::string const& playerName);

    // blob 导出 / 注册（与消费方的持久化配对使用）
    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    bool registerSkinFromBlob(std::string const& blob, std::string& error);

    bool hasSkin(std::string const& skinId) const;
    bool unregisterSkin(std::string const& skinId); // 皮肤不存在返回 false，其余一律成功
    std::vector<std::string> getSkinIds() const;

    // 发包用：拷贝一份皮肤（未注册返回 false）
    bool getSkin(std::string const& skinId, sculk::protocol::SerializedSkin& out) const;

private:
    NpcSkinRegistry()  = default;
    ~NpcSkinRegistry() = default;

    mutable std::mutex                                     mMutex;
    std::map<std::string, sculk::protocol::SerializedSkin> mSkins;
    void*                                                  mGdiplusToken{nullptr}; // ULONG_PTR
};

} // namespace debugshape_export
