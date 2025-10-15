#include "../headers/main.h"

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "libfbxsdk.lib")

const char* m_szName = "eXngine Demo";
const eXngine::Size window_size = eXngine::Size(1024, 768);
float m_fRotationScale = 5.0f;

#ifdef _DEBUG
const std::vector<const char*> debug_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

VkDebugUtilsMessengerEXT debugMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cout << "================" << "\n";
    std::cout << pCallbackData->pMessage << "\n";
    std::cout << "================" << "\n";
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
#else
const std::vector<const char*> debug_extensions = { };
#endif

GLFWApplication* app = new GLFWApplication(m_szName, eXngine::Point(0, 40), window_size, false);
Renderers::Vulkan::Renderer* renderer = new Renderers::Vulkan::Renderer(m_szName, window_size, merge(app->GetExtensions(), debug_extensions));

#ifndef IMGUI_DISABLE
void ImGui_CheckVkResult(VkResult result)
{
    std::cout << result << "\n";
}

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
    //init_info.CheckVkResultFn = ImGui_CheckVkResult;
    ImGui_ImplVulkan_Init(&init_info, renderer->m_pRenderPass);

    {
        const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        renderer->EndSingleTimeCommands(commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void ImGui_OnRender(BaseRenderer* renderer, VkCommandBuffer commandBuffer)
{
	const auto VulkanRenderer = dynamic_cast<Renderers::Vulkan::Renderer*>(renderer);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame(); 

	ImGui::Begin("eXngine Demo");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    //ImGui::Text("Vulkan average %.3f ms/frame (%.1f FPS)", 1000.0f / VulkanRenderer->GetFPS().m_fFPS, VulkanRenderer->GetFPS().m_fFPS);
    ImGui::Text("Swapchain Extent: %dw %dh", VulkanRenderer->m_szSwapChainExtent.width, VulkanRenderer->m_szSwapChainExtent.height);
    ImGui::SliderFloat("Rotation Speed", &m_fRotationScale, 0.0f, 50.0f);
    ImGui::End();

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
    float time = std::chrono::duration<float, std::chrono::minutes::period>(currentTime - startTime).count() * m_fRotationScale;

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(m_fZoomFactor, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)app->GetSize().W / (float)app->GetSize().H, 0.1f, 1000.0f);
	//ubo.view = camera->GetViewMatrix();
	//ubo.proj = camera->GetProjectionMatrix(window_size.W / window_size.H);

    ubo.proj[1][1] *= -1;

    memcpy(buffer, &ubo, sizeof(ubo));
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{

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

    app->SetKeyboardHandler(KeyboardHandler);
    app->SetScrollHandler(ScrollHandler);
    app->SetRenderer(reinterpret_cast<Renderers::BaseRenderer*>(renderer));

    renderer->SetShaders(
        {
            {"main", VK_SHADER_STAGE_VERTEX_BIT, eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.vert.spv")},
            {"main", VK_SHADER_STAGE_FRAGMENT_BIT, eXngine::Utils::File::Read("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\shader.frag.spv")}
        }
    );

    const auto dragonModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\dragon.fbx");
    const auto ballModel = Utils::FbxLoader("C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\models\\model.fbx");

    renderer->LoadModel(
        "dragon", 
        dragonModel.GetMeshes(),
        { 
            { "skin",   "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon.jpg"      },
            { "scales", "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\dragon_skin.jpg" },
        }
    );
    //renderer->LoadModel("ball", ballModel.GetMeshes(), "text2", "C:\\Users\\ex\\Desktop\\GitHub\\eXngine\\output\\assets\\textures\\texture.jpg");
    renderer->SetUpdateUniformBuffersHandler(UpdateUniformBuffer);
    renderer->SetSurface(CreateWindowSurface(renderer, app->GetWindow()));
	renderer->Initialize();

#ifndef IMGUI_DISABLE
    renderer->SetOnRenderHandler(ImGui_OnRender);
    renderer->SetOnCleanupHandler(ImGui_OnExit);

    ImGui_OnInit(app, renderer, window_size);
#endif

    return app->Run();
}