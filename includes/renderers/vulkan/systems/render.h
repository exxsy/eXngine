#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>
#include <unordered_map>
#include <vector>
#include <eXngine.h>
#include <world/system.h>
#include <world/world.h>
#include <renderers/vulkan/renderpass.h>
#include <renderers/vulkan/components/mesh.h>
#include <renderers/vulkan/components/model.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;

    // Draws the objects of a world: keeps draw commands in a render pass for every visible
    // entity with a VkMeshComponent (one command) or a VkModelComponent (one per part),
    // created, updated and removed together with the entity and its components.
    //   world->AddSystem<VkRenderSystem>(renderer);
    // Add it after the systems that move entities, so a frame shows their latest state.
    // The draw commands of other code in the same pass are left alone.
    class EXNEXPORT VkRenderSystem : public World::eXsystem
    {
    public:
        VkRenderSystem(Renderer *, const std::string &renderPass = EXN_SCENE_RENDERPASS);

        void OnUpdate(World::eXworld &, EXFLOAT deltaTime) override;
        void OnDetach(World::eXworld &) override;

    private:
        Renderer *m_pRenderer = nullptr;
        // Looked up by name every time: the renderer deletes its passes in OnExit().
        std::string m_RenderPass;
        // The draw commands of each drawn entity, by entity id.
        std::unordered_map<EXUINT, std::vector<VkDrawCommand *>> m_DrawCommands;
    };
}
#endif
