#include <chrono>
#include <cstdio>
#include <filesystem>
#include <unordered_map>

#include "../headers/file.h"

#include <eXngine.h>
#include <renderers/vulkan/renderer.h>
#include <windows/eXwindow.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_vulkan.h>

#include <types/color.h>
#include <component/name.h>
#include <component/transform.h>
#include <world/world.h>
#include <renderers/vulkan/components/mesh.h>
#include <renderers/vulkan/components/model.h>
#include <renderers/vulkan/systems/render.h>
#include <renderers/vulkan/assets.h>
#include <utils/fbx-loader.h>

#include "cameras/ortographic.h"
#include "cameras/perspective.h"
#include "editor/asset-panel.h"

using namespace eXngine;
using namespace eXngine::Windows;
using namespace eXngine::Renderers;
using namespace eXngine::Renderers::Vulkan;

char exePathBuf[MAX_PATH] = {0};
char name[256] = "eXngine Window";
char className[256] = "eXngineWindowClass";
eXviewport viewport;
eXvec2 window_size = eXvec2(1024, 768);
eXvec2 window_position = eXvec2(100, 100);
PerspectiveCamera camera;
Renderer *renderer;
eXwindow *app;
float m_fRotationScale = 1.0f, m_fZoomFactor = 10.0f;

// Every object of the demo lives in this world; VkRenderSystem draws the ones with a mesh.
World::eXworld *world;
// Loads textures and models at runtime; the "Assets" window shows its settings.
VkAssetManager *assets;
AssetPanel *assetPanel;
VkModel *quadModel;
EXUINT selectedEntity = 0;
EXUINT quadCount = 0;
std::chrono::steady_clock::time_point lastFrameTime;

// Material demo: the 'checker' material can switch between the procedural textures.
const char *textureNames[] = {"checker", "stripes"};
int checkerMaterialTexture = 0;

// Euler angles in degrees per entity, as edited in the UI. Kept here instead of being read
// back from the quaternion, which would flip the values once yaw passes +-90 degrees.
std::unordered_map<EXUINT, eXvec3> editorRotations;

// A game-side component and the system that drives it: entities with a SpinComponent
// rotate around Axis, scaled by the "Rotation Speed" slider.
struct SpinComponent : public Component::eXcomponent
{
    eXvec3 Axis = eXvec3(0.0f, 0.0f, 1.0f);
    EXFLOAT DegreesPerSecond = 90.0f;
};

class SpinSystem : public World::eXsystem
{
public:
    void OnUpdate(World::eXworld &world, EXFLOAT deltaTime) override
    {
        world.Each<SpinComponent, Component::eXtransformComponent>([&](auto &, SpinComponent &spin, Component::eXtransformComponent &transform)
        {
            const EXFLOAT angle = EXMATH::radians(spin.DegreesPerSecond * m_fRotationScale) * deltaTime;
            const EXMATH::quat step = EXMATH::angleAxis(angle, EXMATH::normalize(EXMATH::vec3(spin.Axis)));

            transform.Rotation = EXMATH::normalize(step * EXMATH::quat(transform.Rotation));
        });
    }
};

// Procedural RGBA texture, so the demo does not depend on image files.
std::vector<unsigned char> MakePatternTexture(int size, int cell, bool stripes, eXcolor first, eXcolor second)
{
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const bool isFirst = stripes ? (x / cell) % 2 == 0 : ((x / cell) + (y / cell)) % 2 == 0;
            const eXcolor &color = isFirst ? first : second;
            unsigned char *pixel = &pixels[(static_cast<size_t>(y) * size + x) * 4];

            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = color.a;
        }
    }

    return pixels;
}

// An object of the world: a named entity with a transform and a model to draw. The quad
// is a built-in model of the asset manager, so scene files can refer to it.
Entity::eXentity *CreateQuad(const std::string &name, const eXvec3 &position, const char *material)
{
    Entity::eXentity *quad = world->CreateEntity(name);

    quad->AddComponent<Component::eXtransformComponent>()->Position = position;
    quad->AddComponent<VkModelComponent>(quadModel, renderer->GetMaterial(material));

    return quad;
}

