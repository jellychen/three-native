cmake_minimum_required(VERSION 3.24)
project(threecpp_renderer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(THREECPP_USE_ANGLE "Use ANGLE/EGL/OpenGL ES backend" OFF)
option(THREECPP_ENABLE_ASSIMP "Build Assimp model loader and Assimp viewer example" ON)
option(THREECPP_ENABLE_TBB "Link oneTBB when available" ON)
option(THREECPP_FETCH_DEPS "Fetch glm/glfw from GitHub when packages are not already available" ON)
option(THREECPP_BUILD_EXAMPLES "Build examples" ON)
set(THREECPP_ANGLE_DIR "" CACHE PATH "ANGLE install directory containing include/ and lib/. Example: C:/deps/angle")
set(THREECPP_ANGLE_BIN_DIR "" CACHE PATH "ANGLE runtime directory containing libEGL/libGLESv2 DLLs, dylibs, or SOs")

include(FetchContent)

# glm: official upstream only. This project intentionally does not ship or use mini_glm.
find_package(glm QUIET)
if(NOT glm_FOUND)
    if(NOT THREECPP_FETCH_DEPS)
        message(FATAL_ERROR "glm not found. Install glm or enable THREECPP_FETCH_DEPS.")
    endif()
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    if(NOT THREECPP_FETCH_DEPS)
        message(FATAL_ERROR "glfw3 not found. Install glfw3 or enable THREECPP_FETCH_DEPS.")
    endif()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

if(THREECPP_ENABLE_TBB)
    find_package(TBB QUIET)
    if(TBB_FOUND)
        message(STATUS "threecpp: oneTBB found and enabled")
    else()
        message(WARNING "threecpp: oneTBB not found; renderer test will still build without TBB acceleration")
        set(THREECPP_ENABLE_TBB OFF)
    endif()
endif()

if(THREECPP_ENABLE_ASSIMP)
    find_package(assimp QUIET)
    if(NOT assimp_FOUND)
        message(WARNING "threecpp: Assimp not found; disabling Assimp loader and Assimp viewer example")
        set(THREECPP_ENABLE_ASSIMP OFF)
    endif()
endif()

file(GLOB_RECURSE THREECPP_SOURCES CONFIGURE_DEPENDS src/*.cc)
if(NOT THREECPP_ENABLE_ASSIMP)
    list(FILTER THREECPP_SOURCES EXCLUDE REGEX ".*/src/loader/AssimpLoader\\.cc$")
endif()

add_library(threecpp STATIC ${THREECPP_SOURCES})
target_include_directories(threecpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(threecpp PUBLIC glm::glm glfw)
target_compile_definitions(threecpp PUBLIC THREECPP_USE_ANGLE=$<IF:$<BOOL:${THREECPP_USE_ANGLE}>,1,0>)
target_compile_definitions(threecpp PUBLIC THREECPP_ENABLE_ASSIMP=$<IF:$<BOOL:${THREECPP_ENABLE_ASSIMP}>,1,0>)
target_compile_definitions(threecpp PUBLIC THREECPP_ENABLE_TBB=$<IF:$<BOOL:${THREECPP_ENABLE_TBB}>,1,0>)

if(THREECPP_ENABLE_TBB)
    target_link_libraries(threecpp PUBLIC TBB::tbb)
endif()

if(THREECPP_ENABLE_ASSIMP)
    target_link_libraries(threecpp PUBLIC assimp::assimp)
endif()

if(THREECPP_USE_ANGLE)
    if(THREECPP_ANGLE_DIR)
        target_include_directories(threecpp PUBLIC ${THREECPP_ANGLE_DIR}/include)
        if(EXISTS "${THREECPP_ANGLE_DIR}/lib")
            target_link_directories(threecpp PUBLIC ${THREECPP_ANGLE_DIR}/lib)
        endif()
        if(EXISTS "${THREECPP_ANGLE_DIR}/lib64")
            target_link_directories(threecpp PUBLIC ${THREECPP_ANGLE_DIR}/lib64)
        endif()
    endif()
    target_link_libraries(threecpp PUBLIC EGL GLESv2)
else()
    if(WIN32)
        target_link_libraries(threecpp PUBLIC opengl32)
    elseif(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
        target_link_libraries(threecpp PUBLIC ${OPENGL_FRAMEWORK})
    else()
        target_link_libraries(threecpp PUBLIC GL)
    endif()
endif()

function(threecpp_add_example name)
    add_executable(${name} examples/${name}/main.cc)
    target_link_libraries(${name} PRIVATE threecpp)
    if(THREECPP_ANGLE_BIN_DIR)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${THREECPP_ANGLE_BIN_DIR}"
                    "$<TARGET_FILE_DIR:${name}>"
            COMMENT "Copying ANGLE runtime next to ${name}")
    endif()
endfunction()

if(THREECPP_BUILD_EXAMPLES)
    foreach(example
        00-clear-screen
        01-basic-primitives
        03-pbr-lights
        04-fatline
        05-skinning
        06-physical-transmission
        07-transmission-render-target
        08-macos-glfw-native-test
        09-pbr-envmap
        10-pmrem-lut
        11-material-geometry-matrix
        12-shadow-map
        13-v2-0-threejs-stack
    )
        threecpp_add_example(${example})
    endforeach()

    if(THREECPP_USE_ANGLE)
        threecpp_add_example(08-glfw-angle-renderer-test)
    endif()

    if(THREECPP_ENABLE_ASSIMP)
        threecpp_add_example(02-assimp-viewer)
    endif()
endif()
