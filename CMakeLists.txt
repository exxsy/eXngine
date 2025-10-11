cmake_minimum_required(VERSION 3.16)

# It's better to configure the compiler and linker outside of the CMakeLists.txt
# for portability, for example using CMake presets, a toolchain file, or your IDE's settings.
# set(CMAKE_LINKER LLD)
# set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")

project(eXngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# # Find packages first
# find_package(Vulkan REQUIRED)
# if (Vulkan_FOUND)
#     message(STATUS "Vulkan found")
# endif()

# # Add GLFW as a subdirectory. This will create the 'glfw' target.
# add_subdirectory(3rdparty/glfw)

# # Define your project's targets
# add_executable(eXngine.example example/src/main.cpp)
# add_library(eXngine.window window/src/window.cpp window/src/glfw.cpp)
# add_library(eXngine.renderer renderer/src/renderer.cpp renderer/src/vulkan.cpp)

# # Set up include directories
# target_include_directories(eXngine.window PUBLIC includes)
# target_include_directories(eXngine.renderer PUBLIC includes)
# target_include_directories(eXngine.renderer PRIVATE ${Vulkan_INCLUDE_DIRS})

# # Link libraries
# target_link_libraries(eXngine.example PRIVATE eXngine.window eXngine.renderer)
# target_link_libraries(eXngine.window PRIVATE glfw) # Link against the glfw target from add_subdirectory
# target_link_libraries(eXngine.renderer PRIVATE ${Vulkan_LIBRARIES})