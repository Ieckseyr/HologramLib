// NpcSkinRegistry.h - 假玩家 NPC 皮肤注册表（1.16.0）
//
// 皮肤以 skinId 为键全局注册, NPC 引用 skinId:
//   - PNG 文件注册: GDI+ 解码 64x64/128x128 → RGBA SerializedSkin
//     （geometry/armSize 可自定义; 解码一次常驻内存, 多 NPC 复用零重复开销）
//   - 在线玩家采集: 复制 Player::getSerializedSkin() 全部字段
//     （含几何/披风/动画）→ 永久注册, 玩家之后换肤不影响已采集副本
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

    // PNG 文件注册（失败返回 false, error 给出原因; 重复 skinId 覆盖）
    bool registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error);

    // 从在线玩家采集（Player::getSerializedSkin 全字段拷贝; 玩家不在线返回 false）
    bool captureSkin(std::string const& skinId, std::string const& playerName);

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
