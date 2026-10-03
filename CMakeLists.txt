# eXngine
#
#   cmake --preset default            (or: cmake --preset clang)
#   cmake --build --preset debug      (or: release, clang-debug, clang-release)
#
# Everything CMake generates stays inside output/: build files in output/cmake/<preset>
# (see CMakePresets.json), binaries in output/<arch>/<config>, shaders in output/shaders.
#
# Everything else happens automatically:
#   - GLFW is built from 3rdparty/glfw, glm / stb / imgui come from 3rdparty/
#   - Vulkan SDK (headers, vulkan-1.lib, glslangValidator) is located via VULKAN_SDK
#   - FBX SDK is located via EXN_FBX_SDK_DIR, FBX_SDK_DIR, SDK_PATH or Program Files
#   - shaders/*.vert|frag|... are compiled to SPIR-V whenever they change
#   - libfbxsdk.dll is copied next to the executable
#
# Binaries go to output/<arch>/<config> and shaders to output/shaders: the
# example loads its shaders from <exe dir>/../../shaders.

cmake_minimum_required(VERSION 3.24)

# Generated files must not spread over the source tree: accept a build folder inside
# output/ or outside the repository, refuse the repository root, build/, out/, ...
# Checked before project() so a refused configure does not run compiler detection.
# Only when eXngine is the top-level project: a game that pulls it in with
#   set(EXN_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
#   set(EXN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/eXngine" CACHE PATH "" FORCE)  # optional
#   add_subdirectory(<path to eXngine> eXngine)
# builds it inside its own build folder.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
file(REAL_PATH "${CMAKE_SOURCE_DIR}" EXN_SOURCE_DIR_REAL)
file(REAL_PATH "${CMAKE_BINARY_DIR}" EXN_BINARY_DIR_REAL)
string(TOLOWER "${EXN_SOURCE_DIR_REAL}/" EXN_SOURCE_PREFIX)
string(TOLOWER "${EXN_BINARY_DIR_REAL}/" EXN_BINARY_PREFIX)
string(FIND "${EXN_BINARY_PREFIX}" "${EXN_SOURCE_PREFIX}" EXN_IN_SOURCE_TREE)
string(FIND "${EXN_BINARY_PREFIX}" "${EXN_SOURCE_PREFIX}output/" EXN_IN_OUTPUT_DIR)

if(EXN_IN_SOURCE_TREE EQUAL 0 AND NOT EXN_IN_OUTPUT_DIR EQUAL 0)
    message(FATAL_ERROR
        "Build files would be written to '${CMAKE_BINARY_DIR}', outside output/.\n"
        "Configure with a preset instead:\n"
        "    cmake --preset default\n"
        "    cmake --build --preset debug\n"
        "Then delete the CMakeCache.txt and CMakeFiles/ this attempt left in '${CMAKE_BINARY_DIR}'.")
endif()
endif()

