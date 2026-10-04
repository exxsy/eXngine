# eXngine
Game Engine, for education only.

# What do you need?

## Options

- You can define "UNICODE" pre-processor for WCHAR
- "EXN_DISABLE_VULKAN" disables vulkan from renderer.
- "EXN_IMAGE_STRATEGY_STBI" defines asset manager load strategy as stbi
<!-- - "GLFW_EXPOSE_NATIVE_WIN32" exposes native handle for win32 -->
- "_GLFW_WIN32" Uses glfw as win32

## Compiler

- clang
- MSVC

## SDK 
- Windows SDK (for win builds)
- FBX SDK (preferably 2020.X.X)
- Vulkan SDK (preferable >1.2 (for RTX support))
- GLM SDK (you can tick while installing Vulkan)
- GLFW SDK (for window, [native, sdl will be implemented soon.])
- STB SDK
- ImGui SDK (for debugging)


# 2D and input

- `Renderers::Vulkan::VkCanvas` (`includes/renderers/vulkan/canvas.h`): immediate mode 2D drawing on top of the scene. It draws sprites (parts of textures, rotated, tinted), rectangles, gradients, lines, circles, ellipses, rings, UTF-8 text with `VkFont`, clip rectangles and custom shader quads (`VkCanvasPipeline`). Everything is batched by pipeline, texture, clip and transform and recorded in its own render pass (`canvas.vert` / `canvas.frag`).
- `Input::eXinput` (`includes/input/input.h`): polled keyboard and mouse state of an `eXwindow` (`window->SetInput(&input)`): down / pressed / released, a queue of pressed keys, mouse position, raw mouse delta, wheel, cursor shape and cursor lock.- `eXwindow`: `SetOnResizeHandler`, `GetClientSize`, `SetTitle`, `Close`.
- Using eXngine from another CMake project:
  ```cmake
  set(EXN_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
  set(EXN_OUTPUT_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "" FORCE)
  add_subdirectory(<path to eXngine> eXngine)
  target_link_libraries(game PRIVATE eXngine.renderer eXngine.window eXngine.assets eXngine.core)
  ```
