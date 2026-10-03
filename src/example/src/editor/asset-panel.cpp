#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>

#include <imgui.h>

#include <component/transform.h>
#include "asset-panel.h"

using namespace eXngine;
using namespace eXngine::Renderers::Vulkan;

namespace
{
    const char *GetStateName(VkAssetState state)
    {
        switch (state)
        {
        case VkAssetState::Loading:
            return "Loading";
        case VkAssetState::Ready:
            return "Ready";
        default:
            return "Failed";
        }
    }

    ImVec4 GetStateColor(VkAssetState state)
    {
        switch (state)
        {
        case VkAssetState::Loading:
            return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        case VkAssetState::Ready:
            return ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
        default:
            return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        }
    }

    void HelpMarker(const char *text)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");

        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool ContainsIgnoreCase(const std::string &text, const char *filter)
    {
        if (filter[0] == '\0')
            return true;

        const auto lower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            return s;
        };

        return lower(text).find(lower(filter)) != std::string::npos;
    }
}

bool MaterialCombo(const char *label, Renderer *renderer, VkMaterial *&material, const char *noneLabel)
{
    const char *preview = material != nullptr ? material->GetName().c_str() : (noneLabel != nullptr ? noneLabel : "");
    bool changed = false;

    if (ImGui::BeginCombo(label, preview))
    {
        if (noneLabel != nullptr && ImGui::Selectable(noneLabel, material == nullptr))
        {
            material = nullptr;
            changed = true;
        }

        // Sorted by name: the renderer keeps them in a hash map.
        std::vector<VkMaterial *> materials;

        for (const auto &[name, candidate] : renderer->GetMaterials())
            materials.push_back(candidate);

        std::sort(materials.begin(), materials.end(), [](const VkMaterial *a, const VkMaterial *b)
                  { return a->GetName() < b->GetName(); });

        for (VkMaterial *candidate : materials)
        {
            if (ImGui::Selectable(candidate->GetName().c_str(), candidate == material))
            {
                changed = candidate != material;
                material = candidate;
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}

bool ModelCombo(const char *label, VkAssetManager *assets, VkModel *&model)
{
    bool changed = false;

    if (ImGui::BeginCombo(label, model != nullptr ? model->Path.c_str() : "(none)"))
    {
        for (const auto &[path, asset] : assets->GetAssets())
        {
            if (asset->Type == VkAssetType::Model && ImGui::Selectable(path.c_str(), &asset->Model == model))
            {
                changed = &asset->Model != model;
                model = &asset->Model;
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}

AssetPanel::AssetPanel(Renderer *renderer, VkAssetManager *assets, World::eXworld *world)
    : m_pRenderer(renderer), m_pAssets(assets), m_pWorld(world)
{
    snprintf(m_RootDirectory, sizeof(m_RootDirectory), "%s", assets->GetSettings().RootDirectory.c_str());
    m_BrowseDirectory = assets->GetSettings().RootDirectory;
}

void AssetPanel::Draw(EXUINT &selectedEntity)
{
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 430.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 500.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Assets"))
    {
        ImGui::End();
        return;
    }

    const EXSIZE pending = m_pAssets->GetPendingCount();

    if (pending > 0)
        ImGui::TextColored(GetStateColor(VkAssetState::Loading), "Loading %lld file(s)...", pending);
    else
        ImGui::TextDisabled("%zu assets, %zu textures, %zu materials", m_pAssets->GetAssets().size(),
                            m_pRenderer->GetTextures().size(), m_pRenderer->GetMaterials().size());

    if (ImGui::BeginTabBar("##asset_tabs"))
    {
        if (ImGui::BeginTabItem("Browser"))
        {
            DrawBrowser(selectedEntity);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Loaded"))
        {
            DrawLoaded(selectedEntity);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Materials"))
        {
            DrawMaterials();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings"))
        {
            DrawSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scene"))
        {
            DrawScene();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void AssetPanel::DrawSettings()
{
    VkAssetSettings &settings = m_pAssets->GetSettings();

    ImGui::SeparatorText("Loading");

    if (ImGui::InputText("Asset folder", m_RootDirectory, sizeof(m_RootDirectory), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        settings.RootDirectory = m_RootDirectory;
        m_BrowseDirectory = settings.RootDirectory;
        m_LastRefresh = {};
    }
    HelpMarker("Folder listed by the browser, relative to the working directory. Press Enter to apply.");

    ImGui::Checkbox("Load on a worker thread", &settings.AsyncLoading);
    HelpMarker("Files are decoded in the background and uploaded to the GPU between frames, so loading does not freeze the game. Turn it off to load synchronously in the next frame.");

    ImGui::Checkbox("Load model textures", &settings.LoadModelTextures);
    HelpMarker("Also load the diffuse textures that the materials of a model file (MTL, FBX) refer to.");

    if (ImGui::BeginCombo("Material pipeline", settings.MaterialPipeline.c_str()))
    {
        for (const auto &[name, pipeline] : m_pRenderer->m_pGraphicPipelines)
        {
            if (ImGui::Selectable(name.c_str(), name == settings.MaterialPipeline))
                settings.MaterialPipeline = name;
        }

        ImGui::EndCombo();
    }
    HelpMarker("Pipeline (shaders) of the materials created for models.");

    ImGui::SeparatorText("Model import");
    Assets::ModelImportSettings &import = settings.ModelImport;

    ImGui::Checkbox("Z-up to Y-up", &import.SwapYZ);
    ImGui::SameLine();
    ImGui::Checkbox("Flip V", &import.FlipV);
    HelpMarker("OBJ and FBX count texture coordinates from the bottom of the image, the renderer from the top.");

    ImGui::Checkbox("Center pivot", &import.CenterPivot);
    ImGui::SameLine();
    ImGui::Checkbox("Fit to unit size", &import.FitToUnitSize);
    HelpMarker("Scale the model so that its largest side is 1 unit long: files use any unit from millimeters to meters.");

    ImGui::DragFloat("Scale", &import.Scale, 0.01f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::Checkbox("Bake lighting", &import.BakeLighting);
    HelpMarker("The shaders have no lighting: shade the vertex colors with a directional light at import time.");

    ImGui::BeginDisabled(!import.BakeLighting);
    ImGui::DragFloat3("Light direction", &import.LightDirection.x, 0.01f, -1.0f, 1.0f);
    ImGui::SliderFloat("Ambient", &import.Ambient, 0.0f, 1.0f);
    ImGui::EndDisabled();

    if (ImGui::Button("Reload all models"))
    {
        for (const auto &[path, asset] : m_pAssets->GetAssets())
        {
            if (asset->Type == VkAssetType::Model)
                m_pAssets->Reload(path);
        }
    }
    HelpMarker("Import settings apply to models loaded from now on; reloading applies them to the loaded ones.");

    ImGui::SeparatorText("Spawning");
    ImGui::DragFloat3("Spawn position", &m_SpawnPosition.x, 0.01f);
    ImGui::Checkbox("Select spawned entity", &m_SelectSpawned);
}

void AssetPanel::RefreshBrowser()
{
    m_Entries.clear();
    m_LastRefresh = std::chrono::steady_clock::now();

    std::error_code error;

    for (const auto &item : std::filesystem::directory_iterator(m_BrowseDirectory, error))
    {
        BrowserEntry entry;
        entry.Name = item.path().filename().string();
        entry.Path = VkAssetManager::NormalizePath(item.path().string());
        entry.Directory = item.is_directory(error);

        // Only what can be loaded.
        if (entry.Directory || m_pAssets->IsModelFile(entry.Path) || m_pAssets->IsTextureFile(entry.Path) || entry.Path.ends_with(".exscene"))
            m_Entries.push_back(entry);
    }

    std::sort(m_Entries.begin(), m_Entries.end(), [](const BrowserEntry &a, const BrowserEntry &b)
              { return a.Directory != b.Directory ? a.Directory : a.Name < b.Name; });
}

void AssetPanel::DrawBrowser(EXUINT &selectedEntity)
{
    if (std::chrono::steady_clock::now() - m_LastRefresh > std::chrono::seconds(1))
        RefreshBrowser();

    const std::string &root = m_pAssets->GetSettings().RootDirectory;
    const bool atRoot = VkAssetManager::NormalizePath(m_BrowseDirectory) == VkAssetManager::NormalizePath(root);

    ImGui::BeginDisabled(atRoot);
    if (ImGui::ArrowButton("##up", ImGuiDir_Up))
    {
        m_BrowseDirectory = VkAssetManager::NormalizePath(std::filesystem::path(m_BrowseDirectory).parent_path().string());
        m_LastRefresh = {};
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Text("%s/", m_BrowseDirectory.c_str());

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##filter", "Filter", m_Filter, sizeof(m_Filter));

    if (!std::filesystem::is_directory(m_BrowseDirectory))
        ImGui::TextColored(GetStateColor(VkAssetState::Failed), "Folder not found (see Settings > Asset folder).");

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##files", 2, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 3.5f * ImGui::GetFrameHeightWithSpacing())))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableHeadersRow();

        // Copied: entering a folder refreshes m_Entries.
        const std::vector<BrowserEntry> entries = m_Entries;

        for (const auto &entry : entries)
        {
            if (!entry.Directory && !ContainsIgnoreCase(entry.Name, m_Filter))
                continue;

            ImGui::PushID(entry.Path.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (entry.Directory)
            {
                if (ImGui::Selectable((entry.Name + "/").c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_BrowseDirectory = entry.Path;
                    m_LastRefresh = {};
                }
            }
            else
            {
                const VkAsset *asset = m_pAssets->GetAsset(entry.Path);

                if (asset != nullptr)
                    ImGui::TextColored(GetStateColor(asset->State), "%s", entry.Name.c_str());
                else
                    ImGui::TextUnformatted(entry.Name.c_str());

                ImGui::TableNextColumn();
                DrawFileActions(entry.Path, selectedEntity);
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Load from path");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##path", "e.g. C:/models/robot.fbx", m_ManualPath, sizeof(m_ManualPath));
    DrawFileActions(m_ManualPath, selectedEntity);
}

void AssetPanel::DrawFileActions(const std::string &path, EXUINT &selectedEntity)
{
    if (path.empty())
        return;

    if (path.ends_with(".exscene"))
    {
        if (ImGui::SmallButton("Open scene"))
        {
            std::string error;
            m_SceneStatus = m_pAssets->LoadScene(*m_pWorld, path, error) ? "Loaded " + path : "Failed: " + error;
            snprintf(m_ScenePath, sizeof(m_ScenePath), "%s", path.c_str());
        }

        return;
    }

    const bool isModel = m_pAssets->IsModelFile(path);
    const bool isTexture = m_pAssets->IsTextureFile(path);
    const bool loaded = m_pAssets->GetAsset(path) != nullptr;

    if (!isModel && !isTexture)
    {
        ImGui::TextDisabled("unsupported file type");
        return;
    }

    if (loaded)
    {
        if (ImGui::SmallButton("Reload"))
            m_pAssets->Reload(path);
    }
    else if (ImGui::SmallButton("Load"))
    {
        if (isModel)
            m_pAssets->LoadModel(path);
        else
            m_pAssets->LoadTexture(path);
    }

    if (isModel)
    {
        ImGui::SameLine();

        if (ImGui::SmallButton("Spawn"))
            Spawn(path, selectedEntity);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create an entity with this model (loads it first if needed).");
    }
}

void AssetPanel::Spawn(const std::string &path, EXUINT &selectedEntity)
{
    Types::eXtransform transform;
    transform.Position = m_SpawnPosition;

    if (Entity::eXentity *entity = m_pAssets->Instantiate(*m_pWorld, path, transform); entity != nullptr && m_SelectSpawned)
        selectedEntity = entity->GetID();
}

void AssetPanel::DrawLoaded(EXUINT &selectedEntity)
{
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

    if (!ImGui::BeginTable("##loaded", 4, flags))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 55.0f);
    ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 95.0f);
    ImGui::TableHeadersRow();

    for (const auto &[path, asset] : m_pAssets->GetAssets())
    {
        ImGui::PushID(path.c_str());
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(asset->Type == VkAssetType::Model ? "[M]" : "[T]");
        ImGui::SameLine();
        ImGui::TextUnformatted(std::filesystem::path(path).filename().string().c_str());

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", path.c_str());

        ImGui::TableNextColumn();
        ImGui::TextColored(GetStateColor(asset->State), "%s", GetStateName(asset->State));

        if (asset->State == VkAssetState::Failed && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", asset->Error.c_str());

        ImGui::TableNextColumn();

        if (asset->Type == VkAssetType::Texture)
            ImGui::Text("%dx%d", asset->Width, asset->Height);
        else
            ImGui::Text("%lld tris, %zu part(s)", asset->TriangleCount, asset->Model.Parts.size());

        if (ImGui::IsItemHovered())
        {
            if (asset->Type == VkAssetType::Model)
                ImGui::SetTooltip("%lld vertices, %lld triangles\nLoaded in %.1f ms", asset->VertexCount, asset->TriangleCount, asset->LoadMilliseconds);
            else
                ImGui::SetTooltip("Loaded in %.1f ms", asset->LoadMilliseconds);
        }

        ImGui::TableNextColumn();

        if (asset->Type == VkAssetType::Model)
        {
            if (ImGui::SmallButton("+"))
                Spawn(path, selectedEntity);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Spawn an entity with this model");

            ImGui::SameLine();
        }
        else
        {
            ImGui::BeginDisabled(asset->Texture == nullptr);

            if (ImGui::SmallButton("M"))
            {
                if (VkMaterial *material = m_pAssets->CreateTextureMaterial(path))
                    m_SelectedMaterial = material->GetName();
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Create a material that uses this texture");

            ImGui::EndDisabled();
            ImGui::SameLine();
        }

        ImGui::BeginDisabled(asset->BuiltIn);

        if (ImGui::SmallButton("R"))
            m_pAssets->Reload(path);

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(asset->BuiltIn ? "Built-in assets cannot be reloaded" : "Reload with the current settings");

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::SmallButton("X"))
            m_pAssets->Unload(path);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(asset->Type == VkAssetType::Model ? "Unload: entities using it lose their model" : "Unload: materials using it fall back to the default texture");

        ImGui::PopID();
    }

    ImGui::EndTable();
}

void AssetPanel::DrawMaterials()
{
    VkMaterial *selected = m_pRenderer->GetMaterial(m_SelectedMaterial);

    if (MaterialCombo("Material", m_pRenderer, selected))
        m_SelectedMaterial = selected->GetName();

    if (selected != nullptr)
    {
        // Slot 0 is what the default shaders sample.
        VkTexture *current = selected->GetTexture(0);
        std::string currentName = "(default texture)";

        for (const auto &[name, texture] : m_pRenderer->GetTextures())
        {
            if (texture == current)
                currentName = name;
        }

        if (ImGui::BeginCombo("Texture (slot 0)", currentName.c_str()))
        {
            if (ImGui::Selectable("(default texture)", current == nullptr))
                selected->SetTexture(0, static_cast<VkTexture *>(nullptr));

            std::vector<std::string> names;
            for (const auto &[name, texture] : m_pRenderer->GetTextures())
                names.push_back(name);

            std::sort(names.begin(), names.end());

            for (const auto &name : names)
            {
                VkTexture *texture = m_pRenderer->GetTexture(name.c_str());

                if (ImGui::Selectable(name.c_str(), texture == current))
                    selected->SetTexture(0, texture);
            }

            ImGui::EndCombo();
        }
        HelpMarker("Every entity that uses this material changes. Load textures in the Browser tab.");
    }

    ImGui::SeparatorText("New material");
    ImGui::InputText("Name", m_NewMaterialName, sizeof(m_NewMaterialName));

    if (ImGui::BeginCombo("Pipeline", m_NewMaterialPipeline.c_str()))
    {
        for (const auto &[name, pipeline] : m_pRenderer->m_pGraphicPipelines)
        {
            if (ImGui::Selectable(name.c_str(), name == m_NewMaterialPipeline))
                m_NewMaterialPipeline = name;
        }

        ImGui::EndCombo();
    }

    const bool exists = m_pRenderer->GetMaterial(m_NewMaterialName) != nullptr;
    ImGui::BeginDisabled(m_NewMaterialName[0] == '\0' || exists);

    if (ImGui::Button("Create material"))
    {
        if (VkMaterial *material = m_pRenderer->CreateMaterial(m_NewMaterialName, m_NewMaterialPipeline))
            m_SelectedMaterial = material->GetName();
    }

    ImGui::EndDisabled();

    if (exists)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("name is taken");
    }

    ImGui::TextDisabled("%zu / %u materials", m_pRenderer->GetMaterials().size(), m_pRenderer->MAX_MATERIAL_COUNT);
}

void AssetPanel::DrawScene()
{
    ImGui::TextWrapped("A scene file stores the entities of the world: name, transform, model, material override and visibility.");

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##scene", m_ScenePath, sizeof(m_ScenePath));

    if (ImGui::Button("Save"))
    {
        std::error_code ignored;
        std::filesystem::create_directories(std::filesystem::path(m_ScenePath).parent_path(), ignored);

        std::string error;
        m_SceneStatus = m_pAssets->SaveScene(*m_pWorld, m_ScenePath, error) ? std::string("Saved ") + m_ScenePath : "Failed: " + error;
    }

    ImGui::SameLine();

    if (ImGui::Button("Load (add)"))
    {
        std::string error;
        m_SceneStatus = m_pAssets->LoadScene(*m_pWorld, m_ScenePath, error) ? std::string("Loaded ") + m_ScenePath : "Failed: " + error;
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear world"))
        ImGui::OpenPopup("Clear world?");

    if (ImGui::BeginPopupModal("Clear world?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Destroy all %lld entities?", m_pWorld->GetEntityCount());

        if (ImGui::Button("Clear"))
        {
            m_pWorld->Clear();

            if (OnWorldCleared)
                OnWorldCleared();

            m_SceneStatus = "World cleared";
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (!m_SceneStatus.empty())
        ImGui::TextDisabled("%s", m_SceneStatus.c_str());
}
