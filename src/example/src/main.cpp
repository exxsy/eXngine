// #include "../headers/main.h"
// #include <filesystem>

// #ifdef UNICODE
// const EXCHAR m_szName[] = L"eXngine Demo";
// #else
// const EXCHAR m_szName[] = "eXngine Demo";
// #endif

// const eXngine::Size window_size = eXngine::Size(1024, 768);
// float m_fRotationScale = 5.0f;
// GLFWApplication *app = nullptr;
// Renderer *renderer = nullptr;

// struct VkTestVertex : public eXngine::Renderers::Vulkan::VkVertex
// {
//     static VkVertexInputBindingDescription GetBindingDescription()
//     {
//         return {};
//     }

//     static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
//     {
//         return {};
//     }
// };

// #ifndef IMGUI_DISABLE
// void ImGui_CheckVkResult(VkResult result)
// {
//     // std::cout << result << "\n";
//     EX_INFO("ImGui Vulkan result: %d", result);
// }

// void ImGui_OnInit(GLFWApplication *app, Renderer *renderer)
// {
//     // Setup Dear ImGui context
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGuiIO &io = ImGui::GetIO();
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

//     // Setup Dear ImGui style
//     ImGui::StyleColorsDark();
//     // ImGui::StyleColorsLight();

//     // Dimensions
//     io.DisplaySize = ImVec2(static_cast<float>(renderer->m_szSwapChainExtent.width), static_cast<float>(renderer->m_szSwapChainExtent.height));
//     io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

//     // Setup Platform/Renderer backends
//     ImGui_ImplGlfw_InitForVulkan(app->GetWindow(), true);
//     ImGui_ImplVulkan_InitInfo init_info = {};
//     init_info.Instance = renderer->m_pInstance;
//     init_info.PhysicalDevice = renderer->m_pPhysicalDevice;
//     init_info.Device = renderer->m_pDevice;

//     const auto families = renderer->FindQueueFamiliesWithSurfaces(renderer->m_pSurface, renderer->m_pPhysicalDevice);
//     init_info.QueueFamily = families.graphicsFamily.value();

//     init_info.Queue = renderer->m_pGraphicsQueue;
//     init_info.PipelineCache = VK_NULL_HANDLE;
//     init_info.DescriptorPool = renderer->m_pDescriptorPool;
//     init_info.Subpass = 0;
//     init_info.MinImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
//     init_info.ImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
//     init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//     init_info.Allocator = nullptr;
//     // init_info.CheckVkResultFn = ImGui_CheckVkResult;
//     ImGui_ImplVulkan_Init(&init_info, renderer->m_pRenderPass);

//     {
//         const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

//         ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

//         renderer->EndSingleTimeCommands(commandBuffer);

//         ImGui_ImplVulkan_DestroyFontUploadObjects();
//     }
// }

// void ImGui_OnRender(Renderer *renderer, VkCommandBuffer commandBuffer)
// {
//     ImGui_ImplVulkan_NewFrame();
//     ImGui_ImplGlfw_NewFrame();
//     ImGui::NewFrame();

//     ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
//     ImGui::Begin("eXngine Demo");
//     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//     // ImGui::Text("Vulkan average %.3f ms/frame (%.1f FPS)", 1000.0f / VulkanRenderer->GetFPS().m_fFPS, VulkanRenderer->GetFPS().m_fFPS);
//     ImGui::Text("Swapchain Extent: %dw %dh", renderer->m_szSwapChainExtent.width, renderer->m_szSwapChainExtent.height);
//     ImGui::SliderFloat("Rotation Speed", &m_fRotationScale, 0.0f, 50.0f);
//     ImGui::End();

//     ImGui::Render();

//     ImDrawData *draw_data = ImGui::GetDrawData();
//     const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

//     if (!is_minimized)
//         ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
// }

// void ImGui_OnExit()
// {
//     ImGui_ImplVulkan_Shutdown();
//     ImGui_ImplGlfw_Shutdown();
//     ImGui::DestroyContext();
// }
// #endif

// VkSurfaceKHR CreateWindowSurface(HINSTANCE hInstance, Renderers::Vulkan::Renderer *renderer)
// {
//     VkSurfaceKHR surface = VK_NULL_HANDLE;
//     VkWin32SurfaceCreateInfoKHR createInfo{};
//     createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//     createInfo.hwnd = (HWND)app->GetHandle();
//     createInfo.hinstance = hInstance;
//     createInfo.flags = 0;

//     const VkResult result = vkCreateWin32SurfaceKHR(renderer->GetVulkanInstance(), &createInfo, nullptr, &surface);
//     //const VkResult result = glfwCreateWindowSurface(renderer->GetVulkanInstance(), app->GetWindow(), nullptr, &surface);

