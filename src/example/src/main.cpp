#include <filesystem>

#include "../headers/file.h"

#include <eXngine.h>
#include <renderers/vulkan/renderer.h>
#include <windows/eXwindow.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_vulkan.h>

#include <types/color.h>

#include "cameras/ortographic.h"
#include "cameras/perspective.h"

using namespace eXngine;
using namespace eXngine::Windows;
using namespace eXngine::Renderers;
using namespace eXngine::Renderers::Vulkan;

char exePathBuf[MAX_PATH] = {0};
char name[256] = "eXngine Window";
char className[256] = "eXngineWindowClass";
eXviewport viewport;
Size window_size = Size(1024, 768);
Point window_position = Point(100, 100);
PerspectiveCamera camera;
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

void ImGui_OnRender(eXngine::Renderers::eXrenderer *renderer)
{
    eXngine::Renderers::Vulkan::Renderer *vulkan = static_cast<eXngine::Renderers::Vulkan::Renderer *>(renderer);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    ImGui::Begin("eXngine Demo");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    // ImGui::Text("Vulkan average %.3f ms/frame (%.1f FPS)", 1000.0f / VulkanRenderer->GetFPS().m_fFPS, VulkanRenderer->GetFPS().m_fFPS);
    ImGui::Text("Swapchain Extent: %dw %dh", vulkan->m_szSwapChainExtent.width, vulkan->m_szSwapChainExtent.height);
    ImGui::SliderFloat("Rotation Speed", &m_fRotationScale, 0.0f, 50.0f);

    ImGui::BeginGroup();
    ImGui::Text("Camera Position:");
    ImGui::Text("X: %.2f", camera.GetPosition().x);
    ImGui::Text("Y: %.2f", camera.GetPosition().y);
    ImGui::Text("Z: %.2f", camera.GetPosition().z);
    ImGui::Text("FOV: %.2f", camera.GetFieldOfView());
    ImGui::Text("Yaw: %.2f", camera.GetYaw());
    ImGui::Text("Pitch: %.2f", camera.GetPitch());
    ImGui::EndGroup();

    ImGui::End();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    ImDrawData *draw_data = ImGui::GetDrawData();
    const bool isMinimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (isMinimized)
        return;

    ImGui_ImplVulkan_RenderDrawData(draw_data, vulkan->m_pCommandBuffer);
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
    // case WM_MOUSEWHEEL:
    // {
    //     short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    //     if (zDelta > 0)
    //         m_fZoomFactor -= 1.0f;
    //     else
    //         m_fZoomFactor += 1.0f;

    //     m_fZoomFactor = glm::clamp(m_fZoomFactor, 5.0f, 100.0f);
    //     camera.SetFieldOfView(camera.GetFieldOfView() + m_fZoomFactor);
    // }
    // case WM_KEYDOWN:
    //     if (wParam == 'W')
    //         camera.MoveForward(0.016f);
    //     else if (wParam == 'S')
    //         camera.MoveBackward(0.016f);
    //     else if (wParam == 'A')
    //         camera.MoveLeft(0.016f);
    //     else if (wParam == 'D')
    //         camera.MoveRight(0.016f);
    //     break;
    // case WM_MOUSEMOVE:
    //     static int mouseLastX = LOWORD(lParam);
    //     static int mouseLastY = HIWORD(lParam);

    //     camera.Rotate((LOWORD(lParam) - mouseLastX), (HIWORD(lParam) - mouseLastY));    

    //     mouseLastX = LOWORD(lParam);
    //     mouseLastY = HIWORD(lParam);
    //     break;
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

    std::filesystem::path exeDir;
    if (GetModuleFileName(nullptr, exePathBuf, MAX_PATH) != 0)
        exeDir = std::filesystem::path(exePathBuf).remove_filename();
    else
        exeDir = std::filesystem::current_path();

    std::filesystem::path shadersDir = (exeDir / ".." / ".." / "shaders").lexically_normal();

    auto tri_vert = eXngine::Utils::File::Read((shadersDir / "triangle.vert.spv").string());
    auto tri_frag = eXngine::Utils::File::Read((shadersDir / "triangle.frag.spv").string());
    // auto cube_vert = eXngine::Utils::File::Read((shadersDir / "cube.vert.spv").string());
    // auto cube_frag = eXngine::Utils::File::Read((shadersDir / "cube.frag.spv").string());
    auto vert = eXngine::Utils::File::Read((shadersDir / "shader.vert.spv").string());
    auto frag = eXngine::Utils::File::Read((shadersDir / "shader.frag.spv").string());

    viewport.SetWidth(window_size.W);
    viewport.SetHeight(window_size.H);

    camera.SetViewport(&viewport);
    camera.SetPosition(eXvec3(50.0f, 50.0f, 50.0f));
    camera.SetFront(eXvec3(0.0f, 0.0f, -1.0f));
    camera.SetUp(eXvec3(0.0f, 1.0f, 0.0f));

    renderer = new eXngine::Renderers::Vulkan::Renderer(name, window_size);
    app = new eXngine::Windows::eXwindow(name, window_position, window_size, false);
    app->SetClassName(className);
    app->SetInstance(hInstance);
    app->Initialize();

    renderer->AllocatePipeline<eXngine::Renderers::Vulkan::VkGraphicsPipeline>("triangle_pipeline");

    renderer->AddCamera("MainCamera", camera, true);
    renderer->LoadShader("default.vertex", vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("default.fragment", frag, eXngine::eXshader_Fragment);
    renderer->LoadShader("triangle_pipeline.vertex", tri_vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("triangle_pipeline.fragment", tri_frag, eXngine::eXshader_Fragment);
    
    // renderer->DrawLine(eXvec<float, 2>{{0.0f, .2f}}, eXvec<float, 2>{{.3f, .2f}}, eXcolor(255, 0, 0, 255));
    // renderer->DrawTriangle(eXvec<float, 2>{{.3f, .2f}}, eXvec<float, 2>{{-.3f, .2f}}, eXvec<float, 2>{{.3f, .2f}}, eXcolor(0, 255, 0, 255));
    // renderer->DrawRectangle(eXvec<float, 2>{{-.3f, -.5f}}, eXvec<float, 2>{{.3f, -.5f}}, eXvec<float, 2>{{.3f, .5f}}, eXvec<float, 2>{{-.3f, .5f}}, eXcolor(0, 0, 255, 255));

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



    renderer->CreatePipeline<eXngine::Renderers::Vulkan::VkVertex>("triangle_pipeline");

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