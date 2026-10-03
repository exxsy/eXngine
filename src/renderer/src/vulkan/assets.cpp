#ifndef EXN_DISABLE_VULKAN
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <texture/image.h>
#include <component/name.h>
#include <component/transform.h>
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/assets.h>

#define EXN_BUILTIN_PREFIX "builtin:"
#define EXN_UNTEXTURED_MATERIAL "asset:untextured"
#define EXN_WHITE_TEXTURE "asset:white"

namespace eXngine::Renderers::Vulkan
{
    namespace
    {
        std::string GetLowerExtension(const std::string &path)
        {
            std::string extension = std::filesystem::path(path).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            return extension;
        }

        // The rest of the line after the keyword, e.g. a name with spaces.
        std::string ReadRest(std::istringstream &line)
        {
            std::string rest;
            std::getline(line >> std::ws, rest);

            if (!rest.empty() && rest.back() == '\r')
                rest.pop_back();

            return rest;
        }
    }

    VkAssetManager::VkAssetManager(Renderer *renderer) : m_pRenderer(renderer)
    {
        m_ModelLoaders[".obj"] = Assets::LoadObjModel;
        m_Worker = std::thread(&VkAssetManager::WorkerLoop, this);
    }

    VkAssetManager::~VkAssetManager()
    {
        {
            std::lock_guard lock(m_Mutex);
            m_bStop = true;
        }

        m_Wake.notify_all();
        m_Worker.join();
    }

    void VkAssetManager::RegisterModelLoader(const std::string &extension, Assets::ModelLoader loader)
    {
        m_ModelLoaders[GetLowerExtension("file" + extension)] = std::move(loader);
    }

    EXBOOL VkAssetManager::IsModelFile(const std::string &path) const
    {
        return m_ModelLoaders.contains(GetLowerExtension(path));
    }

    EXBOOL VkAssetManager::IsTextureFile(const std::string &path) const
    {
        // What stb_image decodes.
        static const char *extensions[] = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".hdr", ".pic", ".ppm", ".pgm"};
        const std::string extension = GetLowerExtension(path);

