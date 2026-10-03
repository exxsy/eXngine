#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <list>
#include <string>
#include <vector>

#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <renderers/defines.h>
#include <types/vector.h>
#include <types/matrix.h>

using namespace eXngine::Types;

#define EXN_SCENE_RENDERPASS "scene"

namespace eXngine::Renderers::Vulkan
{
    class Renderer;
    class VkMaterial;
    class VkMesh;

    struct VkRenderPassDescription
    {
        // LOAD keeps what earlier passes drew (e.g. a UI pass on top of the scene).
        VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        EXBOOL useDepth = true;
        VkClearColorValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
    };

    // One draw call. Returned by VkRenderPassObject::Draw; edit it to change the
    // material, texture slot or transform of an object at runtime.
    struct VkDrawCommand
    {
        VkMesh *mesh = EXN_NULL_HANDLE;
        VkMaterial *material = EXN_NULL_HANDLE;
        EXMAT4 model = EXMAT4(1.0f);
        EXUINT32 textureIndex = 0;
    };

    // A render pass that targets the swap chain. Passes are recorded in creation order
    // every frame; each one draws its own list of (mesh, material) pairs and can switch
    // pipeline and textures between draws.
    class EXNEXPORT VkRenderPassObject
    {
    public:
        VkRenderPassObject(Renderer *, const std::string &, const VkRenderPassDescription &);
        ~VkRenderPassObject();

        void Create();
        void CreateFramebuffers();
        void DestroyFramebuffers();
        void Release();

        void Begin(VkCommandBuffer, EXUINT32 imageIndex);
        void RecordDraws(VkCommandBuffer, EXUINT32 frameIndex);
        void End(VkCommandBuffer);

        VkDrawCommand *Draw(VkMesh *, VkMaterial *, const EXMAT4 &model = EXMAT4(1.0f), EXUINT32 textureIndex = 0);
        void Remove(VkDrawCommand *);
        void ClearDraws();

        VkRenderPass GetHandle() const { return m_pRenderPass; }
        const std::string &GetName() const { return m_Name; }
        const VkRenderPassDescription &GetDescription() const { return m_Description; }

    private:
        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        std::string m_Name;
        VkRenderPassDescription m_Description;
        VkRenderPass m_pRenderPass = EXN_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;
        // std::list keeps the VkDrawCommand pointers handed out by Draw() stable.
        std::list<VkDrawCommand> m_DrawCommands;
    };
}
#endif
