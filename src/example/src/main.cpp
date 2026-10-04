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
#include <renderers/vulkan/components/text.h>
#include <renderers/vulkan/systems/text.h>
#include <renderers/vulkan/assets.h>
#include <assets/atlas.h>
#include <utils/fbx-loader.h>

#include "cameras/ortographic.h"
#include "cameras/perspective.h"
#include "cameras/free-camera.h"
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
float m_fRotationScale = 1.0f;

// Keyboard and mouse of the window; the controller flies the camera with them
// (right mouse button: look, WASD / QE: move, see cameras/free-camera.h).
Input::eXinput input;
FreeCameraController cameraController;

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

// Font & atlas demo: a font baked from a system font, a sprite atlas packed from several
// images, a text in the world and a HUD text showing the frame rate.
VkFont *demoFont = EXN_NULL_HANDLE;
Assets::TextureAtlas spriteAtlas;
EXUINT hudEntity = 0;
EXUINT textCount = 0;
const char *alignNames[] = {"Left", "Center", "Right"};
const char *spaceNames[] = {"World", "Screen"};
// The atlas textures as ImGui images, for the "Font & Atlas" window.
VkDescriptorSet fontAtlasImage = EXN_NULL_HANDLE;
VkDescriptorSet spriteAtlasImage = EXN_NULL_HANDLE;

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

// Starting view: in front of the demo objects, looking down -Z.
void ResetCamera()
{
    camera.SetPosition(eXvec3(0.0f, 0.0f, 2.0f));
    cameraController.SetRotation(-90.0f, 0.0f);
}

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

// A font that comes with Windows, so the demo needs no font file of its own.
std::string FindSystemFont()
{
    char windows[MAX_PATH] = {0};

    if (GetWindowsDirectoryA(windows, MAX_PATH) == 0)
        return {};

    for (const char *file : {"segoeui.ttf", "arial.ttf", "tahoma.ttf"})
    {
        const std::filesystem::path path = std::filesystem::path(windows) / "Fonts" / file;

        if (std::filesystem::exists(path))
            return path.string();
    }

    return {};
}

Entity::eXentity *CreateText(const std::string &name, const std::string &text, const eXvec3 &position)
{
    Entity::eXentity *entity = world->CreateEntity(name);

    entity->AddComponent<Component::eXtransformComponent>()->Position = position;
    entity->AddComponent<VkTextComponent>(demoFont, text);

    return entity;
}

