// NpcSkinRegistry.h - 假玩家 NPC 皮肤注册表（纯内存, 库不落盘; 持久化由消费方经 blob API 自理）
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

    // GDI+ 生命周期（ModEntry enable/disable 调用; 重复调用安全）
    void init();
    void shutdown();

    // PNG 注册（geometryData 非空时启用自定义模型; 重复 skinId 覆盖）
    bool registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error);

    // 目录批量导入: 一个子文件夹 = 一套皮肤（PNG 必需 + .json 几何可选）
    // skinId = 子文件夹名; 返回导入数量, 目录无效返回 -1
    int importSkinsFromDir(std::string const& dirPath, std::string& error);

    // 从在线玩家采集（玩家不在线返回 false）
    bool captureSkin(std::string const& skinId, std::string const& playerName);

    // blob 导出/注册（消费方持久化配对用）
    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    bool registerSkinFromBlob(std::string const& blob, std::string& error);

    bool hasSkin(std::string const& skinId) const;
    bool unregisterSkin(std::string const& skinId); // 皮肤不存在返回 false（其余一律成功）
    std::vector<std::string> getSkinIds() const;

    // NPC 发包用: 拷贝输出皮肤（未注册返回 false）
    bool getSkin(std::string const& skinId, sculk::protocol::SerializedSkin& out) const;

private:
    NpcSkinRegistry()  = default;
    ~NpcSkinRegistry() = default;

    mutable std::mutex                                     mMutex;
    std::map<std::string, sculk::protocol::SerializedSkin> mSkins;
    void*                                                  mGdiplusToken{nullptr}; // ULONG_PTR
};

} // namespace debugshape_export