        return std::any_of(std::begin(extensions), std::end(extensions), [&](const char *e)
                           { return extension == e; });
    }

    std::string VkAssetManager::NormalizePath(const std::string &path)
    {
        if (path.empty() || path.starts_with(EXN_BUILTIN_PREFIX))
            return path;

        return std::filesystem::path(path).lexically_normal().generic_string();
    }

    VkAsset *VkAssetManager::GetAsset(const std::string &path)
    {
        const auto it = m_Assets.find(NormalizePath(path));
        return it != m_Assets.end() ? it->second.get() : EXN_NULL_HANDLE;
    }

    VkAsset *VkAssetManager::LoadTexture(const std::string &path)
    {
        return Request(path, VkAssetType::Texture);
    }

    VkModel *VkAssetManager::LoadModel(const std::string &path)
    {
        VkAsset *asset = Request(path, VkAssetType::Model);
        return asset != EXN_NULL_HANDLE ? &asset->Model : EXN_NULL_HANDLE;
    }

    VkAsset *VkAssetManager::Request(const std::string &path, VkAssetType type)
    {
        const std::string key = NormalizePath(path);

        if (key.empty())
            return EXN_NULL_HANDLE;

        if (VkAsset *asset = GetAsset(key))
        {
            if (asset->Type != type)
            {
                EX_WARNING("Asset '%s' is already loaded as a %s.", key.c_str(), asset->Type == VkAssetType::Model ? "model" : "texture");
                return EXN_NULL_HANDLE;
            }

            return asset;
        }

        auto asset = std::make_unique<VkAsset>();
        asset->Path = key;
        asset->Type = type;

        VkAsset *added = asset.get();
        m_Assets.emplace(key, std::move(asset));

        Enqueue(*added);
        return added;
    }

    void VkAssetManager::Enqueue(VkAsset &asset)
    {
        asset.State = VkAssetState::Loading;
        asset.Error.clear();

        Job job;
        job.Path = asset.Path;
        job.Type = asset.Type;
        job.Generation = ++asset.Generation;
        job.ModelImport = m_Settings.ModelImport;
        job.LoadModelTextures = m_Settings.LoadModelTextures;

        if (asset.Type == VkAssetType::Model)
        {
            const auto loader = m_ModelLoaders.find(GetLowerExtension(asset.Path));

            if (loader != m_ModelLoaders.end())
                job.Loader = loader->second;
        }

        if (!m_Settings.AsyncLoading)
        {
            m_SyncJobs.push_back(std::move(job));
            return;
        }

        {
            std::lock_guard lock(m_Mutex);
            m_Jobs.push_back(std::move(job));
        }

        m_Wake.notify_one();
    }

    VkModel *VkAssetManager::CreateModel(const std::string &name, const std::vector<Utils::Vertex> &vertices, const std::vector<EXUINT32> &indices, VkMaterial *material)
    {
        if (VkAsset *existing = GetAsset(name))
        {
            EX_WARNING("Asset '%s' already exists.", name.c_str());
            return existing->Type == VkAssetType::Model ? &existing->Model : EXN_NULL_HANDLE;
        }

        VkMesh *mesh = m_pRenderer->CreateMesh(vertices, indices);

        if (mesh == EXN_NULL_HANDLE)
            return EXN_NULL_HANDLE;

        auto asset = std::make_unique<VkAsset>();
        asset->Path = NormalizePath(name);
        asset->Type = VkAssetType::Model;
        asset->State = VkAssetState::Ready;
        asset->BuiltIn = true;
        asset->Model.Path = asset->Path;
        asset->Model.Parts.push_back({mesh, material != EXN_NULL_HANDLE ? material : GetUntexturedMaterial()});
        asset->VertexCount = static_cast<EXSIZE>(vertices.size());
        asset->TriangleCount = static_cast<EXSIZE>(indices.size() / 3);

        VkModel *model = &asset->Model;
        m_Assets.emplace(asset->Path, std::move(asset));

        return model;
    }

    void VkAssetManager::Reload(const std::string &path)
    {
        VkAsset *asset = GetAsset(path);

        if (asset != EXN_NULL_HANDLE && !asset->BuiltIn)
            Enqueue(*asset);
    }

    void VkAssetManager::Unload(const std::string &path)
    {
        if (GetAsset(path) != EXN_NULL_HANDLE)
            m_Unloads.push_back(NormalizePath(path));
    }

    EXSIZE VkAssetManager::GetPendingCount() const
    {
        std::lock_guard lock(m_Mutex);
        return static_cast<EXSIZE>(m_Jobs.size() + m_nBusy + m_Results.size() + m_SyncJobs.size());
    }

    VkMaterial *VkAssetManager::CreateTextureMaterial(const std::string &texturePath)
    {
        VkTexture *texture = m_pRenderer->GetTexture(NormalizePath(texturePath).c_str());

        if (texture == EXN_NULL_HANDLE)
            return EXN_NULL_HANDLE;

        const std::string name = std::filesystem::path(texturePath).stem().string();

        if (VkMaterial *material = m_pRenderer->GetMaterial(name))
            return material;

        VkMaterial *material = m_pRenderer->CreateMaterial(name, m_Settings.MaterialPipeline);

        if (material != EXN_NULL_HANDLE)
            material->SetTexture(0, texture);

        return material;
    }

    Entity::eXentity *VkAssetManager::Instantiate(World::eXworld &world, const std::string &path, const Types::eXtransform &transform)
    {
        VkModel *model = LoadModel(path);

        if (model == EXN_NULL_HANDLE)
            return EXN_NULL_HANDLE;

        const std::string name = path.starts_with(EXN_BUILTIN_PREFIX) ? path.substr(sizeof(EXN_BUILTIN_PREFIX) - 1)
                                                                      : std::filesystem::path(path).stem().string();

        Entity::eXentity *entity = world.CreateEntity(name);
        entity->AddComponent<Component::eXtransformComponent>(transform);
        entity->AddComponent<VkModelComponent>(model);

        return entity;
    }

    void VkAssetManager::Update(World::eXworld &world)
    {
        for (const auto &path : m_Unloads)
            UnloadNow(world, path);

        m_Unloads.clear();

        // Swapped out first: a job may queue more work.
        std::vector<Job> syncJobs;
        syncJobs.swap(m_SyncJobs);

        for (const auto &job : syncJobs)
        {
            Result result = Execute(job);
            Apply(result);
        }

        std::vector<Result> results;
        {
            std::lock_guard lock(m_Mutex);
            results.swap(m_Results);
        }

        for (auto &result : results)
            Apply(result);
    }

    VkAssetManager::Result VkAssetManager::Execute(const Job &job)
    {
        const auto start = std::chrono::steady_clock::now();

        Result result;
        result.Request = job;

        if (!std::filesystem::exists(job.Path))
        {
            result.Error = "file not found";
            return result;
        }

        if (job.Type == VkAssetType::Texture)
        {
            result.Success = Images::DecodeImageFile(job.Path.c_str(), result.Image.Pixels, result.Image.Width, result.Image.Height);

            if (!result.Success)
                result.Error = "unsupported or corrupt image";
        }
        else if (!job.Loader)
        {
            result.Error = "no loader for this file type";
        }
        else
        {
            // Loaders parse untrusted files: a malformed one must not take the game down.
            try
            {
                result.Success = job.Loader(job.Path, result.Model, result.Error);
            }
            catch (const std::exception &exception)
            {
                result.Success = false;
                result.Error = exception.what();
            }

            if (result.Success)
            {
                Assets::ProcessModel(result.Model, job.ModelImport);

                for (const auto &part : result.Model.Parts)
                {
                    if (!job.LoadModelTextures || part.DiffuseTexture.empty() || result.Textures.contains(part.DiffuseTexture))
                        continue;

                    DecodedImage image;

                    if (Images::DecodeImageFile(part.DiffuseTexture.c_str(), image.Pixels, image.Width, image.Height))
                        result.Textures.emplace(part.DiffuseTexture, std::move(image));
                    else
                        EX_WARNING("Model '%s': cannot load texture '%s'.", job.Path.c_str(), part.DiffuseTexture.c_str());
                }
            }
        }

        result.Milliseconds = std::chrono::duration<EXFLOAT, std::milli>(std::chrono::steady_clock::now() - start).count();
        return result;
    }

    void VkAssetManager::Apply(Result &result)
    {
        const auto start = std::chrono::steady_clock::now();
        VkAsset *asset = GetAsset(result.Request.Path);

        // Unloaded meanwhile, or reloaded again and this result is outdated.
        if (asset == EXN_NULL_HANDLE || asset->Generation != result.Request.Generation)
            return;

        if (!result.Success)
        {
            // A failed reload keeps what was loaded before.
            asset->State = VkAssetState::Failed;
            asset->Error = result.Error;
            EX_WARNING("Failed to load '%s': %s", asset->Path.c_str(), result.Error.c_str());
            return;
        }

        if (asset->Type == VkAssetType::Texture)
        {
            UploadTexture(*asset, result.Image);
        }
        else
        {
            // The model's textures become texture assets of their own. Loaded ones are kept:
            // reload them on their own to pick up changes.
            for (const auto &[path, image] : result.Textures)
            {
                VkAsset *texture = GetAsset(path);

                if (texture == EXN_NULL_HANDLE)
                {
                    auto added = std::make_unique<VkAsset>();
                    added->Path = path;
                    texture = added.get();
                    m_Assets.emplace(path, std::move(added));
                }
                else if (texture->Type != VkAssetType::Texture || (texture->State == VkAssetState::Ready && texture->Texture != EXN_NULL_HANDLE))
                {
                    continue;
                }

                // A load of the same file that is still running is outdated now.
                ++texture->Generation;
                UploadTexture(*texture, image);
            }

            std::vector<VkModelPart> parts;

            for (const auto &part : result.Model.Parts)
            {
                VkModelPart uploaded;
                uploaded.Mesh = m_pRenderer->CreateMesh(part.Vertices, part.Indices);

                // One material per texture, shared by every model that uses it.
                if (VkTexture *texture = part.DiffuseTexture.empty() ? EXN_NULL_HANDLE : m_pRenderer->GetTexture(part.DiffuseTexture.c_str()))
                {
                    uploaded.Material = m_pRenderer->GetMaterial(part.DiffuseTexture);

                    if (uploaded.Material == EXN_NULL_HANDLE)
                    {
                        uploaded.Material = m_pRenderer->CreateMaterial(part.DiffuseTexture, m_Settings.MaterialPipeline);

                        if (uploaded.Material != EXN_NULL_HANDLE)
                            uploaded.Material->SetTexture(0, texture);
                    }
                }

                if (uploaded.Material == EXN_NULL_HANDLE)
                    uploaded.Material = GetUntexturedMaterial();

                parts.push_back(uploaded);
            }

            for (const auto &part : asset->Model.Parts)
                m_pRenderer->DestroyMesh(part.Mesh);

            asset->Model.Path = asset->Path;
            asset->Model.Parts = std::move(parts);
            asset->VertexCount = result.Model.GetVertexCount();
            asset->TriangleCount = result.Model.GetTriangleCount();
            asset->State = VkAssetState::Ready;
        }

        asset->Error.clear();
        asset->LoadMilliseconds = result.Milliseconds + std::chrono::duration<EXFLOAT, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    void VkAssetManager::UploadTexture(VkAsset &asset, const DecodedImage &image)
    {
        // Reloading: the materials that sample the old texture get the new one.
        std::vector<std::pair<VkMaterial *, EXUINT32>> users;

        if (VkTexture *old = m_pRenderer->GetTexture(asset.Path.c_str()))
        {
            for (const auto &[name, material] : m_pRenderer->GetMaterials())
            {
                for (EXUINT32 slot = 0; slot < m_pRenderer->MAX_TEXTURE_COUNT; ++slot)
                {
                    if (material->GetTexture(slot) == old)
                        users.emplace_back(material, slot);
                }
            }

            m_pRenderer->DestroyTexture(asset.Path.c_str());
        }

        asset.Texture = m_pRenderer->CreateTexture(asset.Path.c_str(), image.Pixels.data(), image.Width, image.Height);
        asset.Width = image.Width;
        asset.Height = image.Height;
        asset.State = asset.Texture != EXN_NULL_HANDLE ? VkAssetState::Ready : VkAssetState::Failed;

        for (const auto &[material, slot] : users)
            material->SetTexture(slot, asset.Texture);
    }

    void VkAssetManager::UnloadNow(World::eXworld &world, const std::string &path)
    {
        const auto it = m_Assets.find(path);

        if (it == m_Assets.end())
            return;

        VkAsset &asset = *it->second;

        if (asset.Type == VkAssetType::Model)
        {
            // Collected first: components must not be removed while iterating over them.
            std::vector<EXUINT> users;

            for (auto &[id, component] : world.GetComponents<VkModelComponent>())
            {
                if (component.Model == &asset.Model)
                    users.push_back(id);
            }

            for (const EXUINT id : users)
            {
                if (Entity::eXentity *entity = world.GetEntity(id))
                    entity->RemoveComponent<VkModelComponent>();
            }

            for (const auto &part : asset.Model.Parts)
                m_pRenderer->DestroyMesh(part.Mesh);
        }
        else
        {
            m_pRenderer->DestroyTexture(asset.Path.c_str());
        }

        m_Assets.erase(it);
    }

    VkMaterial *VkAssetManager::GetUntexturedMaterial()
    {
        if (VkMaterial *material = m_pRenderer->GetMaterial(EXN_UNTEXTURED_MATERIAL))
            return material;

        // The renderer's default texture is a "missing texture" red: untextured models are white.
        VkTexture *white = m_pRenderer->GetTexture(EXN_WHITE_TEXTURE);

        if (white == EXN_NULL_HANDLE)
        {
            const unsigned char pixel[4] = {255, 255, 255, 255};
            white = m_pRenderer->CreateTexture(EXN_WHITE_TEXTURE, pixel, 1, 1);
        }

        VkMaterial *material = m_pRenderer->CreateMaterial(EXN_UNTEXTURED_MATERIAL, m_Settings.MaterialPipeline);

        if (material == EXN_NULL_HANDLE)
            material = m_pRenderer->CreateMaterial(EXN_UNTEXTURED_MATERIAL, EXN_DEFAULT_PIPELINE);

        if (material != EXN_NULL_HANDLE)
            material->SetTexture(0, white);

        return material;
    }

    void VkAssetManager::WorkerLoop()
    {
        for (;;)
        {
            Job job;
            {
                std::unique_lock lock(m_Mutex);
                m_Wake.wait(lock, [this]
                            { return m_bStop || !m_Jobs.empty(); });

                if (m_bStop)
                    return;

                job = std::move(m_Jobs.front());
                m_Jobs.pop_front();
                ++m_nBusy;
            }

            Result result = Execute(job);

            std::lock_guard lock(m_Mutex);
            m_Results.push_back(std::move(result));
            --m_nBusy;
        }
    }

    EXBOOL VkAssetManager::SaveScene(World::eXworld &world, const std::string &path, std::string &error)
    {
        std::ofstream file(path);

        if (!file.is_open())
        {
            error = "cannot write file";
            return false;
        }

        file << "# eXngine scene: one block per entity, rotation in degrees\n";

        for (auto &[id, entity] : world)
        {
            const auto *name = entity.GetComponent<Component::eXnameComponent>();
            file << "\nentity " << (name != EXN_NULL_HANDLE ? name->Name : "Entity") << "\n";

            if (const auto *transform = entity.GetComponent<Component::eXtransformComponent>())
            {
                const EXMATH::vec3 rotation = EXMATH::degrees(EXMATH::vec3(transform->GetEulerAngles()));

                file << "position " << transform->Position.x << " " << transform->Position.y << " " << transform->Position.z << "\n";
                file << "rotation " << rotation.x << " " << rotation.y << " " << rotation.z << "\n";
                file << "scale " << transform->Scale.x << " " << transform->Scale.y << " " << transform->Scale.z << "\n";
            }

            if (const auto *model = entity.GetComponent<VkModelComponent>(); model != EXN_NULL_HANDLE && model->Model != EXN_NULL_HANDLE)
            {
                file << "model " << model->Model->Path << "\n";

                if (model->Material != EXN_NULL_HANDLE)
                    file << "material " << model->Material->GetName() << "\n";

                file << "visible " << (model->Visible ? 1 : 0) << "\n";
            }

            file << "end\n";
        }

        return true;
    }

    EXBOOL VkAssetManager::LoadScene(World::eXworld &world, const std::string &path, std::string &error)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            error = "cannot open file";
            return false;
        }

        struct EntityBlock
        {
            std::string Name = "Entity", Model, Material;
            Component::eXtransformComponent Transform;
            EXMATH::vec3 Rotation = EXMATH::vec3(0.0f);
            EXBOOL HasTransform = false, Visible = true;
        };

        std::optional<EntityBlock> block;
        std::string text;
        EXUINT lineNumber = 0;

        while (std::getline(file, text))
        {
            ++lineNumber;

            std::istringstream line(text);
            std::string keyword;
            line >> keyword;

            if (keyword.empty() || keyword[0] == '#')
                continue;

            if (keyword == "entity")
            {
                block = EntityBlock{};
                block->Name = ReadRest(line);
                continue;
            }

            if (!block.has_value())
            {
                EX_WARNING("Scene '%s', line %u: '%s' outside of an entity block.", path.c_str(), lineNumber, keyword.c_str());
                continue;
            }

            if (keyword == "position")
            {
                line >> block->Transform.Position.x >> block->Transform.Position.y >> block->Transform.Position.z;
                block->HasTransform = true;
            }
            else if (keyword == "rotation")
            {
                line >> block->Rotation.x >> block->Rotation.y >> block->Rotation.z;
                block->HasTransform = true;
            }
            else if (keyword == "scale")
            {
                line >> block->Transform.Scale.x >> block->Transform.Scale.y >> block->Transform.Scale.z;
                block->HasTransform = true;
            }
            else if (keyword == "model")
            {
                block->Model = ReadRest(line);
            }
            else if (keyword == "material")
            {
                block->Material = ReadRest(line);
            }
            else if (keyword == "visible")
            {
                line >> block->Visible;
            }
            else if (keyword == "end")
            {
                Entity::eXentity *entity = world.CreateEntity(block->Name);

                if (block->HasTransform)
                {
                    block->Transform.SetEulerAngles(EXMATH::radians(block->Rotation));
                    entity->AddComponent<Component::eXtransformComponent>(block->Transform);
                }

                if (!block->Model.empty())
                {
                    VkMaterial *material = block->Material.empty() ? EXN_NULL_HANDLE : m_pRenderer->GetMaterial(block->Material);

                    if (!block->Material.empty() && material == EXN_NULL_HANDLE)
                        EX_WARNING("Scene '%s': unknown material '%s'.", path.c_str(), block->Material.c_str());

                    if (VkModel *model = LoadModel(block->Model))
                        entity->AddComponent<VkModelComponent>(model, material)->Visible = block->Visible;
                }

                block.reset();
            }
            else
            {
                EX_WARNING("Scene '%s', line %u: unknown keyword '%s'.", path.c_str(), lineNumber, keyword.c_str());
            }
        }

        return true;
    }
}
#endif
