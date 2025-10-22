#include "../headers/main.h"

const char *m_szName = "eXngine Demo";
const eXngine::Size window_size = eXngine::Size(1024, 768);
float m_fRotationScale = 5.0f;
GLFWApplication *app = nullptr;
Renderer *renderer = nullptr;

struct VkTestVertex : public eXngine::Renderers::Vulkan::VkVertex
{
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        return {};
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        return {};
    }
};

// template void VkGraphicsPipeline::CreatePipeline<VkTestVertex>(VkRenderPass);
// template void Renderer::CreatePipeline<VkTestVertex>(std::string);
// template void Renderer::AllocatePipeline<VkGraphicsPipeline>(std::string);

#ifdef _DEBUG
VkDebugUtilsMessengerEXT debugMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    EX_INFO("Validation Layer: %s", pCallbackData->pMessage);

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
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
void ImGui_CheckVkResult(VkResult result)
{
    std::cout << result << "\n";
}

void ImGui_OnInit(GLFWApplication *app, Renderer *renderer, Size sz)
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
    io.DisplaySize = ImVec2(static_cast<float>(sz.W), static_cast<float>(sz.H));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(app->GetWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer->m_pInstance;
    init_info.PhysicalDevice = renderer->m_pPhysicalDevice;
    init_info.Device = renderer->m_pDevice;

    const auto families = renderer->FindQueueFamiliesWithSurfaces(renderer->m_pSurface, renderer->m_pPhysicalDevice);
    init_info.QueueFamily = families.graphicsFamily.value();

    init_info.Queue = renderer->m_pGraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = renderer->m_pDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = renderer->MAX_FRAMES_IN_FLIGHT;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    // init_info.CheckVkResultFn = ImGui_CheckVkResult;
    ImGui_ImplVulkan_Init(&init_info, renderer->m_pRenderPass);

    {
        const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        renderer->EndSingleTimeCommands(commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void ImGui_OnRender(Renderer *renderer, VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
    // Cleanup
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif

VkSurfaceKHR CreateWindowSurface(Renderers::Vulkan::Renderer *renderer, GLFWwindow *window)
{
    // EX_FATAL(glfwVulkanSupported() == GLFW_TRUE, "GLFW Vulkan not supported on this system.");

    // renderer->AddExtension(VK_KHR_SURFACE_EXTENSION_NAME);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)app->GetInstance();
    createInfo.hinstance = GetModuleHandle(0);

    const VkResult result = vkCreateWin32SurfaceKHR(renderer->GetVulkanInstance(), &createInfo, nullptr, &surface);
    // const VkResult result = glfwCreateWindowSurface(renderer->GetVulkanInstance(), window, nullptr, &surface);

    EX_FATAL(result == VK_SUCCESS, "Failed to create window surface.");

    return surface;
}

void KeyboardHandler(GLFWwindow *window, int key, int, int, int)
{
}

void ScrollHandler(GLFWwindow *window, double xoffset, double yoffset)
{
    if (yoffset > 0)
        m_fZoomFactor -= 1.0f;
    else
        m_fZoomFactor += 1.0f;

    m_fZoomFactor = glm::clamp(m_fZoomFactor, 5.0f, 100.0f);
}

void ResizeHandler(GLFWwindow *window, int width, int height)
{
    if (BaseRenderer *renderer = reinterpret_cast<BaseRenderer *>(glfwGetWindowUserPointer(window)))
    {
        renderer->SetFrameBufferSize(Size(width, height));
        renderer->SetFrameBufferResized(true);
    }
}

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

void App_OnRender(void *unused)
{
    renderer->OnRender();
}

void App_OnCleanup(void *unused)
{
    renderer->OnExit();
}

EXINT32 WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    app = new GLFWApplication(m_szName, eXngine::Point(0, 40), window_size, false);
    renderer = new Renderer(m_szName, window_size);

#ifdef _DEBUG
    FILE *stream;
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);

    renderer->AddValidationLayer("VK_LAYER_KHRONOS_validation");
    renderer->AddValidationLayer("VK_LAYER_LUNARG_monitor");
    renderer->AddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    renderer->AddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    renderer->AddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    renderer->AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    renderer->CreateInstance();

    app->Initialize();
    app->SetKeyboardHandler(KeyboardHandler);
    app->SetScrollHandler(ScrollHandler);
    app->SetFramebufferSizeHandler(ResizeHandler);
    // app->SetRenderer(static_cast<Renderers::AbstractRenderer *>(renderer));

    auto tri_vert = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\triangle.vert.spv");
    auto tri_frag = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\triangle.frag.spv");
    auto cube_vert = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\cube.vert.spv");
    auto cube_frag = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\cube.frag.spv");
    auto vert = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.vert.spv");
    auto frag = eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.frag.spv");

    renderer->AllocatePipeline<VkGraphicsPipeline>("triangle_pipeline");
    renderer->AllocatePipeline<VkGraphicsPipeline>("cube_pipeline");

    renderer->LoadShader("default.vertex", vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("default.fragment", frag, eXngine::eXshader_Fragment);
    renderer->LoadShader("triangle_pipeline.vertex", tri_vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("triangle_pipeline.fragment", tri_frag, eXngine::eXshader_Fragment);
    renderer->LoadShader("cube_pipeline.vertex", cube_vert, eXngine::eXshader_Vertex);
    renderer->LoadShader("cube_pipeline.fragment", cube_frag, eXngine::eXshader_Fragment);

    // const auto dragonModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\dragon.fbx");
    // const auto ballModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\model.fbx");

    // renderer->LoadModel(
    //     "dragon", dragonModel.GetMeshes(),
    //     {
    //         { "skin",   "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon.jpg"      },
    //         //{ "scales", "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon_skin.jpg" },
    //     }
    //);

    // renderer->LoadModel(
    //     "ball", ballModel.GetMeshes(),
    //     {
    //         { "texture", "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\texture.jpg" }
    //     }
    //     //"test_custom_pipeline"
    //);

    renderer->SetUpdateUniformBuffersHandler(UpdateUniformBuffer);
    renderer->SetSurface(CreateWindowSurface(renderer, app->GetWindow()));
    renderer->Initialize();

    renderer->CreatePipeline<VkTestVertex>("triangle_pipeline");
    renderer->CreatePipeline<VkTestVertex>("cube_pipeline");

    app->SetOnRenderHandler(App_OnRender);
    app->SetOnCleanupHandler(App_OnCleanup);

#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    populateDebugMessengerCreateInfo(debugCreateInfo);
    debugCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(renderer->GetVulkanInstance(), "vkCreateDebugUtilsMessengerEXT");
    func(renderer->GetVulkanInstance(), &debugCreateInfo, nullptr, &debugMessenger);
#endif

#ifndef IMGUI_DISABLE
    renderer->SetOnRenderHandler(ImGui_OnRender);
    renderer->SetOnCleanupHandler(ImGui_OnExit);

    ImGui_OnInit(app, renderer, window_size);
#endif

    return app->Run();
}

EXINT32 APIENTRY WinMain(_In_ HINSTANCE hInstance,
                         _In_opt_ HINSTANCE hPrevInstance,
                         _In_ LPSTR /*lpCmdLine*/,
                         _In_ int nShowCmd)
{
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nShowCmd);
}