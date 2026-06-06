set_project("threecpp_renderer")
set_version("6.0.60")
set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_defines("GLM_ENABLE_EXPERIMENTAL")

-- glm is intentionally pulled as the official third-party package. No mini_glm or local replacement is used.
add_requires("glm", "glfw")
add_requires("stb")
add_requires("tbb", {optional = true})
add_requires("assimp", {optional = true})

option("use_angle")
    set_default(false)
    set_showmenu(true)
    set_description("Use ANGLE/EGL/OpenGL ES backend. OFF by default for the macOS GLFW native OpenGL test.")
option_end()

option("angle_dir")
    set_default("")
    set_showmenu(true)
    set_description("ANGLE install directory containing include/ and lib/. Example: C:/deps/angle")
option_end()


option("enable_basisu")
    set_default(false)
    set_showmenu(true)
    set_description("Enable external Basis Universal transcoder integration hook if basisu sources are added by the user")
option_end()

option("enable_assimp")
    set_default(false)
    set_showmenu(true)
    set_description("Build Assimp loader and Assimp viewer if Assimp package is available")
option_end()

if has_config("use_angle") then
    add_defines("THREECPP_USE_ANGLE=1")
else
    add_defines("THREECPP_USE_ANGLE=0")
end

if has_config("enable_assimp") then
    add_defines("THREECPP_ENABLE_ASSIMP=1")
else
    add_defines("THREECPP_ENABLE_ASSIMP=0")
end

if has_config("enable_basisu") then
    add_defines("THREECPP_ENABLE_BASISU=1")
else
    add_defines("THREECPP_ENABLE_BASISU=0")
end

includes("src")
includes("examples")
