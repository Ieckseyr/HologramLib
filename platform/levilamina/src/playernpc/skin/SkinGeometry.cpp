#include "SkinGeometry.h"

#include "../NpcPlayerList.h"

#include <nlohmann/json.hpp>

#include <format>
#include <unordered_map>

namespace debugshape_export::skin {

namespace {

constexpr char kStandardHumanoidGeometry[] =
    R"({"format_version":"1.12.0","minecraft:geometry":[{"description":{"identifier":"geometry.humanoid.custom","texture_width":64,"texture_height":64,"visible_bounds_width":2,"visible_bounds_height":4,"visible_bounds_offset":[0,1.5,0]},"bones":[{"name":"body","pivot":[0,24,0],"cubes":[{"origin":[-4,12,-2],"size":[8,12,4],"uv":[16,16]}]},{"name":"waist","pivot":[0,12,0]},{"name":"head","pivot":[0,24,0],"cubes":[{"origin":[-4,24,-4],"size":[8,8,8],"uv":[0,0]}]},{"name":"rightArm","pivot":[-5,22,0],"cubes":[{"origin":[-8,12,-2],"size":[4,12,4],"uv":[40,16]}]},{"name":"leftArm","pivot":[5,22,0],"cubes":[{"origin":[4,12,-2],"size":[4,12,4],"uv":[32,48]}]},{"name":"rightLeg","pivot":[-1.9,12,0],"cubes":[{"origin":[-3.9,0,-2],"size":[4,12,4],"uv":[0,16]}]},{"name":"leftLeg","pivot":[1.9,12,0],"cubes":[{"origin":[-0.1,0,-2],"size":[4,12,4],"uv":[16,48]}]}]}]})";

} // namespace

void finalizeSkin(core::proto::SerializedSkinRef& skin, std::string const& registryId) {
    npc_protocol::finalizeSkinIds(skin, registryId);
    if (skin.geometry_data.empty()) {
        skin.geometry_data                  = kStandardHumanoidGeometry;
        skin.geometry_data_min_engine_version = "1.12.0";
        skin.resource_patch                 = R"({"geometry":{"default":"geometry.humanoid.custom"}})";
    }
}

std::string parseGeometryIdentifier(std::string const& text) {
    try {
        auto const json = nlohmann::json::parse(text);

        if (json.contains("minecraft:geometry")) {
            auto const& geometries = json.at("minecraft:geometry");
            if (geometries.is_array() && !geometries.empty()) {
                auto const& description = geometries.at(0).at("description");
                if (description.contains("identifier"))
                    return description.at("identifier").get<std::string>();
            }
        }
        if (json.contains("geometry")) {
            auto const& geometry = json.at("geometry");
            if (geometry.contains("identifier"))
                return geometry.at("identifier").get<std::string>();
            if (geometry.contains("default"))
                return geometry.at("default").get<std::string>();
        }
    } catch (nlohmann::json::exception const&) {
        // 非法 JSON：调用方回退到 skin.geometry
    }
    return {};
}

std::string parseGeometryVersion(std::string const& text) {
    try {
        auto const json = nlohmann::json::parse(text);
        if (json.contains("format_version"))
            return json.at("format_version").get<std::string>();
    } catch (nlohmann::json::exception const&) {
    }
    return "1.21.100";
}

} // namespace debugshape_export::skin
