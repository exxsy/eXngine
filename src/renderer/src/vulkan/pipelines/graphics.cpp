#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/pipelines/graphics.h>

namespace eXngine::Renderers::Vulkan
{
	VkGraphicsPipeline::VkGraphicsPipeline(VkDevice *device, VkDescriptorPool *pool, VkExtent2D *extent) 
	{
		m_pDevice = device;
		m_pDescriptorPool = pool;
		m_pExtent = extent;
		m_ShaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
	};



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

    VkPipelineInputAssemblyStateCreateInfo VkGraphicsPipeline::GetInputAssemblyInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
    }

    VkPipelineDynamicStateCreateInfo VkGraphicsPipeline::GetDynamicStateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<EXUINT32>(m_DynamicStates.size()),
            .pDynamicStates = m_DynamicStates.data(),
        };
    }

    VkPipelineViewportStateCreateInfo VkGraphicsPipeline::GetViewportStateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = static_cast<EXUINT32>(m_Viewports.size()),
            .pViewports = m_Viewports.data(),
            .scissorCount = static_cast<EXUINT32>(m_Scissors.size()),
            .pScissors = m_Scissors.data(),
        };
    }

    VkPipelineRasterizationStateCreateInfo VkGraphicsPipeline::GetRasterizationStateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f,
        };
    }

    VkPipelineMultisampleStateCreateInfo VkGraphicsPipeline::GetMultisampleStateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };
    }

    VkPipelineColorBlendAttachmentState VkGraphicsPipeline::GetColorBlendAttachmentState()
    {
        return {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
    }

    VkPipelineColorBlendStateCreateInfo VkGraphicsPipeline::GetColorBlendStateInfo()
    {
        static VkPipelineColorBlendAttachmentState colorBlendAttachment = GetColorBlendAttachmentState();

        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
        };
    }

    VkPipelineDepthStencilStateCreateInfo VkGraphicsPipeline::GetDepthStencilStateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };
    }

    VkPushConstantRange* VkGraphicsPipeline::GetPushContantRangeInfo(EXUINT32 & size)
    {
        static std::vector<VkPushConstantRange> pushConstants {
            //{
            //    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            //    .offset = sizeof(float) * 0,
            //    .size = sizeof(float) * 4,
            //},
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(VkModelPushConstants),
            }
        };

		size = static_cast<EXUINT32>(pushConstants.size());

        /*static VkPushConstantRange* pushConstants = new VkPushConstantRange();
		pushConstants->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstants->offset = 0;
		pushConstants->size = sizeof(VkModelPushConstants);

        size = 1;*/

		static VkPushConstantRange* ptr = pushConstants.data();

        return ptr;
    }

    VkPipelineLayoutCreateInfo VkGraphicsPipeline::GetLayoutInfo()
    {
        EXUINT32 pushContantsSize = 0;
		auto pushConstants = GetPushContantRangeInfo(pushContantsSize);

        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_pDescriptorSetLayout,
            .pushConstantRangeCount = pushContantsSize,
            .pPushConstantRanges = pushConstants,
        };
    }
}
#endif // EXN_DISABLE_VULKAN