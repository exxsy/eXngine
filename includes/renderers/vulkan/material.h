#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>
#include <vector>

#include <eXngine.h>
#include <vulkan/vulkan_core.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;
    class VkTexture;
    struct VkGraphicsPipeline;

    // A pipeline (shaders + fixed function state) plus the textures it samples.
    // Textures live in descriptor set 1 as `sampler2D textures[MAX_TEXTURE_COUNT]`;
    // empty slots fall back to the renderer's default texture.
    // Created via Renderer::CreateMaterial; owned by the renderer.
    class EXNEXPORT VkMaterial
    {
    public:
        VkMaterial(Renderer *, const std::string &, VkGraphicsPipeline *);
        ~VkMaterial();

        void SetPipeline(VkGraphicsPipeline *);
        void SetTexture(EXUINT32 slot, VkTexture *);
        bool SetTexture(EXUINT32 slot, const char *path);

        VkGraphicsPipeline *GetPipeline() const { return m_pPipeline; }
        VkTexture *GetTexture(EXUINT32 slot) const;
        const std::string &GetName() const { return m_Name; }

        // Binds the material's textures (set 1) for the given frame in flight.
        void Bind(VkCommandBuffer, VkPipelineLayout, EXUINT32 frameIndex);
        void Release();

    private:
        void AllocateDescriptorSets();
        void UpdateDescriptorSet(EXUINT32 frameIndex);

        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        VkGraphicsPipeline *m_pPipeline = EXN_NULL_HANDLE;
        std::string m_Name;
        std::vector<VkTexture *> m_Textures;
        std::vector<VkDescriptorSet> m_DescriptorSets;
        // One bit per frame in flight whose descriptor set must be rewritten before use.
        EXUINT32 m_nDirtyFrames = 0;
    };
}
#endif
