// PacketTrace.h - 发包审计（测试用）
//
// 只记录"真的交给了 NetworkPeer"的包：名称 / 包 id / 收件人 / 字节数 / 时间 / 序号。
// 由测试命令显式 start，平时不记录、不占内存。
//
// 断线保护：trace 进行中如果收件人退出，自动把"退出前发过的包"写成
// plugins/HologramLib/wire-trace-<player>-<epoch>.json（含按包名统计 + 最后 10 条），
// 同时往日志里打一份摘要。
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

class Player;

namespace debugshape_export::wire {

struct TraceEntry {
    std::uint64_t seq{};
    std::uint64_t timeMs{};
    std::uint32_t id{};
    std::size_t   bytes{};
    std::string   name;
    std::string   player;
};

// 注册断线监听（ModEntry::enable 调一次，幂等）
void traceInit();

void        traceStart(std::string tag);
void        traceStop();
void        traceClear();
bool        traceActive();
std::size_t traceCount();
std::string traceDumpJson();

// 发送成功后调用（由 PacketSink 入口统一调用）
void noteSent(std::string_view name, std::uint32_t id, ::Player& player, std::size_t bytes);

// 打一个分组标记（测试用：把"跑到哪一组"也记进发包账本，断线时能看出最后在跑什么）
void traceMark(std::string_view text);

} // namespace debugshape_export::wire
