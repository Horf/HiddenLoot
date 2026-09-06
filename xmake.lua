-- include subprojects
includes("lib/commonlibsse-ng")

-- set project constants
set_project("HiddenLoot")
set_version("1.5.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- add requirenments
add_requires("nlohmann_json")

-- define targets
target("HiddenLoot")
    add_deps("commonlibsse-ng")

    add_packages("nlohmann_json")

    add_rules("commonlibsse-ng.plugin", {
        name = "HiddenLoot",
        author = "PRieST47",
        description = "Hides Armor, Shields, Clothing and/or Weapons while looting NPCs"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
