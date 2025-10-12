#include <iostream>
#include <Windows.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <chrono>

#include <glm/glm.hpp>
#include <renderers/vulkan/renderer.h>
#include <windows/glfw.h>

#include "../headers/file.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <imgui.h>
#include <imconfig.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#pragma comment(lib, "eXngine.window.lib")
#pragma comment(lib, "eXngine.renderer.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "libfbxsdk.lib")

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_VULKAN
#define GLM_CONFIG_CLIP_CONTROL GLM_CLIP_CONTROL_RH_NO

using namespace eXngine;
using namespace eXngine::Applications;
using namespace eXngine::Renderers;
using namespace eXngine::Renderers::Vulkan; 

template <typename T>
std::vector<T> merge(std::vector<T> const &a, std::vector<T> const &b) {
    std::vector<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

struct Camera {
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 front;
    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch) : 
        position(startPosition), up(startUp), yaw(startYaw), 
        pitch(startPitch), front(glm::vec3(0.0f, 0.0f, 0.0f)), 
        movementSpeed(2.5f), mouseSensitivity(0.1f) {

    }
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }
    glm::mat4 GetProjectionMatrix(float aspectRatio) {
        return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	}
};

Camera* camera = new Camera(glm::vec3(28.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f);
float m_fZoomFactor = 0.0f;

#ifdef _DEBUG
VkDebugUtilsMessengerEXT debugMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cout << "================" << std::endl;
	std::cout << pCallbackData->pMessage << std::endl;
    std::cout << "================" << std::endl;
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}
#endif

#ifndef IMGUI_DISABLE
void ImGui_OnInit(GLFWApplication * app, Renderer * renderer, Size sz)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Dimensions
    io.DisplaySize = ImVec2(static_cast<float>(sz.W), static_cast<float>(sz.H));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(app->GetWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer->m_pInstance;
    init_info.PhysicalDevice = renderer->m_pPhysicalDevice;
    init_info.Device = renderer->m_pDevice;

    const auto families = renderer->FindQueueFamiliesWithSurfaces(renderer->m_pSurface,renderer->m_pPhysicalDevice);
    init_info.QueueFamily = families.graphicsFamily.value();

    init_info.Queue = renderer->m_pGraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = renderer->m_pDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info, renderer->m_pRenderPass);

    {
        const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        renderer->EndSingleTimeCommands(commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void ImGui_OnRender(VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //ImGui::ShowDemoWindow();
    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (!is_minimized)
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void ImGui_OnExit()
{
    // Cleanup
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif

VkSurfaceKHR CreateWindowSurface(Renderers::Vulkan::Renderer *renderer, GLFWwindow *window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(renderer->GetVulkanInstance(), window, nullptr, &surface);
    assert(result == VK_SUCCESS);

    return surface;
}

void KeyboardHandler(GLFWwindow *window, int key, int, int, int) 
{ 

}

void ScrollHandler(GLFWwindow* window, double xoffset, double yoffset) 
{
    if (yoffset > 0)
        m_fZoomFactor -= 1.0f;
    else
		m_fZoomFactor += 1.0f;
}

void UpdateUniformBuffer(void* buffer, uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::minutes::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.view = glm::lookAt(glm::vec3(28.f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.proj = glm::perspective(glm::radians(45.0f), 1024.f / 768.f, 0.1f, 100.0f);
	ubo.view = camera->GetViewMatrix();
	ubo.proj = camera->GetProjectionMatrix(1024.f / 768.f);

    ubo.proj[1][1] *= -1;

    memcpy(buffer, &ubo, sizeof(ubo));
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nShowCmd)
{
    const char* m_szName = "eXngine Demo";
    const Size window_size = Size(1024, 768);
    const std::vector<const char*> debug_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    GLFWApplication* app = new GLFWApplication(m_szName, Point(0, 40), window_size, false);
    Renderers::Vulkan::Renderer* renderer = new Renderers::Vulkan::Renderer(m_szName, window_size, merge(app->GetExtensions(), debug_extensions));

    renderer->SetShaders(
        {
            {"main", VK_SHADER_STAGE_VERTEX_BIT, eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.vert.spv")},
            {"main", VK_SHADER_STAGE_FRAGMENT_BIT, eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.frag.spv")}
        }
    );

    const auto dragonModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\dragon.fbx");
    const auto ballModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\model.fbx");

    renderer->LoadModel("dragon", dragonModel.GetMeshes(), "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon.jpg");
    renderer->LoadModel("ball", ballModel.GetMeshes(), "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\texture.jpg");
    renderer->SetUpdateUniformBuffersHandler(UpdateUniformBuffer);
    renderer->SetOnRenderHandler(ImGui_OnRender);
    renderer->SetOnCleanupHandler(ImGui_OnExit);
    renderer->SetSurface(CreateWindowSurface(renderer, app->GetWindow()));

    app->SetKeyboardHandler(KeyboardHandler);
    app->SetScrollHandler(ScrollHandler);
    app->SetRenderer(reinterpret_cast<Renderers::BaseRenderer*>(renderer));

	renderer->Initialize();

#ifdef _DEBUG
    FILE* stream;
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    populateDebugMessengerCreateInfo(debugCreateInfo);
    debugCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(renderer->GetVulkanInstance(), "vkCreateDebugUtilsMessengerEXT");
    func(renderer->GetVulkanInstance(), &debugCreateInfo, nullptr, &debugMessenger);
#endif

#ifndef IMGUI_DISABLE
    ImGui_OnInit(app, renderer, window_size);
#endif

    return app->Run();
}