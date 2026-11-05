add_rules("mode.debug", "mode.release")

-- 缺失符号报告规则（丢在项目根目录的 symbol_missing.lua / symbol_missing.exe）
-- 编译后会在项目根目录产出 symbol_missing.txt / symbol_missing.json
includes("symbol_missing.lua")

-- 构建后部署: DLL / PDB / 导入库落回插件目录（服务器实际加载位置），
-- 并把脚本导入层（lib/*.js|d.ts|exports.json）搬到 bin/HologramLib/lib/。
-- 用 rule 承载 after_build：与 @levibuildscript/modpacker 同用时 target 级 after_build 实测不触发，
-- rule 级能稳定触发。
rule("holo_deploy")
    after_build(function (target)
        local projectdir = os.projectdir()
        local targetfile = target:targetfile()
        if not os.isfile(targetfile) then
            return
        end
        -- 部署单个文件：直接 cp 失败（目标被正在跑的服务端加载占用）就改名旧文件再拷。
        -- 注意：xmake 回调沙箱里没有 pcall，用 try/catch（在回调环境里才有）。
        local function deploy(src, dst)
            if not os.isfile(src) then
                return false
            end
            local ok = false
            try {
                function ()
                    os.cp(src, dst, {force = true})
                    ok = true
                end,
                catch {
                    function (e)
                    end
                }
            }
            if ok then
                return true
            end
            local backup = dst .. ".old"
            try {
                function ()
                    os.rm(backup)
                end,
                catch {
                    function (e)
                    end
                }
            }
            try {
                function ()
                    os.mv(dst, backup)
                end,
                catch {
                    function (e)
                    end
                }
            }
            try {
                function ()
                    os.cp(src, dst, {force = true})
                    ok = true
                end,
                catch {
                    function (e)
                    end
                }
            }
            if ok then
                cprint("${yellow}[HoloDeploy]: ${reset}目标被占用，旧文件改名为 " .. backup)
            else
                print("warn: 部署失败（文件被占用且改名失败）: " .. dst)
            end
            return ok
        end

        deploy(targetfile, path.join(projectdir, "HologramLib.dll"))
        local pdbfile = targetfile:gsub("%.dll$", ".pdb")
        deploy(pdbfile, path.join(projectdir, "HologramLib.pdb"))
        local implib = targetfile:gsub("%.dll$", ".lib")
        deploy(implib, path.join(projectdir, "HologramLib.lib"))
        cprint("${bright green}[HoloDeploy]: ${reset}DLL / PDB / 导入库已部署到插件目录")

        local outdir = path.join(projectdir, "bin", target:name(), "lib")
        os.mkdir(outdir)
        for _, name in ipairs({ "HologramLib.js", "HologramLib.d.ts", "exports.json" }) do
            local src = path.join(projectdir, "lib", name)
            if os.isfile(src) then
                deploy(src, path.join(outdir, name))
            else
                print("warn: 缺少 lib/" .. name .. "（先跑 python tools/gen_lse_shim.py）")
            end
        end
    end)
rule_end()


-- release 产 PDB（部署链已含 PDB; 崩溃栈符号化必需）
if is_mode("release") then
    add_cxflags("/Zi", "/FS", {force = true})
    add_ldflags("/DEBUG:FULL", {force = true})
end

-- 本地 liteldev-repo 缓存
local function find_local_repo()
    local candidates = {}
    -- 换机器/CI 用这个指过来
    local env_repo = os.getenv("HOLO_REPO_DIR")
    if env_repo and env_repo ~= "" then
        table.insert(candidates, env_repo)
    end
    -- 服务器根目录的离线仓库（26.60 SDK，预构建 levilamina 26.60.0）
    table.insert(candidates, path.join(os.projectdir(), "..", "..", ".llsdk", "liteldev-repo"))
    -- 本机缓存, 再退到兄弟工程
    table.insert(candidates, path.join(os.projectdir(), ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"))
    for _, sibling in ipairs({ "MeowMenu", "MeowKb", "MeowPAPI", "MeowSidebar", "ItemPhys-main" }) do
        table.insert(candidates, path.join(os.projectdir(), "..", sibling, ".xmake", os.host(), os.arch(), "repositories", "liteldev-repo"))
    end
    -- 只认带 26.60.0 登记的那份, 别捡到旧缓存
    for _, p in ipairs(candidates) do
        if os.exists(path.join(p, "packages", "l", "levilamina", "versions", "26_60_0.lua")) then
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

-- 插件统一用 clang-cl（与 LeviLamina 本体一致；缓存里的依赖包也是这套工具链）
if is_plat("windows") then
    set_toolchains("clang-cl")
end

-- 与服务端(26.60 / BDS 1.26.60.6)匹配的 levilamina 版本（bedrockdata v26.60.6-server.1）
if is_config("target_type", "server") then
    add_requires("levilamina 26.60.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 26.60.0", {configs = {target_type = "client"}})
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

-- 警告当错误, CI 用; xmake f --holo_werror=y
option("holo_werror")
    set_default(false)
    set_showmenu(true)
    set_description("警告当错误 /WX")
option_end()

-- ASan（MSVC 只有 address, 没有 UBSan）; xmake f --holo_asan=y
option("holo_asan")
    set_default(false)
    set_showmenu(true)
    set_description("AddressSanitizer /fsanitize=address")
option_end()

target("HologramLib")
    add_rules("@levibuildscript/linkrule")
    add_rules("symbol_missing")
    add_rules("@levibuildscript/modpacker")
    add_rules("holo_deploy")
    -- 不加 /EHa, 与 set_exceptions 冲突
    add_cxflags("/utf-8", "/W4")
    if has_config("holo_werror") then
        -- deprecated 的 6 处是 MCAPI 收紧后的登记入口（见验收标准「已登记豁免」），
        -- 严格模式只把它们降级，其余告警依旧 -Werror
        add_cxflags("/WX", "-Wno-deprecated-declarations")
    end
    if has_config("holo_asan") then
        add_cxflags("/fsanitize=address")
        add_ldflags("/fsanitize=address")
    end
    add_defines("NOMINMAX", "UNICODE", "HOLOGLIB_EXPORTS")
    -- getenv 这类告警项目级关掉
    add_defines("_CRT_SECURE_NO_WARNINGS")
    if has_config("holo_diag") then
        add_defines("HOLOGLIB_DIAG_LOG=1")
    end
    add_packages("levilamina")
    add_packages("magic_enum")
    -- 标准 C++ 异常 /EHsc
    set_exceptions("cxx")
    set_kind("shared")
    set_languages("c++23")

    -- BedrockProtocol 静态库
    -- 26.51 补丁: TextDataPayload.LineGapHeight 插在 BackgroundColor 与 DepthTest 之间,
    -- ItemStackResponseInfo.Containers 改 optional 编码
    -- 目录可用 HOLO_PROTOCOL_DIR 覆盖
    local protocol_install = os.getenv("HOLO_PROTOCOL_DIR")
    if not protocol_install or protocol_install == "" then
        protocol_install = path.join(os.projectdir(), "..", "BedrockProtocol-main", "install")
    end
    if not os.isdir(path.join(protocol_install, "include")) then
        raise(format("找不到 BedrockProtocol: %s\n用 HOLO_PROTOCOL_DIR 指向 install/ 目录", protocol_install))
    end
    -- 三方头走 -isystem：clang 不会对系统头里的属性/弃用告警发噪音
    -- （sculk 的 ValueTag/TagVariant/MetaData 用 [[no_unique_address]]，clang 22 会报 unknown-attributes）
    add_sysincludedirs(path.join(protocol_install, "include"))
    add_linkdirs(path.join(protocol_install, "lib"))
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
