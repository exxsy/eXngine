#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/pipelines/graphics.h>

namespace eXngine::Renderers::Vulkan
{
	VkGraphicsPipeline::VkGraphicsPipeline(VkDevice device, VkExtent2D extent) 
	{
		m_pDevice = device;
		m_ShaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
		m_pExtent = extent;
        m_Scissors = {
            {
				.offset = { 0, 0 },
				.extent = m_pExtent
            }
        };
        m_Viewports = {
            {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float)m_pExtent.width,
                .height = (float)m_pExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            }
        };
	};

    VkGraphicsPipeline::~VkGraphicsPipeline()
    {
        vkDestroyPipelineLayout(m_pDevice, m_pLayout, nullptr);
		vkDestroyPipeline(m_pDevice, m_pPipeline, nullptr);
		vkDestroyDescriptorSetLayout(m_pDevice, m_pDescriptorSetLayout, nullptr);
    }

    void VkGraphicsPipeline::SetExtent(VkExtent2D extent)
    {
		m_pExtent = extent;
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
		states = s;
    }

    void VkGraphicsPipeline::CreateDescriptorPool()
    {
        constexpr int maxCount = 10;

        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, maxCount},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxCount}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * EX_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = static_cast<EXUINT32>(EX_ARRAYSIZE(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;

        assert(vkCreateDescriptorPool(m_pDevice, &pool_info, nullptr, &m_pDescriptorPool) == VK_SUCCESS);
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

        assert(vkCreateDescriptorSetLayout(m_pDevice, &layoutInfo, nullptr, &m_pDescriptorSetLayout) == VK_SUCCESS);
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
            .dynamicStateCount = static_cast<EXUINT32>(states.size()),
            .pDynamicStates = states.data(),
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