set_project("farfarwest")
set_version("0.1.0")

add_repositories("gf-repo https://github.com/GamedevFramework/xmake-repo")

add_requires("gamedevframework2", "nlohmann_json")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "$(buildir)"})

set_policy("build.warning", true)
set_warnings("allextra")
set_languages("cxx17")
set_encodings("utf-8")

if is_plat("windows") then
  add_cxflags("/wd4251") -- Disable warning: class needs to have dll-interface to be used by clients of class blah blah blah
  add_defines("NOMINMAX", "_CRT_SECURE_NO_WARNINGS")
end

set_configdir("$(buildir)/config")
set_configvar("FARFARWEST_DATADIR", "$(projectdir)/data/farfarwest")
add_configfiles("code/config.h.in", {pattern = "@(.-)@"})

target("farfarwest")
    set_kind("binary")
    add_files("code/farfarwest.cc")
    add_files("code/bits/*.cc")
    add_includedirs("$(buildir)/config")
    add_packages("gamedevframework2", "nlohmann_json")

target("world-generation")
    set_kind("binary")
    add_files("code/world-generation.cc")
    add_files("code/bits/Date.cc")
    add_files("code/bits/Names.cc")
    add_files("code/bits/*State.cc")
    add_files("code/bits/WorldGeneration.cc")
    add_packages("gamedevframework2", "nlohmann_json")

target("name-generation")
    set_kind("binary")
    add_files("code/name-generation.cc")
    add_files("code/bits/Names.cc")
    add_packages("gamedevframework2")
