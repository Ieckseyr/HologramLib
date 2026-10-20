// ServiceHost.h - 域的注册与生命周期
//
// 收敛前的形状: ModEntry::enable() / disable() / exportLseFunctions() 三处各有一张**手工清单**，
// 新增一个域要记得改三遍。漏改过一次: ghost 域的两个 LSE 导出曾长期不在注册流程里
// （脚本 ll.import 拿到 null），代码里留着一行注释记着这件事。
//
// 收敛后: 新增域 = 在 ModEntry 里写一个 IDomain 适配器 + 一次 add；
// 启动、关停、LSE 导出由本类统一遍历，顺序规则只有一条:
//   · start 按声明顺序（依赖先行）
//   · stop 按**反向**声明顺序（后起的先关）
//   · exportLse 按声明顺序
//
// 同时它补上了一个既有缺陷: ContainerMenu / NpcDialogue / PlayerSensing / FakeInventory
// 四个域早就写好了 shutdown()，但 ModEntry::disable() 从来没调用过 —— 关插件时打开的
// 虚拟容器不关、NPC 对话框不关、伪造背包不还原，客户端要重连才恢复。接进 IDomain::stop
// 之后这类"漏在清单外"的清理不会再发生。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "DiagLog.h"

namespace debugshape_export {
namespace core {

struct IDomain {
    virtual ~IDomain() = default;

    virtual char const* name() const noexcept = 0;

    /// 返回 false = 启用失败（ServiceHost 会回滚已启动的域，整个库不启用）
    virtual bool start() { return true; }

    /// 必须 noexcept: 关停路径上没有异常可以逃逸（本库编译期关闭异常）
    virtual void stop() noexcept {}

    /// LSE 导出（lrca 可用时由 ModEntry 统一调用一次）
    virtual void exportLse() {}
};

class ServiceHost {
public:
    ServiceHost()                                 = default;
    ServiceHost(ServiceHost const&)               = delete;
    ServiceHost& operator=(ServiceHost const&)    = delete;

    template <class D, class... Args>
    D& add(Args&&... args) {
        auto owned = std::make_unique<D>(std::forward<Args>(args)...);
        D&   ref   = *owned;
        mDomains.push_back(std::move(owned));
        return ref;
    }

    bool startAll() {
        for (std::size_t i = 0; i < mDomains.size(); ++i) {
            auto& domain = *mDomains[i];
            if (!domain.start()) {
                HLIB_LOG_ERROR("[host] domain '{}' failed to start, rolling back", domain.name());
                stopFrom(i); // 只回滚已经起来的那些
                return false;
            }
            HLIB_LOG_INFO("[host] started '{}'", domain.name());
        }
        return true;
    }

    /// 反向关停并清空注册表（可重复调用；下次 enable 会重新注册）
    void stopAll() noexcept {
        stopFrom(mDomains.size());
        mDomains.clear();
    }

    void exportAll() {
        for (auto& domain : mDomains) {
            domain->exportLse();
        }
    }

    std::size_t size() const noexcept { return mDomains.size(); }

    std::string describe() const {
        std::string out;
        for (auto& domain : mDomains) {
            if (!out.empty()) out += ", ";
            out += domain->name();
        }
        return out;
    }

private:
    void stopFrom(std::size_t exclusiveEnd) noexcept {
        while (exclusiveEnd > 0) {
            --exclusiveEnd;
            auto& domain = *mDomains[exclusiveEnd];
            domain.stop();
            HLIB_LOG_INFO("[host] stopped '{}'", domain.name());
        }
    }

    // unique_ptr: 域之间不允许互相持有引用生命周期; 顺序由本类保证
    std::vector<std::unique_ptr<IDomain>> mDomains;
};

} // namespace core
} // namespace debugshape_export