project(eXngine VERSION 1.0.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "eXngine currently builds on Windows only.")
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(EXN_BUILD_EXAMPLE "Build the example application" ON)
option(EXN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(EXN_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/output" CACHE PATH "Root folder for binaries and compiled shaders")
set(EXN_FBX_SDK_DIR "" CACHE PATH "FBX SDK root (contains include/ and lib/). Searched automatically when empty")

get_property(EXN_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT EXN_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Output layout
# ---------------------------------------------------------------------------

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EXN_ARCH x64)
else()
    set(EXN_ARCH x32)
endif()

set(EXN_BIN_DIR "${EXN_OUTPUT_DIR}/${EXN_ARCH}/$<CONFIG>")
set(EXN_SHADER_OUTPUT_DIR "${EXN_OUTPUT_DIR}/shaders")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXN_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${EXN_BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${EXN_BIN_DIR}")

set(EXN_3RDPARTY_DIR "${PROJECT_SOURCE_DIR}/3rdparty")

# ---------------------------------------------------------------------------
# Vulkan SDK
# ---------------------------------------------------------------------------

find_package(Vulkan COMPONENTS glslangValidator)
if(NOT Vulkan_FOUND OR NOT TARGET Vulkan::glslangValidator)
    message(FATAL_ERROR
        "Vulkan SDK not found. Install it from https://vulkan.lunarg.com/ and make sure "
        "the VULKAN_SDK environment variable points to it, then re-run CMake.")
endif()

# ---------------------------------------------------------------------------
# Header-only third party libraries
# ---------------------------------------------------------------------------

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE "${EXN_3RDPARTY_DIR}/glm")

add_library(stb_image INTERFACE)
target_include_directories(stb_image SYSTEM INTERFACE "${EXN_3RDPARTY_DIR}/stb_image")

# The copy of stb_truetype that comes with Dear ImGui (imstb_truetype.h).
add_library(stb_truetype INTERFACE)
target_include_directories(stb_truetype SYSTEM INTERFACE "${EXN_3RDPARTY_DIR}/imgui")

# ---------------------------------------------------------------------------
# GLFW (built from 3rdparty/glfw/src, Win32 backend)
# ---------------------------------------------------------------------------

set(EXN_GLFW_SRC "${EXN_3RDPARTY_DIR}/glfw/src")

add_library(glfw STATIC
    "${EXN_GLFW_SRC}/context.c"
    "${EXN_GLFW_SRC}/init.c"
    "${EXN_GLFW_SRC}/input.c"
    "${EXN_GLFW_SRC}/monitor.c"
    "${EXN_GLFW_SRC}/platform.c"
    "${EXN_GLFW_SRC}/vulkan.c"
    "${EXN_GLFW_SRC}/window.c"
    "${EXN_GLFW_SRC}/egl_context.c"
    "${EXN_GLFW_SRC}/osmesa_context.c"
    "${EXN_GLFW_SRC}/null_init.c"
    "${EXN_GLFW_SRC}/null_monitor.c"
    "${EXN_GLFW_SRC}/null_window.c"
    "${EXN_GLFW_SRC}/null_joystick.c"
    "${EXN_GLFW_SRC}/win32_init.c"
    "${EXN_GLFW_SRC}/win32_joystick.c"
    "${EXN_GLFW_SRC}/win32_module.c"
    "${EXN_GLFW_SRC}/win32_monitor.c"
    "${EXN_GLFW_SRC}/win32_thread.c"
    "${EXN_GLFW_SRC}/win32_time.c"
    "${EXN_GLFW_SRC}/win32_window.c"
    "${EXN_GLFW_SRC}/wgl_context.c")

# GLFW refuses to compile with header option macros (GLFW_INCLUDE_VULKAN, ...)
# defined, so it must not inherit the engine-wide definitions below.
target_include_directories(glfw SYSTEM PUBLIC "${EXN_3RDPARTY_DIR}/glfw/include")
target_compile_definitions(glfw PRIVATE _GLFW_WIN32 UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(glfw PUBLIC gdi32)
set_target_properties(glfw PROPERTIES
    FOLDER "3rdparty"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/$<CONFIG>")

# ---------------------------------------------------------------------------
# FBX SDK
# ---------------------------------------------------------------------------

function(exn_find_fbx_sdk out_var)
    set(candidates)
    foreach(env_var FBX_SDK_DIR FBXSDK_DIR)
        if(DEFINED ENV{${env_var}})
            file(TO_CMAKE_PATH "$ENV{${env_var}}" dir)
            list(APPEND candidates "${dir}")
        endif()
    endforeach()

    # Version folders, newest first: <SDK_PATH>/FBX/<ver>, Program Files/Autodesk/FBX/FBX SDK/<ver>
    set(parents)
    if(DEFINED ENV{SDK_PATH})
        file(TO_CMAKE_PATH "$ENV{SDK_PATH}/FBX" dir)
        list(APPEND parents "${dir}")
    endif()
    if(DEFINED ENV{ProgramFiles})
        file(TO_CMAKE_PATH "$ENV{ProgramFiles}/Autodesk/FBX/FBX SDK" dir)
        list(APPEND parents "${dir}")
    endif()
    foreach(parent IN LISTS parents)
        file(GLOB versions LIST_DIRECTORIES true "${parent}/*")
        list(SORT versions COMPARE NATURAL ORDER DESCENDING)
        list(APPEND candidates ${versions})
    endforeach()

    foreach(dir IN LISTS candidates)
        if(EXISTS "${dir}/include/fbxsdk.h")
            set(${out_var} "${dir}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

if(NOT EXN_FBX_SDK_DIR)
    exn_find_fbx_sdk(EXN_FOUND_FBX_SDK_DIR)
    if(EXN_FOUND_FBX_SDK_DIR)
        set(EXN_FBX_SDK_DIR "${EXN_FOUND_FBX_SDK_DIR}" CACHE PATH
            "FBX SDK root (contains include/ and lib/). Searched automatically when empty" FORCE)
    endif()
endif()

if(EXN_ARCH STREQUAL "x64")
    set(EXN_FBX_ARCH x64)
else()
    set(EXN_FBX_ARCH x86)
endif()

set(EXN_FBX_LIB_DIR "${EXN_FBX_SDK_DIR}/lib/${EXN_FBX_ARCH}")

if(EXN_FBX_SDK_DIR AND EXISTS "${EXN_FBX_LIB_DIR}/release/libfbxsdk.dll")
    message(STATUS "FBX SDK: ${EXN_FBX_SDK_DIR}")
    add_library(fbxsdk SHARED IMPORTED GLOBAL)
    set_target_properties(fbxsdk PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_IMPLIB_DEBUG "${EXN_FBX_LIB_DIR}/debug/libfbxsdk.lib"
        IMPORTED_LOCATION_DEBUG "${EXN_FBX_LIB_DIR}/debug/libfbxsdk.dll"
        IMPORTED_IMPLIB_RELEASE "${EXN_FBX_LIB_DIR}/release/libfbxsdk.lib"
        IMPORTED_LOCATION_RELEASE "${EXN_FBX_LIB_DIR}/release/libfbxsdk.dll"
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL Release)
else()
    message(WARNING
        "FBX SDK not found, falling back to the import library in 3rdparty/lib. "
        "libfbxsdk.dll will not be copied next to the executable. Set EXN_FBX_SDK_DIR "
        "(or the FBX_SDK_DIR environment variable) to the SDK root to fix this.")
    add_library(fbxsdk UNKNOWN IMPORTED GLOBAL)
    set_target_properties(fbxsdk PROPERTIES
        IMPORTED_LOCATION "${EXN_3RDPARTY_DIR}/lib/libfbxsdk.lib")
endif()

# Headers always come from 3rdparty/fbx: the stock 2020.3.x SDK headers contain a
# typo (fbxredblacktree.h: mLefttChild) that recent clang versions reject.
set_target_properties(fbxsdk PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXN_3RDPARTY_DIR}/fbx"
    INTERFACE_COMPILE_DEFINITIONS FBXSDK_SHARED)

# ---------------------------------------------------------------------------
# Shaders (GLSL -> SPIR-V)
# ---------------------------------------------------------------------------

file(GLOB EXN_SHADER_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.geom"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
    "${PROJECT_SOURCE_DIR}/shaders/*.tesc"
    "${PROJECT_SOURCE_DIR}/shaders/*.tese")

set(EXN_SHADER_BINARIES)
foreach(shader IN LISTS EXN_SHADER_SOURCES)
    get_filename_component(shader_name "${shader}" NAME)
    set(spirv "${EXN_SHADER_OUTPUT_DIR}/${shader_name}.spv")
    add_custom_command(
        OUTPUT "${spirv}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXN_SHADER_OUTPUT_DIR}"
        COMMAND Vulkan::glslangValidator -V "${shader}" -o "${spirv}"
        DEPENDS "${shader}"
        COMMENT "Compiling shader ${shader_name}"
        VERBATIM)
    list(APPEND EXN_SHADER_BINARIES "${spirv}")
endforeach()

add_custom_target(eXngine.shaders ALL
    DEPENDS ${EXN_SHADER_BINARIES}
    SOURCES ${EXN_SHADER_SOURCES})
set_target_properties(eXngine.shaders PROPERTIES FOLDER "eXngine")

# ---------------------------------------------------------------------------
# Engine-wide settings
# ---------------------------------------------------------------------------

add_library(eXngine.config INTERFACE)
target_compile_features(eXngine.config INTERFACE cxx_std_20)
target_include_directories(eXngine.config INTERFACE "${PROJECT_SOURCE_DIR}/includes")
target_compile_definitions(eXngine.config INTERFACE
    EXN_IMAGE_STRATEGY_STBI
    NOMINMAX
    _GLFW_WIN32
    GLFW_EXPOSE_NATIVE_WIN32
    GLFW_INCLUDE_VULKAN
    VK_VERSION_1_0)
target_link_libraries(eXngine.config INTERFACE glm Vulkan::Headers)

function(exn_configure_target target)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # C4251: STL members in exported classes - harmless, every module shares one CRT.
        # /utf-8: sources are UTF-8 (e.g. Turkish text literals), not the ANSI code page.
        target_compile_options(${target} PRIVATE /W3 /wd4251 /utf-8 $<$<CXX_COMPILER_ID:MSVC>:/MP>)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wvarargs)
    endif()
    set_target_properties(${target} PROPERTIES
        COMPILE_WARNING_AS_ERROR ${EXN_WARNINGS_AS_ERRORS}
        FOLDER "eXngine")
endfunction()

# Adds eXngine.<name> as a DLL built from every source under src/<name>/src.
function(exn_add_module name)
    set(source_dir "${PROJECT_SOURCE_DIR}/src/${name}/src")
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${source_dir}/*.c" "${source_dir}/*.cpp")
    add_library(eXngine.${name} SHARED ${sources})
    source_group(TREE "${source_dir}" FILES ${sources})
    target_link_libraries(eXngine.${name} PUBLIC eXngine.config)
    exn_configure_target(eXngine.${name})
endfunction()

# ---------------------------------------------------------------------------
# Engine modules
# ---------------------------------------------------------------------------

exn_add_module(core)

exn_add_module(window)
target_link_libraries(eXngine.window PUBLIC eXngine.core glfw)

exn_add_module(assets)
target_link_libraries(eXngine.assets PUBLIC eXngine.core PRIVATE stb_image stb_truetype)

exn_add_module(renderer)
target_link_libraries(eXngine.renderer PUBLIC eXngine.core eXngine.assets Vulkan::Vulkan)

exn_add_module(physics)
target_link_libraries(eXngine.physics PUBLIC eXngine.core)

# ---------------------------------------------------------------------------
# Example application
# ---------------------------------------------------------------------------

if(EXN_BUILD_EXAMPLE)
    set(EXN_EXAMPLE_DIR "${PROJECT_SOURCE_DIR}/src/example")
    file(GLOB_RECURSE EXN_EXAMPLE_SOURCES CONFIGURE_DEPENDS
        "${EXN_EXAMPLE_DIR}/src/*.cpp"
        "${EXN_EXAMPLE_DIR}/src/*.h"
        "${EXN_EXAMPLE_DIR}/headers/*.h")

    add_executable(eXngine.example WIN32 ${EXN_EXAMPLE_SOURCES})
    source_group(TREE "${EXN_EXAMPLE_DIR}" FILES ${EXN_EXAMPLE_SOURCES})
    target_include_directories(eXngine.example PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${EXN_3RDPARTY_DIR}/imgui")
    target_link_libraries(eXngine.example PRIVATE
        eXngine.renderer
        eXngine.physics
        eXngine.window
        eXngine.assets
        eXngine.core
        glfw
        fbxsdk
        Vulkan::Vulkan)
    exn_configure_target(eXngine.example)

    # imgui.ini and relative asset paths resolve from src/example (same as .vscode/launch.json).
    set_target_properties(eXngine.example PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${EXN_EXAMPLE_DIR}")
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT eXngine.example)

    add_dependencies(eXngine.example eXngine.shaders)

    # Copy imported runtime DLLs (libfbxsdk.dll) next to the executable.
    add_custom_command(TARGET eXngine.example POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_RUNTIME_DLLS:eXngine.example> $<TARGET_FILE_DIR:eXngine.example>
        COMMAND_EXPAND_LISTS)
endif()
