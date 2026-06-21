add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("client")
    set_showmenu(true)
    set_values("server", "client")
option_end()

package("levilamina")
    add_configs("target_type", {default = "server", values = {"server", "client"}})
    add_defines("ENTT_PACKED_PAGE=128", "ENTT_SPARSE_PAGE=2048", "ENTT_NO_MIXIN")
    add_deps("entt v3.15.0")
    add_deps("expected-lite v0.8.0")
    add_deps("fmt 11.2.0")
    add_deps("gsl v4.2.0")
    add_deps("glm 1.0.1")
    add_deps("leveldb 1.23")
    add_deps("magic_enum v0.9.7")
    add_deps("nlohmann_json v3.11.3")
    add_deps("rapidjson v1.1.0")
    add_deps("type_safe v0.2.4")
    add_deps("pcg_cpp v1.0.0")
    add_deps("pfr 2.1.1")
    add_deps("symbolprovider v1.2.0")
    add_deps("parallel-hashmap v1.3.12")
    add_deps("concurrentqueue v1.0.4")
    add_deps("stb 2025.03.14")
    add_deps("demangler v17.0.7")
    add_deps("mimalloc v2.1.7")
    add_deps("cpr[ssl=y] 1.11.1")
    add_deps("trampoline 2024.11.7")
    add_deps("preloader v1.15.7")
    add_deps("libhat 0.4.0")
    on_load(function(package)
        if package:config("target_type") == "server" then
            package:add("defines", "LL_PLAT_S")
            package:add("deps", "bedrockdata v26.10.4-server.17")
        else
            package:add("defines", "LL_PLAT_C")
            package:add("deps", "bedrockdata v26.10.4-client.17")
        end
    end)
    set_sourcedir("D:/FastRenderer/LeviLamina")
    on_install(function(package)
        if package:config("target_type") == "server" then
            import("package.tools.xmake").install(package)
        else
            import("package.tools.xmake").install(package, {"--target_type=client"})
        end
    end)
package_end()

add_requires("levilamina", {configs = {target_type = get_config("target_type")}})
add_requires("levibuildscript")

add_requires("imgui", {configs = {shared = false, win32 = true, dx11 = true}})
add_requires("minhook", {configs = {shared = false}})
add_requires("nlohmann_json")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("FastRenderer-Client")
    add_rules("@levibuildscript/linkrule")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204",
        "/wd4100",
        "/wd4189"
    )
    add_defines("NOMINMAX", "UNICODE", "FR_EXPORT")
    add_packages("levilamina", "imgui", "minhook", "nlohmann_json")
    add_syslinks("d3d11", "dxgi", "user32", "delayimp")
    add_ldflags("/DELAYLOAD:dwmapi.dll", "/DELAYLOAD:imm32.dll", "/DELAYLOAD:LeviLamina.dll")
    add_shflags("/DELAYLOAD:dwmapi.dll", "/DELAYLOAD:imm32.dll", "/DELAYLOAD:LeviLamina.dll")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("../FastRenderer-Core/src/**.h")
    add_headerfiles("../FastRenderer-SDK/include/**.h")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("../FastRenderer-Core/src")
    add_includedirs("../FastRenderer-SDK/include")
    add_includedirs("src")
    after_build(function(target)
        local bindir = path.join(os.projectdir(), "..", "bin", "FastRenderer")
        os.mkdir(bindir)
        os.cp(target:targetfile(), bindir .. "/FastRenderer-Client.dll")
        os.cp(target:symbolfile(), bindir)
        os.cp(path.join(os.projectdir(), "..", "FastRenderer-Client-Win", "manifest.json"), bindir .. "/manifest.json")
        os.cp(path.join(os.projectdir(), "..", "FastRenderer-Client-Win", "gui_defs"), bindir .. "/gui_defs")
    end)
