-- include subprojects
includes("lib/commonlibsse-ng")

-- set project constants
set_project("HiddenLoot")
set_version("1.1.3")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- define targets
target("HiddenLoot")
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
