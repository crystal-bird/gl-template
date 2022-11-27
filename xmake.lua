
add_rules("mode.debug", "mode.release")

set_rundir(".")
set_targetdir("bin")
set_objectdir("bin-int")

set_warnings("allextra", "error")
set_languages("clatest")
add_defines("_CRT_SECURE_NO_WARNINGS")

add_requires("glfw", "glad", "stb")

target("gl-template")
    set_kind("binary")
    add_files("src/**.c")

    add_packages("glfw", "glad", "stb")