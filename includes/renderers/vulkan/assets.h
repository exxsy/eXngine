#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <eXngine.h>
#include <assets/model.h>
#include <types/transform.h>
#include <world/world.h>
#include <renderers/vulkan/components/model.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;
    class VkTexture;
    class VkMaterial;

    enum class VkAssetType
    {
        Texture,
        Model,
    };

    enum class VkAssetState
    {
        Loading, // queued or being decoded
        Ready,
        Failed,
    };

    struct VkAssetSettings
    {
        // The folder tools browse; relative paths resolve from the working directory.
        std::string RootDirectory = "assets";
        // Decode files on a worker thread. The GPU upload always happens in Update().
        EXBOOL AsyncLoading = true;
        // Used by every model (re)load from now on: Reload() applies changes to a loaded model.
        Assets::ModelImportSettings ModelImport;
        // Also load the textures the materials of a model file refer to.
        EXBOOL LoadModelTextures = true;
        // Pipeline of the materials created for models.
        std::string MaterialPipeline = "default";
    };

    // A file known to the asset manager. Owned by the manager: the address, and with it
    // Model, stays valid until the asset is unloaded.
    struct VkAsset
    {
        std::string Path;
        VkAssetType Type = VkAssetType::Texture;
        VkAssetState State = VkAssetState::Loading;
        std::string Error;
        EXFLOAT LoadMilliseconds = 0.0f;
        // Built-in assets (CreateModel) have no file and cannot be reloaded.
        EXBOOL BuiltIn = false;
        // Bumped by every (re)load, so the result of an outdated load is dropped.
        EXUINT32 Generation = 0;

        // Texture: also registered in the renderer under Path (Renderer::GetTexture).
        VkTexture *Texture = EXN_NULL_HANDLE;
        EXINT32 Width = 0, Height = 0;

        // Model
        VkModel Model;
        EXSIZE VertexCount = 0, TriangleCount = 0;
    };

    // Loads textures and models while the game runs and places models in a world.
    //   auto *assets = new VkAssetManager(renderer);
    //   auto *crate = world->CreateEntity("Crate");
    //   crate->AddComponent<VkModelComponent>(assets->LoadModel("assets/models/crate.obj"));
    //   ...
    //   assets->Update(*world); // every frame, before world->Update and renderer->OnRender
    // Load*, Reload and Unload only queue work and can be called from anywhere on the main
    // thread, including the ImGui handler. Files are decoded on a worker thread (see
    // VkAssetSettings::AsyncLoading); Update() uploads the results and does the unloading,
    // between frames, where creating and destroying GPU resources is safe.
    // The GPU resources belong to the renderer and are released by Renderer::OnExit.
    class EXNEXPORT VkAssetManager
    {
    public:
        explicit VkAssetManager(Renderer *);
        ~VkAssetManager();
        VkAssetManager(const VkAssetManager &) = delete;
        VkAssetManager &operator=(const VkAssetManager &) = delete;

        VkAssetSettings &GetSettings() { return m_Settings; }

        // Adds a model format, e.g. RegisterModelLoader(".fbx", LoadFbxModel). The extension
        // is matched case-insensitively. ".obj" is built in.
        void RegisterModelLoader(const std::string &extension, Assets::ModelLoader loader);
        EXBOOL IsModelFile(const std::string &path) const;
        EXBOOL IsTextureFile(const std::string &path) const;

        // Returns the asset at once, still loading. Paths are the keys: loading a known
        // path again returns the existing asset.
        VkAsset *LoadTexture(const std::string &path);
        VkModel *LoadModel(const std::string &path);
        // A model made in code, e.g. CreateModel("builtin:quad", vertices, indices, material).
        // It can be saved in scenes by name like a file.
        VkModel *CreateModel(const std::string &name, const std::vector<Utils::Vertex> &, const std::vector<EXUINT32> &, VkMaterial *);

        // Loads the file again, with the current settings.
        void Reload(const std::string &path);
        // Removes the model from the entities of the world passed to Update and releases
        // its GPU resources. Materials that use an unloaded texture show the default one.
        void Unload(const std::string &path);

        // Null when the path is unknown.
        VkAsset *GetAsset(const std::string &path);
        const std::map<std::string, std::unique_ptr<VkAsset>> &GetAssets() const { return m_Assets; }
        EXSIZE GetPendingCount() const;

        // A material named after the texture's file that samples it in slot 0.
        VkMaterial *CreateTextureMaterial(const std::string &texturePath);

        // A new entity named after the file, at transform, showing the model.
        Entity::eXentity *Instantiate(World::eXworld &, const std::string &path, const Types::eXtransform & = {});

        // Entities as text: name, transform, model path, material override and visibility.
        // Loading adds the entities to the world and queues the models they use.
        EXBOOL SaveScene(World::eXworld &, const std::string &path, std::string &error);
        EXBOOL LoadScene(World::eXworld &, const std::string &path, std::string &error);

        // Uploads what the worker finished and carries out Unload/Reload requests.
        void Update(World::eXworld &);

        // Paths are compared in this form: "assets/models/../a.obj" -> "assets/a.obj".
        static std::string NormalizePath(const std::string &path);

    private:
        struct DecodedImage
        {
            std::vector<EXUINT8> Pixels;
            EXINT32 Width = 0, Height = 0;
        };

        struct Job
        {
            std::string Path;
            VkAssetType Type = VkAssetType::Texture;
            EXUINT32 Generation = 0;
            Assets::ModelLoader Loader;
            Assets::ModelImportSettings ModelImport;
            EXBOOL LoadModelTextures = true;
        };

        struct Result
        {
            Job Request;
            EXBOOL Success = false;
            std::string Error;
            EXFLOAT Milliseconds = 0.0f;
            DecodedImage Image;
            Assets::ModelData Model;
            // The model's textures, by path.
            std::map<std::string, DecodedImage> Textures;
        };

        VkAsset *Request(const std::string &path, VkAssetType type);
        void Enqueue(VkAsset &);
        static Result Execute(const Job &);
        void Apply(Result &);
        void UploadTexture(VkAsset &, const DecodedImage &);
        void UnloadNow(World::eXworld &, const std::string &path);
        VkMaterial *GetUntexturedMaterial();
        void WorkerLoop();

        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        VkAssetSettings m_Settings;
        std::map<std::string, std::unique_ptr<VkAsset>> m_Assets;
        std::unordered_map<std::string, Assets::ModelLoader> m_ModelLoaders;
        // Work for the next Update: loads in sync mode, and unloads.
        std::vector<Job> m_SyncJobs;
        std::vector<std::string> m_Unloads;

        // Shared with the worker thread.
        mutable std::mutex m_Mutex;
        std::condition_variable m_Wake;
        std::deque<Job> m_Jobs;
        std::vector<Result> m_Results;
        EXSIZE m_nBusy = 0;
        EXBOOL m_bStop = false;
        std::thread m_Worker;
    };
}
#endif
