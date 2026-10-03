#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <eXngine.h>
#include <component/component.h>

namespace eXngine::Renderers::Vulkan
{
    class VkMesh;
    class VkMaterial;

    // Makes an entity visible: VkRenderSystem draws Mesh with Material at the entity's
    // eXtransformComponent (or at the origin when it has none).
    //   entity->AddComponent<VkMeshComponent>(renderer->CreateMesh(...), renderer->GetMaterial("checker"));
    struct VkMeshComponent : public Component::eXcomponent
    {
        VkMesh *Mesh = nullptr;
        VkMaterial *Material = nullptr;
        EXUINT32 TextureIndex = 0;
        EXBOOL Visible = true;

        VkMeshComponent() = default;
        VkMeshComponent(VkMesh *mesh, VkMaterial *material) : Mesh(mesh), Material(material) {}
    };
}
#endif