//     EX_FATAL(result == VK_SUCCESS, "Failed to create window surface.");

//     return surface;
// }

// void KeyboardHandler(GLFWwindow *window, int key, int, int, int)
// {
// }

// void ScrollHandler(GLFWwindow *window, double xoffset, double yoffset)
// {
//     if (yoffset > 0)
//         m_fZoomFactor -= 1.0f;
//     else
//         m_fZoomFactor += 1.0f;

//     m_fZoomFactor = glm::clamp(m_fZoomFactor, 5.0f, 100.0f);
// }

// void ResizeHandler(GLFWwindow *window, int width, int height)
// {
//     if (BaseRenderer *renderer = reinterpret_cast<BaseRenderer *>(glfwGetWindowUserPointer(window)))
//     {
//         renderer->SetFrameBufferSize(Size(width, height));
//         renderer->SetFrameBufferResized(true);
//     }
// }

// void UpdateUniformBuffer(void *buffer, uint32_t currentImage)
// {
//     static auto startTime = std::chrono::high_resolution_clock::now();
//     auto currentTime = std::chrono::high_resolution_clock::now();
//     float time = std::chrono::duration<float, std::chrono::minutes::period>(currentTime - startTime).count() * m_fRotationScale;

//     UniformBufferObject ubo{};
//     ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//     ubo.view = glm::lookAt(glm::vec3(m_fZoomFactor, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//     ubo.proj = glm::perspective(glm::radians(45.0f), (float)window_size.W / (float)window_size.H, 0.1f, 1000.0f);
//     // ubo.view = camera->GetViewMatrix();
//     // ubo.proj = camera->GetProjectionMatrix(window_size.W / window_size.H);

//     ubo.proj[1][1] *= -1;

//     memcpy(buffer, &ubo, sizeof(ubo));
// }

// void App_OnLoop(void *unused)
// {
//     auto io = ImGui::GetIO();
//     auto size = app->GetSize();

//     int w, h;
//     int display_w, display_h;
//     glfwGetWindowSize(app->GetWindow(), &w, &h);
//     glfwGetFramebufferSize(app->GetWindow(), &display_w, &display_h);

//     renderer->OnRender();
// }

// void App_OnCleanup(void *unused)
// {
//     renderer->OnExit();
// }

// EXINT32 WINAPI Window(HINSTANCE hInstance)
// {
//     app = new GLFWApplication(m_szName, eXngine::Point(0, 40), window_size, false);
//     renderer = new Renderer(m_szName, window_size);

// #ifdef _DEBUG
//     FILE *stream;
//     AllocConsole();
//     freopen_s(&stream, "CONOUT$", "w", stdout);

//     renderer->AddValidationLayer("VK_LAYER_KHRONOS_validation");
//     renderer->AddValidationLayer("VK_LAYER_LUNARG_monitor");
//     renderer->AddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//     renderer->CreateDebugPipeline();
// #endif

//     renderer->AddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
//     renderer->AddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
//     renderer->AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
//     renderer->CreateInstance();

//     app->Initialize();
//     app->SetKeyboardHandler(KeyboardHandler);
//     app->SetScrollHandler(ScrollHandler);
//     app->SetFramebufferSizeHandler(ResizeHandler);

//     namespace fs = std::filesystem;

//     char exePathBuf[MAX_PATH] = {0};
//     fs::path exeDir;
//     if (GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH) != 0)
//     {
//         exeDir = fs::path(exePathBuf).remove_filename();
//     }
//     else
//     {
//         exeDir = fs::current_path();
//     }

//     fs::path shadersDir = (exeDir / ".." / ".." / "shaders").lexically_normal();

//     auto tri_vert = eXngine::Utils::File::Read((shadersDir / "triangle.vert.spv").string());
//     auto tri_frag = eXngine::Utils::File::Read((shadersDir / "triangle.frag.spv").string());
//     auto cube_vert = eXngine::Utils::File::Read((shadersDir / "cube.vert.spv").string());
//     auto cube_frag = eXngine::Utils::File::Read((shadersDir / "cube.frag.spv").string());
//     auto vert = eXngine::Utils::File::Read((shadersDir / "shader.vert.spv").string());
//     auto frag = eXngine::Utils::File::Read((shadersDir / "shader.frag.spv").string());

//     renderer->AllocatePipeline<VkGraphicsPipeline>("triangle_pipeline");
//     renderer->AllocatePipeline<VkGraphicsPipeline>("cube_pipeline");

