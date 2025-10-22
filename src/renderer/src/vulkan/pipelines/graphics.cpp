#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/pipelines/graphics.h>
namespace eXngine::Renderers::Vulkan
{
    void VkGraphicsPipeline::SetExtent(VkExtent2D extent)
    {
		m_pExtent = &extent;

        if (m_pExtent != EXN_NULL_HANDLE && m_Viewports.size() <= 1 && m_Scissors.size() <= 1)
        {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_pExtent->width);
            viewport.height = static_cast<float>(m_pExtent->height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = *m_pExtent;

			if (m_Viewports.size() == 1)
				m_Viewports.clear();

            if (m_Viewports.empty())
                m_Viewports.push_back({
                    .x = 0.0f,            
                    .y = 0.0f,            
                    .width = static_cast<float>(m_pExtent->width),
                    .height = static_cast<float>(m_pExtent->height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                });

            if (m_Scissors.size() == 1)
				m_Scissors.clear();

            if (m_Scissors.empty())
                m_Scissors.push_back({
                    .offset = { 0, 0 },
                    .extent = *m_pExtent,
			    });
        }
    }

    void VkGraphicsPipeline::AddViewport(VkViewport viewport)
    {
		m_Viewports.push_back(viewport);
    }

    void VkGraphicsPipeline::AddScissor(VkRect2D rect)
    {
		m_Scissors.push_back(rect);
    }

    void VkGraphicsPipeline::SetDynamicStates(std::vector<VkDynamicState> s)
    {
		m_DynamicStates = s;
    }

    void VkGraphicsPipeline::CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = MAX_TEXTURE_COUNT;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // to be used in fragment shaders

        const std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<EXUINT32>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        assert(vkCreateDescriptorSetLayout(*m_pDevice, &layoutInfo, nullptr, &m_pDescriptorSetLayout) == VK_SUCCESS);
    }

    void VkGraphicsPipeline::CreateDescriptorSets(VkSampler sampler, VkTexture * texture, VkBuffer buffer)
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, this->m_pDescriptorSetLayout);

        VkDescriptorImageInfo defaultImageInfo {
            .sampler = sampler,
            .imageView = texture->m_pView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = *this->m_pDescriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptorSets;
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        assert(vkAllocateDescriptorSets(*m_pDevice, &allocInfo, descriptorSets.data()) == VK_SUCCESS);

        for (EXUINT32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            std::vector<VkWriteDescriptorSet> descriptorWrites {
                {
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = descriptorSets[i],
                     .dstBinding = 0,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     .pBufferInfo = &bufferInfo
                },
                {
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = descriptorSets[i],
                     .dstBinding = 1,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					 .pImageInfo = &defaultImageInfo,
                }
            };

			m_DescriptorSets = descriptorSets;

            vkUpdateDescriptorSets(*m_pDevice, static_cast<EXUINT32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}
#endif // EXN_DISABLE_VULKAN