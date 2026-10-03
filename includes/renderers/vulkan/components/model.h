#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>
#include <vector>
#include <eXngine.h>
#include <component/component.h>

namespace eXngine::Renderers::Vulkan
{
    class VkMesh;
    class VkMaterial;

    struct VkModelPart
    {
        VkMesh *Mesh = nullptr;
        VkMaterial *Material = nullptr;
    };

    // A model on the GPU: one (mesh, material) pair per part of the file. Owned by the
    // VkAssetManager, which keeps the address stable for as long as the model is known,
    // even while it is loading (no parts yet) or being reloaded (parts replaced).
    struct VkModel
    {
        // Key of the model in the asset manager: the file path, or e.g. "builtin:quad".
        std::string Path;
        std::vector<VkModelPart> Parts;
    };

    // Makes an entity show a model: VkRenderSystem draws every part at the entity's
    // eXtransformComponent.
    //   entity->AddComponent<VkModelComponent>(assets->LoadModel("assets/models/crate.obj"));
    struct VkModelComponent : public Component::eXcomponent
    {
        VkModel *Model = nullptr;
        // Replaces the material of every part when set.
        VkMaterial *Material = nullptr;
        EXUINT32 TextureIndex = 0;
        EXBOOL Visible = true;

        VkModelComponent() = default;
        VkModelComponent(VkModel *model, VkMaterial *material = nullptr) : Model(model), Material(material) {}
    };
}
#endif