void CreateDemoScene()
{
    const auto checkerPixels = MakePatternTexture(64, 8, false, eXcolor(230, 120, 30, 255), eXcolor(40, 40, 40, 255));
    const auto stripePixels = MakePatternTexture(64, 8, true, eXcolor(40, 160, 220, 255), eXcolor(240, 240, 240, 255));

    renderer->CreateTexture("checker", checkerPixels.data(), 64, 64);
    renderer->CreateTexture("stripes", stripePixels.data(), 64, 64);

    // Same pipeline, different textures...
    renderer->CreateMaterial("checker")->SetTexture(0, renderer->GetTexture("checker"));
    renderer->CreateMaterial("stripes")->SetTexture(0, renderer->GetTexture("stripes"));
    // ...and a different pipeline (shaders).
    renderer->CreateMaterial("triangle", "triangle_pipeline");

    const std::vector<eXngine::Utils::Vertex> vertices{
        {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {-0.5f, -0.5f, 0.0f}, .uv = {0.0f, 1.0f}},
        {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {0.5f, -0.5f, 0.0f}, .uv = {1.0f, 1.0f}},
        {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {0.5f, 0.5f, 0.0f}, .uv = {1.0f, 0.0f}},
        {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {-0.5f, 0.5f, 0.0f}, .uv = {0.0f, 0.0f}},
    };
    const std::vector<EXUINT32> indices{0, 1, 2, 2, 3, 0};

    quadModel = assets->CreateModel("builtin:quad", vertices, indices, renderer->GetMaterial("checker"));

    // Gameplay systems first, so the render system draws their result in the same frame.
    world->AddSystem<SpinSystem>();
    world->AddSystem<VkRenderSystem>(renderer);

    CreateQuad("Left quad", eXvec3(-0.6f, 0.0f, 0.0f), "checker");
    CreateQuad("Right quad", eXvec3(0.6f, 0.0f, 0.0f), "stripes");
}

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
    init_info.PipelineInfoMain.RenderPass = renderer->GetRenderPass("ui")->GetHandle();
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

    ImGui::SeparatorText("Materials");

    if (ImGui::Combo("'checker' texture", &checkerMaterialTexture, textureNames, IM_ARRAYSIZE(textureNames)))
        vulkan->GetMaterial("checker")->SetTexture(0, vulkan->GetTexture(textureNames[checkerMaterialTexture]));

    ImGui::SeparatorText("World");
    ImGui::Text("%lld entities", world->GetEntityCount());

    if (ImGui::Button("Add quad"))
    {
        const std::string name = "Quad " + std::to_string(++quadCount);
        selectedEntity = CreateQuad(name, eXvec3(0.0f, 0.0f, 0.0f), "checker")->GetID();
    }

    Entity::eXentity *selected = world->GetEntity(selectedEntity);

    ImGui::SameLine();
    ImGui::BeginDisabled(selected == EXN_NULL_HANDLE);

    if (ImGui::Button("Destroy"))
    {
        world->DestroyEntity(selectedEntity);
        editorRotations.erase(selectedEntity);
        selected = EXN_NULL_HANDLE;
    }

    ImGui::EndDisabled();

    // Outliner: every object of the world.
    if (ImGui::BeginListBox("##entities", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
    {
        for (auto &[id, entity] : *world)
        {
            const auto *name = entity.GetComponent<Component::eXnameComponent>();
            char label[128];
            snprintf(label, sizeof(label), "%s##%u", name != EXN_NULL_HANDLE ? name->Name.c_str() : "Entity", id);

            if (ImGui::Selectable(label, id == selectedEntity))
                selectedEntity = id;
        }

        ImGui::EndListBox();
    }

    // Inspector: the components of the selected entity.
    if (selected != EXN_NULL_HANDLE)
    {
        const auto *name = selected->GetComponent<Component::eXnameComponent>();
        ImGui::Text("%s (entity %u)", name != EXN_NULL_HANDLE ? name->Name.c_str() : "Entity", selected->GetID());

        auto *spin = selected->GetComponent<SpinComponent>();

        if (auto *transform = selected->GetComponent<Component::eXtransformComponent>())
        {
            ImGui::DragFloat3("Position", &transform->Position.x, 0.01f);

            eXvec3 &rotation = editorRotations.try_emplace(selected->GetID(), EXMATH::degrees(EXMATH::vec3(transform->GetEulerAngles()))).first->second;

            // The spin system owns the rotation while it runs: show it, continue from it later.
            if (spin != EXN_NULL_HANDLE)
                rotation = EXMATH::degrees(EXMATH::vec3(transform->GetEulerAngles()));

            ImGui::BeginDisabled(spin != EXN_NULL_HANDLE);

            if (ImGui::DragFloat3("Rotation", &rotation.x, 1.0f))
                transform->SetEulerAngles(EXMATH::radians(EXMATH::vec3(rotation)));

            ImGui::EndDisabled();
            ImGui::DragFloat3("Scale", &transform->Scale.x, 0.01f);
        }

        if (auto *mesh = selected->GetComponent<VkMeshComponent>())
        {
            MaterialCombo("Material", vulkan, mesh->Material);
            ImGui::Checkbox("Visible", &mesh->Visible);
        }

        if (auto *model = selected->GetComponent<VkModelComponent>())
        {
            ModelCombo("Model", assets, model->Model);

            if (const VkAsset *asset = model->Model != EXN_NULL_HANDLE ? assets->GetAsset(model->Model->Path) : EXN_NULL_HANDLE; asset != EXN_NULL_HANDLE && asset->State != VkAssetState::Ready)
                ImGui::TextDisabled(asset->State == VkAssetState::Loading ? "(loading...)" : "(failed: %s)", asset->Error.c_str());

            MaterialCombo("Material", vulkan, model->Material, "(model materials)");
            ImGui::Checkbox("Visible", &model->Visible);
        }
        else if (ImGui::Button("Add model"))
        {
            selected->AddComponent<VkModelComponent>(quadModel);
        }

        bool spinning = spin != EXN_NULL_HANDLE;

        if (ImGui::Checkbox("Spin", &spinning))
        {
            if (spinning)
                selected->AddComponent<SpinComponent>();
            else
                selected->RemoveComponent<SpinComponent>();
        }
        else if (spin != EXN_NULL_HANDLE)
        {
            ImGui::DragFloat("Degrees per second", &spin->DegreesPerSecond, 1.0f);
        }
    }

    ImGui::End();

    assetPanel->Draw(selectedEntity);

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

void OnRender(eXngine::Renderers::eXrenderer *renderer)
{
    

#ifndef IMGUI_DISABLE
    ImGui_OnRender(renderer);
#endif
}

void App_OnLoop(void *unused)
{
    const auto now = std::chrono::steady_clock::now();
    const EXFLOAT deltaTime = std::chrono::duration<EXFLOAT>(now - lastFrameTime).count();
    lastFrameTime = now;

    // Uploads finished loads and unloads between frames, before the systems use the models.
    assets->Update(*world);
    world->Update(deltaTime);
    renderer->OnRender();
}

void App_OnCleanup(void *unused)
{
    // The world goes first: its render system removes its draw commands from the passes
    // that renderer->OnExit() deletes.
    delete world;
    world = EXN_NULL_HANDLE;

    // Stops the loader thread; the GPU resources it created go with renderer->OnExit().
    delete assetPanel;
    delete assets;
    assetPanel = EXN_NULL_HANDLE;
    assets = EXN_NULL_HANDLE;

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
        renderer->SetFrameBufferSize(eXvec2(width, height));
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

    viewport.SetWidth(window_size.x);
    viewport.SetHeight(window_size.y);

    camera.SetViewport(&viewport);
    camera.SetPosition(eXvec3(0.0f, 0.0f, 2.0f));
    camera.SetFront(eXvec3(0.0f, 0.0f, -1.0f));
    camera.SetUp(eXvec3(0.0f, 1.0f, 0.0f));
    camera.SetFieldOfView(EXMATH::radians(60.0f));

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

    // Second pass: keeps the scene (LOAD) and draws ImGui on top, without depth.
    renderer->CreateRenderPass("ui", {.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .useDepth = false});

    world = new World::eXworld();

    assets = new VkAssetManager(renderer);
    assets->RegisterModelLoader(".fbx", eXngine::Utils::LoadFbxModel);

    CreateDemoScene();

    assetPanel = new AssetPanel(renderer, assets, world);
    assetPanel->OnWorldCleared = []
    {
        editorRotations.clear();
        selectedEntity = 0;
    };

#ifndef IMGUI_DISABLE
    ImGui_OnInit(app, renderer);

    renderer->SetOnRenderHandler(OnRender);
    renderer->SetOnCleanupHandler(ImGui_OnExit);
#endif

    app->SetWndProcHandler(WndProc);
    app->SetOnLoopHandler(App_OnLoop);
    app->SetOnCleanupHandler(App_OnCleanup);

    lastFrameTime = std::chrono::steady_clock::now();
    return app->Run();
}