//     renderer->LoadShader("default.vertex", vert, eXngine::eXshader_Vertex);
//     renderer->LoadShader("default.fragment", frag, eXngine::eXshader_Fragment);
//     renderer->LoadShader("triangle_pipeline.vertex", tri_vert, eXngine::eXshader_Vertex);
//     renderer->LoadShader("triangle_pipeline.fragment", tri_frag, eXngine::eXshader_Fragment);
//     renderer->LoadShader("cube_pipeline.vertex", cube_vert, eXngine::eXshader_Vertex);
//     renderer->LoadShader("cube_pipeline.fragment", cube_frag, eXngine::eXshader_Fragment);

//     // const auto dragonModel = Utils::FbxLoader("..\\..\\assets\\models\\dragon.fbx");
//     // const auto ballModel = Utils::FbxLoader("..\\..\\assets\\models\\model.fbx");

//     // renderer->LoadModel(
//     //     "dragon", dragonModel.GetMeshes(),
//     //     {
//     //         { "skin",   "..\\..\\assets\\textures\\dragon.jpg"      },
//     //         //{ "scales", "..\\..\\assets\\textures\\dragon_skin.jpg" },
//     //     }
//     //);

//     // renderer->LoadModel(
//     //     "ball", ballModel.GetMeshes(),
//     //     {
//     //         { "texture", "..\\..\\assets\\textures\\texture.jpg" }
//     //     }
//     //     //"test_custom_pipeline"
//     //);

//     renderer->SetUpdateUniformBuffersHandler(UpdateUniformBuffer);
//     renderer->SetSurface(CreateWindowSurface(hInstance, renderer));
//     renderer->Initialize();

//     renderer->CreatePipeline<VkTestVertex>("triangle_pipeline");
//     renderer->CreatePipeline<VkTestVertex>("cube_pipeline");

// #ifndef IMGUI_DISABLE
//     ImGui_OnInit(app, renderer);

//     renderer->SetOnRenderHandler(ImGui_OnRender);
//     renderer->SetOnCleanupHandler(ImGui_OnExit);
// #endif

//     // app->SetOnInitializeHandler(renderer->Initialize);
//     app->SetOnLoopHandler(App_OnLoop);
//     app->SetOnCleanupHandler(App_OnCleanup);

//     return app->Run();
// }

// EXINT32 WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
// {
//     return Window(hInstance);
// }

// EXINT32 APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
// {
//     return Window(hInstance);
// }

#include <filesystem>

#include "../headers/file.h"

#include <eXngine.h>
#include <renderers/vulkan/renderer.h>
#include <windows/eXwindow.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_vulkan.h>

using namespace eXngine;
using namespace eXngine::Renderers::Vulkan;
using namespace eXngine::Windows;

Size window_size = Size(1024, 768);
Point window_position = Point(100, 100);
Renderer *renderer;
eXwindow *app;
float m_fRotationScale = 1.0f, m_fZoomFactor = 10.0f;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef IMGUI_DISABLE
void ImGui_CheckVkResult(VkResult result)
{
    // std::cout << result << "\n";
    EX_INFO("ImGui Vulkan result: %d", result);
}

void ImGui_OnInit(eXwindow *app, Renderer *renderer)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Dimensions
    io.DisplaySize = ImVec2(static_cast<float>(renderer->m_szSwapChainExtent.width), static_cast<float>(renderer->m_szSwapChainExtent.height));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(app->GetHandle());
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer->m_pInstance;
    init_info.PhysicalDevice = renderer->m_pPhysicalDevice;
    init_info.Device = renderer->m_pDevice;

    const auto families = renderer->FindQueueFamiliesWithSurfaces(renderer->m_pSurface, renderer->m_pPhysicalDevice);
    init_info.QueueFamily = families.graphicsFamily.value();

    init_info.Queue = renderer->m_pGraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = renderer->m_pDescriptorPool;
    init_info.MinImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = renderer->m_pRenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // init_info.CheckVkResultFn = ImGui_CheckVkResult;
    ImGui_ImplVulkan_Init(&init_info);

    {
        const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

        // ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        // renderer->EndSingleTimeCommands(commandBuffer);

        // ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void ImGui_OnRender(Renderer *renderer, VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    ImGui::Begin("eXngine Demo");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    // ImGui::Text("Vulkan average %.3f ms/frame (%.1f FPS)", 1000.0f / VulkanRenderer->GetFPS().m_fFPS, VulkanRenderer->GetFPS().m_fFPS);
    ImGui::Text("Swapchain Extent: %dw %dh", renderer->m_szSwapChainExtent.width, renderer->m_szSwapChainExtent.height);
    ImGui::SliderFloat("Rotation Speed", &m_fRotationScale, 0.0f, 50.0f);
    ImGui::End();

    ImGui::Render();

    ImDrawData *draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (!is_minimized)
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void ImGui_OnExit()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
#endif

// void KeyboardHandler(GLFWwindow *window, int key, int, int, int)
// {
// }

// void ScrollHandler(GLFWwindow *window, double xoffset, double yoffset)
// {
//     if (yoffset > 0)
//         m_fZoomFactor -= 1.0f;
//     else
//         m_fZoomFactor += 1.0f;

//     m_fZoomFactor = glm::clamp(m_fZoomFactor, 5.0f, 100.0f);
// }

// void ResizeHandler(GLFWwindow *window, int width, int height)
// {
//     if (BaseRenderer *renderer = reinterpret_cast<BaseRenderer *>(glfwGetWindowUserPointer(window)))
//     {
//         renderer->SetFrameBufferSize(Size(width, height));
//         renderer->SetFrameBufferResized(true);
//     }
// }

void UpdateUniformBuffer(void *buffer, uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::minutes::period>(currentTime - startTime).count() * m_fRotationScale;

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(m_fZoomFactor, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)window_size.W / (float)window_size.H, 0.1f, 1000.0f);
    // ubo.view = camera->GetViewMatrix();
    // ubo.proj = camera->GetProjectionMatrix(window_size.W / window_size.H);

    ubo.proj[1][1] *= -1;

    memcpy(buffer, &ubo, sizeof(ubo));
}

