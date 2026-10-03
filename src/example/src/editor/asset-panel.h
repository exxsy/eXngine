#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include <eXngine.h>
#include <world/world.h>
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/assets.h>

// Combo that picks one of the renderer's materials. noneLabel adds an entry for "no
// material" (null). Returns true when the selection changed.
bool MaterialCombo(const char *label, eXngine::Renderers::Vulkan::Renderer *renderer,
                   eXngine::Renderers::Vulkan::VkMaterial *&material, const char *noneLabel = nullptr);

// Combo that picks one of the models the asset manager knows. Returns true when the selection changed.
bool ModelCombo(const char *label, eXngine::Renderers::Vulkan::VkAssetManager *assets, eXngine::Renderers::Vulkan::VkModel *&model);

// The "Assets" window: the settings of the asset manager, a file browser to load textures
// and models and spawn entities, the loaded assets, materials and scene files.
class AssetPanel
{
public:
    AssetPanel(eXngine::Renderers::Vulkan::Renderer *, eXngine::Renderers::Vulkan::VkAssetManager *, eXngine::World::eXworld *);

    // selectedEntity is set to the entities the panel spawns.
    void Draw(EXUINT &selectedEntity);

    // Called after the panel removed every entity of the world (Scene > Clear).
    std::function<void()> OnWorldCleared;

private:
    struct BrowserEntry
    {
        std::string Name;
        std::string Path;
        bool Directory = false;
    };

    void DrawSettings();
    void DrawBrowser(EXUINT &selectedEntity);
    void DrawLoaded(EXUINT &selectedEntity);
    void DrawMaterials();
    void DrawScene();

    // Load / Spawn buttons for a file, depending on its type.
    void DrawFileActions(const std::string &path, EXUINT &selectedEntity);
    void RefreshBrowser();
    void Spawn(const std::string &path, EXUINT &selectedEntity);

    eXngine::Renderers::Vulkan::Renderer *m_pRenderer;
    eXngine::Renderers::Vulkan::VkAssetManager *m_pAssets;
    eXngine::World::eXworld *m_pWorld;

    char m_RootDirectory[260] = {};
    char m_ManualPath[260] = {};
    char m_Filter[64] = {};
    char m_ScenePath[260] = "assets/scenes/demo.exscene";
    char m_NewMaterialName[64] = "material";
    std::string m_NewMaterialPipeline = EXN_DEFAULT_PIPELINE;
    std::string m_SelectedMaterial;
    std::string m_SceneStatus;

    eXvec3 m_SpawnPosition = eXvec3(0.0f, 0.0f, 0.0f);
    bool m_SelectSpawned = true;

    // The browser lists m_BrowseDirectory, read again every second so new files show up.
    std::string m_BrowseDirectory;
    std::vector<BrowserEntry> m_Entries;
    std::chrono::steady_clock::time_point m_LastRefresh;
};
