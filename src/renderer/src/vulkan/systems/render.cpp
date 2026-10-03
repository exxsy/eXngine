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

        std::unordered_map<EXUINT, std::vector<VkDrawCommand *>> drawn;
        drawn.reserve(m_DrawCommands.size());

        // Reuses the entity's draw commands from the last frame, or creates new ones.
        const auto draw = [&](EXUINT id, VkMesh *mesh, VkMaterial *material, EXUINT32 textureIndex)
        {
            if (mesh == EXN_NULL_HANDLE || material == EXN_NULL_HANDLE)
                return;

            VkDrawCommand *command = EXN_NULL_HANDLE;
            const auto it = m_DrawCommands.find(id);

            if (it != m_DrawCommands.end() && !it->second.empty())
            {
                command = it->second.back();
                it->second.pop_back();
            }
            else
            {
                command = pass->Draw(mesh, material);
            }

            const auto *entity = world.GetEntity(id);
            const auto *transform = entity != EXN_NULL_HANDLE ? entity->GetComponent<Component::eXtransformComponent>() : EXN_NULL_HANDLE;

            command->mesh = mesh;
            command->material = material;
            command->textureIndex = textureIndex;
            command->model = transform != EXN_NULL_HANDLE ? transform->GetMatrix() : EXMAT4(1.0f);

            drawn[id].push_back(command);
        };

        for (auto &[id, mesh] : world.GetComponents<VkMeshComponent>())
        {
            if (mesh.Visible)
                draw(id, mesh.Mesh, mesh.Material, mesh.TextureIndex);
        }

        // A model that is still loading has no parts yet and draws nothing.
        for (auto &[id, model] : world.GetComponents<VkModelComponent>())
        {
            if (!model.Visible || model.Model == EXN_NULL_HANDLE)
                continue;

            for (const auto &part : model.Model->Parts)
                draw(id, part.Mesh, model.Material != EXN_NULL_HANDLE ? model.Material : part.Material, model.TextureIndex);
        }

        // What is left belongs to entities that were destroyed, hidden, lost their mesh or
        // now have a model with fewer parts.
        for (auto &[id, commands] : m_DrawCommands)
        {
            for (auto *command : commands)
                pass->Remove(command);
        }

        m_DrawCommands = std::move(drawn);
    }

    void VkRenderSystem::OnDetach(World::eXworld &)
    {
        // After Renderer::OnExit the pass, and with it every draw command, is already gone.
        if (VkRenderPassObject *pass = m_pRenderer->GetRenderPass(m_RenderPass))
        {
            for (auto &[id, commands] : m_DrawCommands)
            {
                for (auto *command : commands)
                    pass->Remove(command);
            }
        }

        m_DrawCommands.clear();
    }
}
#endif
