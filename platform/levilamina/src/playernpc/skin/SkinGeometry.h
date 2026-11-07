#pragma once

#include "core/proto/Version.h"

#include <string>

namespace debugshape_export::skin {

//（geometry.json）
std::string parseGeometryIdentifier(std::string const& json);
std::string parseGeometryVersion(std::string const& json);

// 给协议结构补上默认几何与 skin id 派生字段。
void finalizeSkin(core::proto::SerializedSkinRef& skin, std::string const& registryId);

} // namespace debugshape_export::skin
