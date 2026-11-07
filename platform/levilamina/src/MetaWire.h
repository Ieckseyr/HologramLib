#pragma once

// DataItem 构造：按实参类型选 Payload（顺序同线上 tag：0 byte … 8 vec3）。

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "core/proto/Version.h"

namespace debugshape_export::meta_wire {

namespace proto = ::debugshape_export::core::proto;

using Entry = proto::DataItemEntry;

namespace detail {

inline std::uint32_t idOf(proto::ActorDataIDs id) noexcept {
    return static_cast<std::uint32_t>(id);
}

template <class PayloadT, class ValueT>
inline Entry make(proto::ActorDataIDs id, ValueT&& value) {
    return Entry{idOf(id), PayloadT{std::forward<ValueT>(value)}};
}

} // namespace detail

inline Entry byte(proto::ActorDataIDs id, std::uint8_t v) {
    return detail::make<proto::bp::DataItemBytePayload>(id, static_cast<std::int8_t>(v));
}

inline Entry byte(proto::ActorDataIDs id, std::int8_t v) {
    return detail::make<proto::bp::DataItemBytePayload>(id, v);
}

inline Entry int16(proto::ActorDataIDs id, std::int16_t v) {
    return detail::make<proto::bp::DataItemShortPayload>(id, v);
}

inline Entry int32(proto::ActorDataIDs id, std::int32_t v) {
    return detail::make<proto::bp::DataItemIntPayload>(id, v);
}

inline Entry int64(proto::ActorDataIDs id, std::int64_t v) {
    return detail::make<proto::bp::DataItemInt64Payload>(id, v);
}

inline Entry float32(proto::ActorDataIDs id, float v) {
    return detail::make<proto::bp::DataItemFloatPayload>(id, v);
}

inline Entry string(proto::ActorDataIDs id, std::string v) {
    return detail::make<proto::bp::DataItemStringPayload>(id, std::move(v));
}

inline Entry string(proto::ActorDataIDs id, std::string_view v) {
    return string(id, std::string{v});
}

inline Entry vec3(proto::ActorDataIDs id, proto::Vec3 v) {
    return detail::make<proto::bp::DataItemVec3Payload>(id, v);
}

} // namespace debugshape_export::meta_wire
