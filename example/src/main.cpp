#include <iostream>
#include <Windows.h>
#include <stdlib.h>
#include <cassert>

#include <glm/glm.hpp>
#include <renderers/vulkan/renderer.h>
#include <windows/glfw.h>

#include "../headers/file.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#pragma comment(lib, "eXngine.window.lib")
#pragma comment(lib, "eXngine.renderer.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "libfbxsdk.lib")

using namespace eXngine;
using namespace eXngine::Applications;
using namespace eXngine::Renderers;
using namespace eXngine::Renderers::Vulkan; 

const char *m_szName = "eXngine Demo";

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_VULKAN
#define GLM_CONFIG_CLIP_CONTROL GLM_CLIP_CONTROL_RH_NO

#ifdef _DEBUG
VkDebugUtilsMessengerEXT debugMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cout << "================" << std::endl;
	std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
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

    //VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    //populateDebugMessengerCreateInfo(debugCreateInfo);
    //createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
}
#endif

VkSurfaceKHR CreateWindowSurface(Renderers::Vulkan::Renderer *renderer, GLFWwindow *window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(renderer->GetVulkanInstance(), window, nullptr, &surface);
    assert(result == VK_SUCCESS);

    return surface;
}

void KeyboardHandler(GLFWwindow *window, int key, int, int, int) { }

void UpdateUniformBuffer(void* buffer, uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::minutes::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f) /*identity matrix*/,
        time *glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(glm::vec3(28.f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), 1024.f / 768.f, 0.1f, 100.0f);

    ubo.proj[1][1] *= -1;

    memcpy(buffer, &ubo, sizeof(ubo));
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nShowCmd)
{
    const auto vertex_shader = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.vert.spv");
    const auto frag_shader = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.frag.spv");
	const Size window_size = Size(1024, 768);

    GLFWApplication *app = new GLFWApplication(m_szName, Point(0, 40), window_size, false);

    auto extensions = app->GetExtensions();

#ifdef _DEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    Renderers::Vulkan::Renderer* renderer = new Renderers::Vulkan::Renderer(m_szName, window_size, extensions);
    renderer->SetShaders(
        {
            {"main", VK_SHADER_STAGE_VERTEX_BIT, vertex_shader},
            {"main", VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}
        }
    );

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

    renderer->LoadModel("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\model.fbx");
    //renderer->LoadModel("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\dragon.fbx");
    renderer->QueueTexture("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\texture.jpg");
    //renderer->QueueTexture("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon.jpg");
    renderer->SetUpdateUniformBuffersCallback(UpdateUniformBuffer);
    renderer->SetSurface(CreateWindowSurface(renderer, app->GetWindow()));

    app->SetKeyboardHandler(KeyboardHandler);
    app->SetRenderer(reinterpret_cast<Renderers::BaseRenderer*>(renderer));

    return app->Run();
}