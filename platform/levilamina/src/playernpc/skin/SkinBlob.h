#pragma once

#include "core/proto/Version.h"

#include <string>

namespace debugshape_export::skin {

// 皮肤二进制缓存（HLNS v3）：序列化/反序列化只做字节搬运，不碰注册表。
std::string serializeSkin(std::string const& skinId, core::proto::SerializedSkinRef const& skin);
bool        deserializeSkin(std::string const& blob, std::string& skinId, core::proto::SerializedSkinRef& skin);

} // namespace debugshape_export::skin
