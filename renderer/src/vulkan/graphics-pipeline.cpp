#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>

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
}
#endif // EXN_DISABLE_VULKAN