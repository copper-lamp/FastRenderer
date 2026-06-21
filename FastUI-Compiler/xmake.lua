add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")
add_requires("pugixml")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("FastUI-Compiler")
    set_kind("binary")
    set_languages("c++20")
    set_symbols("debug")
    add_packages("nlohmann_json", "pugixml")
    add_files("src/**.cpp")
    add_includedirs("src")
