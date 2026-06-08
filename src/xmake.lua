target("three")
    set_kind("static")
    add_files("**.cc")
    if not has_config("enable_assimp") then
        remove_files("loader/assimp-loader.cc")
    end
    add_includedirs(".", {public = true})
    add_packages("glm", "glfw", {public = true})
    add_packages("stb", {public = true})
    add_defines("THREECPP_ENABLE_STB_IMAGE=1", {public = true})
    if has_package("tbb") then
        add_packages("tbb", {public = true})
        add_defines("THREECPP_ENABLE_TBB=1", {public = true})
    else
        add_defines("THREECPP_ENABLE_TBB=0", {public = true})
    end
    if has_config("enable_assimp") then
        add_packages("assimp", {public = true})
        add_defines("THREECPP_ENABLE_ASSIMP=1", {public = true})
    else
        add_defines("THREECPP_ENABLE_ASSIMP=0", {public = true})
    end
    if has_config("enable_basisu") then
        add_defines("THREECPP_ENABLE_BASISU=1", {public = true})
    else
        add_defines("THREECPP_ENABLE_BASISU=0", {public = true})
    end
    if has_config("use_angle") then
        local angle_dir = get_config("angle_dir")
        if angle_dir and angle_dir ~= "" then
            add_includedirs(path.join(angle_dir, "include"), {public = true})
            add_linkdirs(path.join(angle_dir, "lib"), {public = true})
            add_linkdirs(path.join(angle_dir, "lib64"), {public = true})
        end
        add_links("EGL", "GLESv2", {public = true})
    else
        if is_plat("windows") then
            add_links("opengl32", {public = true})
        elseif is_plat("macosx") then
            add_frameworks("OpenGL", {public = true})
        else
            add_links("GL", {public = true})
        end
    end
