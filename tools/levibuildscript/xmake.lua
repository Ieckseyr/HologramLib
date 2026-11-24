-- levibuildscript 0.6.2 的 linkrule / modpacker 内联版（离线构建复用不了已装的包）。
-- 用法：includes("tools/levibuildscript") 后 add_rules("linkrule"/"modpacker")。

-- 与原版差异：prelink.exe / bedrock_runtime_data 的定位顺序改为
--   环境变量 HOLO_PRELINK / HOLO_BEDROCK_DATA → 包环境 PATH → 固定路径。
rule("linkrule")
    on_config(function(target)
        target:add("shflags", "/DELAYLOAD:bedrock_runtime.dll")
    end)

    before_link(function(target)
        import("lib.detect.find_file")
        import("core.project.config")
        import("core.project.depend")

        local plat = config.get("plat") or "windows"
        local arch = config.get("arch") or "x64"
        local target_type = config.get("target_type") or "server"

        local builddir = config.builddir()
        local outdir = path.join(builddir, ".prelink")
        local libdir = path.join(outdir, "lib")

        os.mkdir(libdir)

        local envs = os.joinenvs(target:pkgenvs(), os.getenvs())
        local searchpaths = path.splitenv(envs.PATH or envs.Path or envs.path or "")

        -- 898 线用 prelink v0.7.1 + bedrockdata v1.21.132-server.8；v0.8.4 会读不了这份 data。
        local pkgroot = path.join(os.getenv("LOCALAPPDATA") or "", ".xmake", "packages")

        local data = os.getenv("HOLO_BEDROCK_DATA")
        if data == "" or (data and not os.isfile(data)) then
            data = nil
        end
        if not data then
            data = find_file("bedrock_runtime_data", searchpaths)
        end
        if not data then
            local guess = path.join(
                pkgroot, "b", "bedrockdata", "v1.21.132-server.8"
            )
            for _, dir in ipairs(os.dirs(path.join(guess, "*"))) do
                local candidate = path.join(dir, "bin", "bedrock_runtime_data")
                if os.isfile(candidate) then
                    data = candidate
                    break
                end
            end
        end
        if not data then
            local sdk = os.getenv("HOLO_LL_SDK_DIR")
            if sdk and sdk ~= "" then
                local candidate = path.join(sdk, "tools", "bedrock_runtime_data")
                if os.isfile(candidate) then
                    data = candidate
                end
            end
        end
        if not data then
            local fallback = path.join(os.projectdir(), "..", "..", "bedrock_runtime_data")
            if os.isfile(fallback) then
                data = path.absolute(fallback)
            end
        end
        assert(data, "Cannot find bedrock_runtime_data (set HOLO_BEDROCK_DATA)")

        local link = os.getenv("HOLO_PRELINK")
        if link == "" or (link and not os.isfile(link)) then
            link = nil
        end
        if not link then
            link = find_file("prelink.exe", searchpaths)
        end
        if not link then
            local sdk = os.getenv("HOLO_LL_SDK_DIR")
            if sdk and sdk ~= "" then
                local candidate = path.join(sdk, "tools", "prelink.exe")
                if os.isfile(candidate) then
                    link = candidate
                end
            end
        end
        if not link then
            local guess = path.join(pkgroot, "p", "prelink", "v0.7.1")
            for _, dir in ipairs(os.dirs(path.join(guess, "*"))) do
                local candidate = path.join(dir, "bin", "prelink.exe")
                if os.isfile(candidate) then
                    link = candidate
                    break
                end
            end
        end
        assert(link, "Cannot find prelink.exe (set HOLO_PRELINK)")

        local inputs = table.copy(target:objectfiles())

        for _, dep in ipairs(target:orderdeps()) do
            if dep:kind() == "static" then
                local libfile = dep:targetfile()
                if libfile and os.isfile(libfile) then
                    table.insert(inputs, libfile)
                end
            end
        end
        for _, pkg in ipairs(target:orderpkgs()) do
            for _, linkdir in ipairs(pkg:get("linkdirs")) do
                for _, linkname in pairs(pkg:get("links")) do
                    local libfile = path.join(linkdir, linkname .. ".lib")
                    if os.isfile(libfile) then
                        table.insert(inputs, libfile)
                    end
                end
            end
        end

        -- HOLO_SKIP_PRELINK=1 跳过 prelink（排障用）。
        local skip = os.getenv("HOLO_SKIP_PRELINK") or ""
        if skip ~= "1" then
            -- wrap the expensive prelink step with depend.on_changed
            depend.on_changed(function ()
                print("Running prelink...")

                os.execv(link, {
                    string.format("%s-%s-%s", target_type, plat, arch),
                    outdir,
                    data,
                    table.unpack(inputs)
                })
            end, {
                changed = target:is_rebuilt(),
                values = { link, data },
                files = inputs
            })

            target:add("linkdirs", libdir)
            target:add("links", "bedrock_runtime_api")
        end
    end)

    after_link(function(target)
        os.rm("$(builddir)/.prelink/lib/*.lib")
    end)
rule_end()

-- 与原版差异：extraconf 规则名同时认 "@levibuildscript/modpacker" 和 "modpacker"。
rule("modpacker")
after_build(function(target)
    local mod_define = target:extraconf("rules", "@levibuildscript/modpacker")
        or target:extraconf("rules", "modpacker")
    mod_define = mod_define or {}
    mod_define.modName = mod_define.modName or target:name()
    mod_define.modFile = mod_define.modFile or path.filename(target:targetfile())
    if mod_define.modVersion == nil then
        local tag = (os.iorunv("git", {"describe", "--tags", "--abbrev=0", "--always"}) or ""):trim()
        local major, minor, patch, suffix = tag:match("v(%d+)%.(%d+)%.(%d+)(.*)")
        if not major then
            print("Failed to parse version tag, using 0.0.0")
            major, minor, patch = 0, 0, 0
        end
        mod_define.modVersion = mod_define.modVersion or (major .. "." .. minor .. "." .. patch)
    end
    import("core.project.config")
    mod_define.modPlatform = mod_define.modPlatform or config.get("target_type")

    function string_formatter(str, variables)
        return (str:gsub("%${(.-)}", function(var)
            return variables[var] or "${" .. var .. "}"
        end))
    end

    function pack_mod(target, mod_define)
        import("lib.detect.find_file")

        local manifest_path = find_file("manifest.json", os.projectdir())
        if manifest_path then
            local manifest = io.readfile(manifest_path)
            local bindir = path.join(os.projectdir(), "bin")
            local outputdir = path.join(bindir, mod_define.modName)
            local targetfile = path.join(outputdir, mod_define.modFile)
            local pdbfile = path.join(outputdir, path.basename(mod_define.modFile) .. ".pdb")
            local manifestfile = path.join(outputdir, "manifest.json")
            local oritargetfile = target:targetfile()
            local oripdbfile = path.join(path.directory(oritargetfile), path.basename(oritargetfile) .. ".pdb")

            os.mkdir(outputdir)
            os.cp(oritargetfile, targetfile)
            if os.isfile(oripdbfile) then
                os.cp(oripdbfile, pdbfile)
            end

            local formattedmanifest = string_formatter(manifest, mod_define)
            io.writefile(manifestfile, formattedmanifest)
            cprint("${bright green}[Mod Packer]: ${reset}mod already generated to " .. outputdir)
        else
            cprint("${bright yellow}warn: ${reset}not found manifest.json in root dir!")
        end
    end

    pack_mod(target, mod_define)
end)
