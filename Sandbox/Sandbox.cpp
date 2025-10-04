// Sandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <stdlib.h>
#include <Renderers/VulkanRenderer.h>
#include <cassert>
#include <Applications/glfw.h>
#include "Utils.h"

#pragma comment(lib, "eXngine.Window.lib")
#pragma comment(lib, "eXngine.Renderer.lib")
#pragma comment(lib, "glfw3.lib")

using namespace eXngine;
using namespace eXngine::Renderers;
using namespace eXngine::Applications;

const char *m_szName = "eXngine Demo";

#define WIN32_LEAN_AND_MEAN

VkSurfaceKHR CreateWindowSurface(VulkanRenderer *renderer, GLFWwindow *window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(renderer->GetVulkanInstance(), window, nullptr, &surface);
    assert(result == VK_SUCCESS);

    return surface;
}

void KeyboardHandler(GLFWwindow *window, int key, int, int, int)
{
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nShowCmd)
{
    const auto vertex_shader = Utils::ReadFile("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\vert.spv");
    const auto frag_shader = Utils::ReadFile("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\frag.spv");

    GLFWApplication *app = new GLFWApplication(m_szName, Point(0, 40), Size(1024, 768), false);
    VulkanRenderer *renderer = new VulkanRenderer(m_szName, app->GetExtensions());
    renderer->SetShaders(
        {
            {"main", VK_SHADER_STAGE_VERTEX_BIT, vertex_shader},
            {"main", VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}
        }
    );
    renderer->SetSurface(CreateWindowSurface(renderer, app->GetWindow()));
    renderer->SetFrameBufferSize(app->GetFrameBufferSize());

    app->SetKeyboardHandler(KeyboardHandler);
    app->SetRenderer(reinterpret_cast<Renderer *>(renderer));

#ifdef _IMGUI

#endif

    return app->Run();
}