// NpcSkinRegistry.h - 假玩家 NPC 皮肤注册表（1.16.0; 1.18.0 目录导入 + blob API）
//
// 纯内存注册表（库不落盘）, 皮肤以 skinId 为键全局注册, NPC 引用 skinId:
//   - PNG 文件注册: GDI+ 解码 64x64/128x128 → RGBA SerializedSkin
//     （geometry/armSize 可自定义; geometryData 提供时启用自定义模型）
//   - 目录导入: 一个子文件夹 = 一套皮肤（PNG 必需 + .json 几何可选 = 默认玩家模型）
//   - 在线玩家采集: Player::getSerializedSkin 全字段拷贝 → 快照注册, 之后换肤不影响
//   - blob 导出/注册: 消费方负责持久化（存自己的目录, 重启时 registerSkinFromBlob 恢复）
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
    // skin.geometryData 非空时启用自定义模型（identifier 自动提取）
    bool registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error);

    // 目录批量导入: 一个子文件夹 = 一套皮肤
    //   子文件夹内: PNG 贴图（必需, 64/128）+ .json 几何模型（可选, 缺省 = 标准玩家模型）
    //   skinId = 子文件夹名; 逐文件夹独立注册（单个失败仅跳过并 warn）
    // 返回成功导入数量; 目录无效返回 -1（error 给出原因）
    int importSkinsFromDir(std::string const& dirPath, std::string& error);

    // 从在线玩家采集（Player::getSerializedSkin 全字段拷贝; 玩家不在线返回 false）
    bool captureSkin(std::string const& skinId, std::string const& playerName);

    // 皮肤全字段序列化导出（消费方持久化用; 未注册返回 false）
    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    // blob 反序列化注册（与 getSkinBlob 配对; 格式非法返回 false, error 给出原因）
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