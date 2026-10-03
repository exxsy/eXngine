#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>
#include <unordered_map>
#include <eXngine.h>
#include <world/system.h>
#include <world/world.h>
#include <renderers/vulkan/renderpass.h>
#include <renderers/vulkan/components/text.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;
    class VkMesh;

    // Draws the VkTextComponents of a world. Every text gets a dynamic mesh with one
    // rectangle per glyph, rebuilt only when the text or its layout settings change; moving,
    // scaling or resizing it just changes the draw command's matrix.
    //   renderer->CreateRenderPass("transparent", {.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
    //                                              .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD});
    //   world->AddSystem<VkTextSystem>(renderer, "transparent");
    // Text is alpha blended (EXN_TRANSPARENT_PIPELINE) and writes no depth, so it should be
    // drawn after the objects behind it: in a pass after the scene pass, as above. In the
    // scene pass itself, objects drawn later would cover it.
    class EXNEXPORT VkTextSystem : public World::eXsystem
    {
    public:
        VkTextSystem(Renderer *, const std::string &renderPass = EXN_SCENE_RENDERPASS);

        void OnUpdate(World::eXworld &, EXFLOAT deltaTime) override;
        void OnDetach(World::eXworld &) override;

    private:
        struct DrawnText
        {
            VkMesh *Mesh = nullptr;
            VkDrawCommand *Command = nullptr;
            // The component as it was when the mesh was built, to notice changes.
            VkTextComponent Built;
            EXBOOL HasMesh = false;
        };

        void BuildMesh(DrawnText &, const VkTextComponent &);
        void Release(DrawnText &, VkRenderPassObject *);

        Renderer *m_pRenderer = nullptr;
        // Looked up by name every time: the renderer deletes its passes in OnExit().
        std::string m_RenderPass;
        // By entity id.
        std::unordered_map<EXUINT, DrawnText> m_Texts;
    };
}
#endif
