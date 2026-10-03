#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/material.h>
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    VkMaterial::VkMaterial(Renderer *renderer, const std::string &name, VkGraphicsPipeline *pipeline)
        : m_pRenderer(renderer), m_pPipeline(pipeline), m_Name(name), m_Textures(renderer->MAX_TEXTURE_COUNT, EXN_NULL_HANDLE)
    {
    }

    VkMaterial::~VkMaterial()
    {
        Release();
    }

    void VkMaterial::SetPipeline(VkGraphicsPipeline *pipeline)
    {
        m_pPipeline = pipeline;
    }

    void VkMaterial::SetTexture(EXUINT32 slot, VkTexture *texture)
    {
        if (slot >= m_Textures.size())
        {
            EX_WARNING("Material '%s': texture slot %u is out of range (0-%u).", m_Name.c_str(), slot, static_cast<EXUINT32>(m_Textures.size()) - 1);
            return;
        }

        m_Textures[slot] = texture;

        // Sets may still be in use by frames in flight; each one is rewritten the next
        // time it is bound, after its frame's fence has been waited on.
        m_nDirtyFrames = (1u << m_pRenderer->MAX_FRAMES_IN_FLIGHT) - 1;
    }

    bool VkMaterial::SetTexture(EXUINT32 slot, const char *path)
    {
        VkTexture *texture = m_pRenderer->LoadTexture(path);
        SetTexture(slot, texture);

        return texture != EXN_NULL_HANDLE;
    }

    VkTexture *VkMaterial::GetTexture(EXUINT32 slot) const
    {
        return slot < m_Textures.size() ? m_Textures[slot] : EXN_NULL_HANDLE;
    }

    void VkMaterial::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, EXUINT32 frameIndex)
    {
        if (m_DescriptorSets.empty())
            AllocateDescriptorSets();

        if (m_nDirtyFrames & (1u << frameIndex))
        {
            UpdateDescriptorSet(frameIndex);
            m_nDirtyFrames &= ~(1u << frameIndex);
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_DescriptorSets[frameIndex], 0, nullptr);
    }

    void VkMaterial::Release()
    {
        if (!m_DescriptorSets.empty() && m_pRenderer->m_pDevice != EXN_NULL_HANDLE)
            vkFreeDescriptorSets(m_pRenderer->m_pDevice, m_pRenderer->m_pDescriptorPool, static_cast<EXUINT32>(m_DescriptorSets.size()), m_DescriptorSets.data());

        m_DescriptorSets.clear();
    }

    void VkMaterial::AllocateDescriptorSets()
    {
        const EXUINT32 frameCount = m_pRenderer->MAX_FRAMES_IN_FLIGHT;
        const std::vector<VkDescriptorSetLayout> layouts(frameCount, m_pRenderer->m_pMaterialSetLayout);

        const VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_pRenderer->m_pDescriptorPool,
            .descriptorSetCount = frameCount,
            .pSetLayouts = layouts.data(),
        };

        m_DescriptorSets.resize(frameCount);
        const VkResult result = vkAllocateDescriptorSets(m_pRenderer->m_pDevice, &allocInfo, m_DescriptorSets.data());
        EX_FATAL(result == VK_SUCCESS, "Failed to allocate material descriptor sets (is MAX_MATERIAL_COUNT exceeded?).");

        m_nDirtyFrames = (1u << frameCount) - 1;
    }

    void VkMaterial::UpdateDescriptorSet(EXUINT32 frameIndex)
    {
        // Every slot must hold a valid image; empty slots sample the default texture.
        std::vector<VkDescriptorImageInfo> imageInfos(m_Textures.size());

        for (size_t i = 0; i < m_Textures.size(); ++i)
        {
            const VkTexture *texture = m_Textures[i];

            if (texture == EXN_NULL_HANDLE || texture->m_pView == EXN_NULL_HANDLE)
                texture = m_pRenderer->m_DefaultTexture;

            imageInfos[i] = {
                .sampler = m_pRenderer->m_pTextureSampler,
                .imageView = texture->m_pView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
        }

        const VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_DescriptorSets[frameIndex],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<EXUINT32>(imageInfos.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = imageInfos.data(),
        };

        vkUpdateDescriptorSets(m_pRenderer->m_pDevice, 1, &write, 0, nullptr);
    }
}
#endif
