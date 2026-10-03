#ifndef EXN_DISABLE_VULKAN
#include <component/transform.h>
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/systems/render.h>

namespace eXngine::Renderers::Vulkan
{
    VkRenderSystem::VkRenderSystem(Renderer *renderer, const std::string &renderPass)
        : m_pRenderer(renderer), m_RenderPass(renderPass)
    {
    }

    void VkRenderSystem::OnUpdate(World::eXworld &world, EXFLOAT)
    {
        VkRenderPassObject *pass = m_pRenderer->GetRenderPass(m_RenderPass);

        if (pass == EXN_NULL_HANDLE)
            return;

        std::unordered_map<EXUINT, VkDrawCommand *> drawn;
        drawn.reserve(m_DrawCommands.size());

        for (auto &[id, mesh] : world.GetComponents<VkMeshComponent>())
        {
            if (!mesh.Visible)
                continue;

            // Reuse the entity's draw command from the last frame, or create its first one.
            VkDrawCommand *draw = EXN_NULL_HANDLE;
            const auto it = m_DrawCommands.find(id);

            if (it != m_DrawCommands.end())
            {
                draw = it->second;
                m_DrawCommands.erase(it);
            }
            else
            {
                draw = pass->Draw(mesh.Mesh, mesh.Material);
            }

            const auto *entity = world.GetEntity(id);
            const auto *transform = entity != EXN_NULL_HANDLE ? entity->GetComponent<Component::eXtransformComponent>() : EXN_NULL_HANDLE;

            draw->mesh = mesh.Mesh;
            draw->material = mesh.Material;
            draw->textureIndex = mesh.TextureIndex;
            draw->model = transform != EXN_NULL_HANDLE ? transform->GetMatrix() : EXMAT4(1.0f);

            drawn.emplace(id, draw);
        }

        // What is left belongs to entities that were destroyed, hidden or lost their mesh.
        for (auto &[id, draw] : m_DrawCommands)
            pass->Remove(draw);

        m_DrawCommands = std::move(drawn);
    }

    void VkRenderSystem::OnDetach(World::eXworld &)
    {
        // After Renderer::OnExit the pass, and with it every draw command, is already gone.
        if (VkRenderPassObject *pass = m_pRenderer->GetRenderPass(m_RenderPass))
        {
            for (auto &[id, draw] : m_DrawCommands)
                pass->Remove(draw);
        }

        m_DrawCommands.clear();
    }
}
#endif
