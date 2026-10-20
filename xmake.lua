add_rules("mode.debug", "mode.release")

-- release 产 PDB（部署链已含 PDB; 崩溃栈符号化必需）
if is_mode("release") then
    add_cxflags("/Zi", "/FS", {force = true})
    add_ldflags("/DEBUG:FULL", {force = true})
end

-- 优先使用本地缓存的 liteldev-repo（含 26.51.0 登记以匹配全局包缓存），回退远程
local function find_local_repo()
    local candidates = {
        path.join(os.projectdir(), ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowMenu", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowKb", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowPAPI", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "MeowSidebar", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
        path.join(os.projectdir(), "..", "ItemPhys-main", ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"),
    }
    -- 必须含 levilamina 26.51.0 版本登记, 防止捡到旧版缓存 repo 导致解析回退到旧版本
    for _, p in ipairs(candidates) do
        if os.exists(path.join(p, "packages", "l", "levilamina", "versions", "26_51_0.lua")) then
            return p
        end
    end
    return nil
end
local local_repo = find_local_repo()
if local_repo then
    add_repositories("liteldev-repo " .. local_repo)
else
    add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
end

-- 与服务端(26.51 / BDS 1.26.51)匹配的 levilamina 版本（bedrockdata v26.51.1-server.2）
if is_config("target_type", "server") then
    add_requires("levilamina 26.51.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 26.51.0", {configs = {target_type = "client"}})
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

-- 诊断日志: 默认关闭 —— HologramLib 是纯前置库, 正式产物不携带任何日志
-- （日志字符串在关闭时不会进二进制）。排障: xmake f --holo_diag=y && xmake build
option("holo_diag")
    set_default(false)
    set_showmenu(true)
    set_description("把 HologramLib 的诊断日志编进产物（排障用; 正式发布必须保持关闭）")
option_end()

target("HologramLib")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags("/EHa", "/utf-8", "/W4")
    add_defines("NOMINMAX", "UNICODE", "HOLOGLIB_EXPORTS")
    if has_config("holo_diag") then
        add_defines("HOLOGLIB_DIAG_LOG=1")
    end
    add_packages("levilamina")
    add_packages("magic_enum")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++23")

    -- BedrockProtocol-main 静态库（CMake 构建后安装到本地, 26.51 补丁: TextDataPayload.LineGapHeight
    -- 插在 BackgroundColor 与 DepthTest 之间, ItemStackResponseInfo.Containers 改 optional 编码）
    add_includedirs("../BedrockProtocol-main/install/include")
    add_linkdirs("../BedrockProtocol-main/install/lib")
    add_links("Protocol")

    -- GDI+（NPC 皮肤 PNG 解码）
    add_syslinks("gdiplus")

    -- 对外公开头（native 消费插件 include 此目录）
    add_headerfiles("include/(hologramlib/HologramLib.h)")

    -- 源文件: 目录通配 —— 新增/删除 .cpp/.h 不再需要改这份清单。
    -- 旧清单是 40 行手工 add_files, 曾漏注册 ghost/GhostInteractRouter.cpp,
    -- 于是它被 #include 进了 HologramLibImpl.cpp（见 git log 与 docs/architecture/overview.md）。
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src", "include")
    set_symbols("debug") -- 崩溃栈符号化（部署链已含 PDB; 原值 hidden）
    add_ldflags("/OPT:REF", "/OPT:ICF")

    if is_config("target_type", "server") then
        add_defines("LL_PLAT_S")
    else
        add_defines("LL_PLAT_C")
    end
