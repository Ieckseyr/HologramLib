add_rules("mode.debug", "mode.release")

-- 优先使用本地缓存的 liteldev-repo（含 bedrockdata server.17 登记以匹配全局包缓存），回退远程
local function find_local_repo()
    local candidates = {
        path.join(os.projectdir(), ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowMenu", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowKb", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowPAPI", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "ItemPhys-main", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
    }
    for _, p in ipairs(candidates) do
        if os.exists(p) then return p end
    end
    return nil
end
local local_repo = find_local_repo()
if local_repo then
    add_repositories("liteldev-repo " .. local_repo)
else
    add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
end

-- 与服务端(26.10.14 / 协议944)匹配的 levilamina 版本
if is_config("target_type", "server") then
    add_requires("levilamina 26.10.14", {configs = {target_type = "server"}})
else
    add_requires("levilamina 26.10.14", {configs = {target_type = "client"}})
end

add_requires("levibuildscript")
add_requires("magic_enum v0.9.7")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

target("HologramLib")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags("/EHa", "/utf-8", "/W4")
    add_defines("NOMINMAX", "UNICODE", "HOLOGLIB_EXPORTS")
    add_packages("levilamina")
    add_packages("magic_enum")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++23")

    -- BedrockProtocol-944 静态库（CMake 构建后安装到本地）
    add_includedirs("../BedrockProtocol-944/install/include")
    add_linkdirs("../BedrockProtocol-944/install/lib")
    add_links("Protocol")

    -- 对外公开头（native 消费插件 include 此目录）
    add_headerfiles("include/(hologramlib/HologramLib.h)")

    -- 插件源文件
    add_headerfiles("src/ProtocolShape.h")
    add_headerfiles("src/PacketDebugRenderer.h")
    add_headerfiles("src/RemoteCallExporter.h")
    add_headerfiles("src/ModEntry.h")
    add_headerfiles("src/ProtocolPackets.h")
    add_headerfiles("src/FloatingTextExporter.h")
    add_headerfiles("src/FloatingTextManager.h")
    add_headerfiles("src/GradientLineExporter.h")
    add_headerfiles("src/GradientLineManager.h")
    add_headerfiles("src/lse/RemoteCallTypes.h")
    add_headerfiles("src/lse/LseBridge.h")
    add_headerfiles("src/itemdetail/ItemDetailManager.h")
    add_headerfiles("src/itemdetail/ItemDetailExporter.h")
    add_headerfiles("src/itemdisplay/ItemDisplayManager.h")
    add_headerfiles("src/itemdisplay/ItemDisplayExporter.h")
    add_files("src/PacketDebugRenderer.cpp")
    add_files("src/RemoteCallExporter.cpp")
    add_files("src/ModEntry.cpp")
    add_files("src/ProtocolPackets.cpp")
    add_files("src/MemoryOperators.cpp")
    add_files("src/FloatingTextExporter.cpp")
    add_files("src/FloatingTextManager.cpp")
    add_files("src/GradientLineExporter.cpp")
    add_files("src/GradientLineManager.cpp")
    add_files("src/lse/LseBridge.cpp")
    add_files("src/itemdetail/ItemDetailManager.cpp")
    add_files("src/itemdetail/ItemDetailExporter.cpp")
    add_files("src/itemdisplay/ItemDisplayManager.cpp")
    add_files("src/itemdisplay/ItemDisplayExporter.cpp")
    add_files("src/HologramLibImpl.cpp")
    add_includedirs("src", "include")
    set_symbols("hidden")
    add_ldflags("/OPT:REF", "/OPT:ICF", "/DEBUG:NONE")

    if is_config("target_type", "server") then
        add_defines("LL_PLAT_S")
    else
        add_defines("LL_PLAT_C")
    end