void CreateFontAndAtlasDemo()
{
    // Sprite atlas: four images packed into one texture, drawn with one material.
    const auto dotPixels = MakePatternTexture(32, 4, false, eXcolor(250, 210, 60, 255), eXcolor(200, 60, 90, 255));
    const auto barPixels = MakePatternTexture(48, 6, true, eXcolor(90, 200, 120, 255), eXcolor(30, 60, 40, 255));

    spriteAtlas.AddImage("pattern", dotPixels.data(), 32, 32, 4);
    spriteAtlas.AddImage("bars", barPixels.data(), 48, 48, 4);
    spriteAtlas.AddImageFile("crate", "assets/textures/crate.png");
    spriteAtlas.AddImageFile("uv-grid", "assets/textures/uv-grid.png");

    if (spriteAtlas.Build())
    {
        VkTexture *texture = renderer->CreateTexture("sprite-atlas", spriteAtlas.GetPixels().data(), spriteAtlas.GetWidth(), spriteAtlas.GetHeight());
        VkMaterial *material = renderer->CreateMaterial("sprite-atlas");
        material->SetTexture(0, texture);

        // One sprite per region: the same texture and material, other texture coordinates.
        EXFLOAT x = -0.9f;

        for (const auto &[regionName, region] : spriteAtlas.GetRegions())
        {
            std::vector<eXngine::Utils::Vertex> vertices;
            std::vector<EXUINT32> indices;

            // As many pixels per unit as the image is tall: every sprite is one unit high.
            Assets::BuildSpriteQuad(region, static_cast<EXFLOAT>(region.Height), vertices, indices);

            Entity::eXentity *sprite = world->CreateEntity("Sprite " + regionName);
            auto *transform = sprite->AddComponent<Component::eXtransformComponent>();
            transform->Position = eXvec3(x, -0.75f, 0.0f);
            transform->Scale = eXvec3(0.35f, 0.35f, 0.35f);
            sprite->AddComponent<VkMeshComponent>(renderer->CreateMesh(vertices, indices), material);

            x += 0.6f;
        }
    }

    const std::string fontPath = FindSystemFont();
    demoFont = fontPath.empty() ? EXN_NULL_HANDLE : renderer->LoadFont("default", fontPath, {.PixelHeight = 48.0f});

    if (demoFont == EXN_NULL_HANDLE)
    {
        EX_WARNING("No system font found: the text demo is disabled.");
        return;
    }

    // Draws after the scene pass, so the blended text covers the objects behind it.
    world->AddSystem<VkTextSystem>(renderer, "transparent");

    auto *title = CreateText("Title text", "Merhaba eXngine!\nFont & Atlas: çğıöşü ÇĞİÖŞÜ", eXvec3(0.0f, 0.72f, 0.0f))->GetComponent<VkTextComponent>();
    title->Size = 0.12f;
    title->Align = Assets::TextAlign::Center;
    title->Color = eXvec3(1.0f, 0.85f, 0.3f);

    // HUD: Size and position in pixels; App_OnLoop keeps it in the bottom-right corner.
    auto *hud = CreateText("HUD text", "", eXvec3(0.0f, 0.0f, 0.0f));
    auto *hudText = hud->GetComponent<VkTextComponent>();
    hudText->Space = VkTextSpace::Screen;
    hudText->Size = 22.0f;
    hudText->Align = Assets::TextAlign::Right;
    hudText->Pivot = eXvec2(1.0f, 1.0f);
    hudEntity = hud->GetID();
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

    CreateFontAndAtlasDemo();
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
    // A focused window keeps navigating with the arrow keys, but leaves WASD to the camera;
    // only an active widget (a text field) takes the keyboard away from it.
    io.ConfigNavCaptureKeyboard = false;

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

    // The atlases as ImGui images (a descriptor set each), shown in the "Font & Atlas" window.
    if (demoFont != EXN_NULL_HANDLE)
        fontAtlasImage = ImGui_ImplVulkan_AddTexture(renderer->m_pTextureSampler, demoFont->GetTexture()->m_pView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (VkTexture *texture = renderer->GetTexture("sprite-atlas"))
        spriteAtlasImage = ImGui_ImplVulkan_AddTexture(renderer->m_pTextureSampler, texture->m_pView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    {
        const VkCommandBuffer commandBuffer = renderer->BeginSingleTimeCommands();

        // ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        // renderer->EndSingleTimeCommands(commandBuffer);

        // ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

// An atlas texture, scaled down to the width of the window.
void AtlasImage(VkDescriptorSet image, EXINT32 width, EXINT32 height)
{
    if (image == EXN_NULL_HANDLE || width <= 0 || height <= 0)
        return;

    const float scale = std::min(1.0f, ImGui::GetContentRegionAvail().x / static_cast<float>(width));
    ImGui::Image(reinterpret_cast<ImTextureID>(image), ImVec2(width * scale, height * scale));
}

void FontAndAtlasWindow()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 340.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380.0f, 400.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Font & Atlas"))
    {
        ImGui::End();
        return;
    }

    if (demoFont != EXN_NULL_HANDLE)
    {
        const Assets::Font &font = demoFont->GetFont();
        const Assets::TextureAtlas &atlas = font.GetAtlas();

        ImGui::SeparatorText("Font");
        ImGui::Text("'%s': %.0f px, %d glyphs", demoFont->GetName().c_str(), font.GetSettings().PixelHeight, static_cast<int>(font.GetGlyphs().size()));
        ImGui::Text("Ascent %.1f, descent %.1f, line height %.1f px", font.GetAscent(), font.GetDescent(), font.GetLineHeight());
        ImGui::Text("Atlas %dx%d, %.0f%% used", atlas.GetWidth(), atlas.GetHeight(), atlas.GetOccupancy() * 100.0f);
        AtlasImage(fontAtlasImage, atlas.GetWidth(), atlas.GetHeight());
    }
    else
    {
        ImGui::TextDisabled("No font loaded.");
    }

    ImGui::SeparatorText("Sprite atlas");
    ImGui::Text("%lld images in %dx%d, %.0f%% used", spriteAtlas.GetImageCount(), spriteAtlas.GetWidth(), spriteAtlas.GetHeight(), spriteAtlas.GetOccupancy() * 100.0f);
    AtlasImage(spriteAtlasImage, spriteAtlas.GetWidth(), spriteAtlas.GetHeight());

    if (ImGui::BeginTable("##regions", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Region");
        ImGui::TableSetupColumn("Pixels");
        ImGui::TableSetupColumn("UV");
        ImGui::TableHeadersRow();

        for (const auto &[regionName, region] : spriteAtlas.GetRegions())
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(regionName.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%d,%d %dx%d", region.X, region.Y, region.Width, region.Height);
            ImGui::TableNextColumn();
            ImGui::Text("%.2f,%.2f - %.2f,%.2f", region.U0, region.V0, region.U1, region.V1);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

// Edits the VkTextComponent of the selected entity.
void TextInspector(Renderer *vulkan, Entity::eXentity *selected)
{
    auto *text = selected->GetComponent<VkTextComponent>();

    if (text == EXN_NULL_HANDLE)
    {
        if (demoFont != EXN_NULL_HANDLE && ImGui::Button("Add text"))
            selected->AddComponent<VkTextComponent>(demoFont, "Text");

        return;
    }

    ImGui::SeparatorText("Text");

    // ImGui edits its own copy while the field is active, so a fresh buffer per frame is fine.
    char buffer[1024];
    strncpy_s(buffer, text->Text.c_str(), _TRUNCATE);

    if (ImGui::InputTextMultiline("##text", buffer, sizeof(buffer), ImVec2(-FLT_MIN, 3.5f * ImGui::GetTextLineHeightWithSpacing())))
        text->Text = buffer;

    if (ImGui::BeginCombo("Font", text->Font != EXN_NULL_HANDLE ? text->Font->GetName().c_str() : "(none)"))
    {
        for (const auto &[fontName, font] : vulkan->GetFonts())
        {
            if (ImGui::Selectable(fontName.c_str(), font == text->Font))
                text->Font = font;
        }

        ImGui::EndCombo();
    }

    const bool screen = text->Space == VkTextSpace::Screen;
    int space = static_cast<int>(text->Space);
    int align = static_cast<int>(text->Align);

    ImGui::ColorEdit3("Color", &text->Color.x);
    ImGui::DragFloat(screen ? "Size (px)" : "Size", &text->Size, screen ? 0.5f : 0.005f, 0.0f, 1000.0f);

    if (ImGui::Combo("Align", &align, alignNames, IM_ARRAYSIZE(alignNames)))
        text->Align = static_cast<Assets::TextAlign>(align);

    ImGui::DragFloat("Max width", &text->MaxWidth, screen ? 1.0f : 0.01f, 0.0f, 10000.0f, text->MaxWidth > 0.0f ? "%.2f" : "no wrapping");
    ImGui::DragFloat("Line spacing", &text->LineSpacing, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat2("Pivot", &text->Pivot.x, 0.01f, 0.0f, 1.0f);

    if (ImGui::Combo("Space", &space, spaceNames, IM_ARRAYSIZE(spaceNames)))
        text->Space = static_cast<VkTextSpace>(space);

    ImGui::Checkbox("Visible##text", &text->Visible);
    ImGui::SameLine();

    if (ImGui::Button("Remove text"))
        selected->RemoveComponent<VkTextComponent>();
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

    ImGui::SeparatorText("Camera");
    ImGui::TextDisabled("Hold RMB: look, WASD: move, Q/E: down/up, Shift: fast\nWheel: forward/back (speed while looking), MMB: pan");

    eXvec3 cameraPosition = camera.GetPosition();
    if (ImGui::DragFloat3("Position##camera", &cameraPosition.x, 0.01f))
        camera.SetPosition(cameraPosition);

    float yawPitch[2] = {cameraController.GetYaw(), cameraController.GetPitch()};
    if (ImGui::DragFloat2("Yaw / Pitch", yawPitch, 0.5f))
        cameraController.SetRotation(yawPitch[0], yawPitch[1]);

    float fieldOfView = camera.GetFieldOfView();
    if (ImGui::SliderAngle("Field of view", &fieldOfView, 10.0f, 120.0f))
        camera.SetFieldOfView(fieldOfView);

    float moveSpeed = cameraController.GetMoveSpeed();
    if (ImGui::DragFloat("Move speed", &moveSpeed, 0.05f, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
        cameraController.SetMoveSpeed(moveSpeed);

    float lookSensitivity = cameraController.GetLookSensitivity();
    if (ImGui::DragFloat("Look sensitivity", &lookSensitivity, 0.005f, 0.01f, 1.0f, "%.3f"))
        cameraController.SetLookSensitivity(lookSensitivity);

    bool invertY = cameraController.GetInvertY();
    if (ImGui::Checkbox("Invert Y", &invertY))
        cameraController.SetInvertY(invertY);

    ImGui::SameLine();

    if (ImGui::Button("Reset camera"))
        ResetCamera();

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

    if (demoFont != EXN_NULL_HANDLE)
    {
        ImGui::SameLine();

        if (ImGui::Button("Add text"))
        {
            auto *text = CreateText("Text " + std::to_string(++textCount), "Text", eXvec3(0.0f, 0.0f, 0.1f));
            text->GetComponent<VkTextComponent>()->Size = 0.1f;
            selectedEntity = text->GetID();
        }
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

        TextInspector(vulkan, selected);

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
    FontAndAtlasWindow();

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

#ifndef IMGUI_DISABLE
    // ImGui comes first: the camera only gets the mouse and keyboard ImGui does not want.
    const ImGuiIO &io = ImGui::GetIO();
    const bool wasLooking = cameraController.IsLooking();

    cameraController.Update(input, deltaTime, !io.WantCaptureMouse, !io.WantCaptureKeyboard);

    // Looking around unfocuses the ImGui windows, like a click on the scene: the keys go to
    // the camera then, not to a text field.
    if (!wasLooking && cameraController.IsLooking())
        ImGui::SetWindowFocus(nullptr);
#else
    cameraController.Update(input, deltaTime);
#endif

    // The HUD text changes every frame: VkTextSystem rebuilds its (dynamic) mesh.
    if (Entity::eXentity *hud = world->GetEntity(hudEntity))
    {
        auto *text = hud->GetComponent<VkTextComponent>();
        auto *transform = hud->GetComponent<Component::eXtransformComponent>();

        if (text != EXN_NULL_HANDLE && transform != EXN_NULL_HANDLE)
        {
            static EXFLOAT framesPerSecond = 0.0f;

            if (deltaTime > 0.0f)
                framesPerSecond += (1.0f / deltaTime - framesPerSecond) * 0.05f;

            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%.0f FPS\n%lld entities", framesPerSecond, world->GetEntityCount());
            text->Text = buffer;

            // Bottom-right corner, also after the window was resized.
            const VkExtent2D extent = renderer->m_szSwapChainExtent;
            transform->Position = eXvec3(extent.width - 16.0f, extent.height - 12.0f, 0.0f);
        }
    }

    // Uploads finished loads and unloads between frames, before the systems use the models.
    assets->Update(*world);
    world->Update(deltaTime);
    renderer->OnRender();

    input.EndFrame();
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
    case WM_SIZING:
    case WM_SIZE:
        EXINT width = LOWORD(lParam);
        EXINT height = HIWORD(lParam);
        renderer->SetFrameBufferSize(eXvec2(width, height));
        renderer->SetFrameBufferResized(true);

        // The aspect ratio of the camera follows the window (WM_SIZING's lParam is a RECT *).
        if (uMsg == WM_SIZE && width > 0 && height > 0)
        {
            viewport.SetWidth(width);
            viewport.SetHeight(height);
        }
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

    camera.SetViewport(&viewport);
    camera.SetUp(eXvec3(0.0f, 1.0f, 0.0f));
    camera.SetFieldOfView(EXMATH::radians(60.0f));
    cameraController.SetCamera(&camera);
    ResetCamera();

    renderer = new eXngine::Renderers::Vulkan::Renderer(name, window_size);
    app = new eXngine::Windows::eXwindow(name, window_position, window_size, false);
    app->SetClassName(className);
    app->SetInstance(hInstance);
    app->Initialize();
    app->SetInput(&input);

    // window_size includes the frame; the first WM_SIZE came before WndProc was set.
    const eXvec2 clientSize = app->GetClientSize();
    viewport.SetWidth(static_cast<EXUINT32>(clientSize.x));
    viewport.SetHeight(static_cast<EXUINT32>(clientSize.y));

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

    // Second pass: keeps color and depth of the scene (LOAD) and blends text on top of it.
    renderer->CreateRenderPass("transparent", {.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD});
    // Last pass: keeps the image (LOAD) and draws ImGui on top, without depth.
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