void App_OnLoop(void *unused)
{
    renderer->OnRender();
}

void App_OnCleanup(void *unused)
{
    renderer->OnExit();
}

LRESULT WndProc(EXWND hwnd, EXUINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_SIZING:
    case WM_SIZE:
        EXINT width = LOWORD(lParam);
        EXINT height = HIWORD(lParam);
        renderer->SetFrameBufferSize(Size(width, height));
        renderer->SetFrameBufferResized(true);
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

EXINT32 APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    FILE *stream;
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);

    char name[256] = "eXngine Window";
    char className[256] = "eXngineWindowClass";

    namespace fs = std::filesystem;

    char exePathBuf[MAX_PATH] = {0};
    fs::path exeDir;
    if (GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH) != 0)
        exeDir = fs::path(exePathBuf).remove_filename();
    else
        exeDir = fs::current_path();

    fs::path shadersDir = (exeDir / ".." / ".." / "shaders").lexically_normal();

    auto tri_vert = eXngine::Utils::File::Read((shadersDir / "triangle.vert.spv").string());
    auto tri_frag = eXngine::Utils::File::Read((shadersDir / "triangle.frag.spv").string());
    // auto cube_vert = eXngine::Utils::File::Read((shadersDir / "cube.vert.spv").string());
    // auto cube_frag = eXngine::Utils::File::Read((shadersDir / "cube.frag.spv").string());
    // auto vert = eXngine::Utils::File::Read((shadersDir / "shader.vert.spv").string());
    // auto frag = eXngine::Utils::File::Read((shadersDir / "shader.frag.spv").string());

    renderer = new eXngine::Renderers::Vulkan::Renderer(name, window_size);
    app = new eXngine::Windows::eXwindow(name, window_position, window_size, false);
    app->SetClassName(className);
    app->SetInstance(hInstance);
    app->Initialize();

    renderer->SetUpdateUniformBuffersHandler(UpdateUniformBuffer);

    // renderer->AllocatePipeline<eXngine::Renderers::Vulkan::VkGraphicsPipeline>("triangle_pipeline");
    renderer->LoadShader("default.vertex", tri_vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("default.fragment", tri_frag, eXngine::eXshader_Fragment);

#ifdef _DEBUG
    renderer->AddValidationLayer("VK_LAYER_KHRONOS_validation");
    renderer->AddValidationLayer("VK_LAYER_LUNARG_monitor");
    renderer->AddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    renderer->AddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    renderer->AddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    renderer->AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    renderer->CreateInstance();
    renderer->CreateDebugPipeline();
    renderer->CreateSurface(app->GetHandle());
    renderer->Initialize();

    // renderer->CreatePipeline<eXngine::Renderers::Vulkan::VkVertex>("triangle_pipeline");

#ifndef IMGUI_DISABLE
    ImGui_OnInit(app, renderer);

    renderer->SetOnRenderHandler(ImGui_OnRender);
    renderer->SetOnCleanupHandler(ImGui_OnExit);
#endif

    app->SetWndProcHandler(WndProc);
    app->SetOnLoopHandler(App_OnLoop);
    app->SetOnCleanupHandler(App_OnCleanup);

    return app->Run();
}