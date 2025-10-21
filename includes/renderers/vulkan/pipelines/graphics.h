#pragma once
#ifndef EXN_DISABLE_VULKAN
#include <cassert>
#include <vector>
#include <concepts>

#include <eXngine.h>
#include <vulkan/vulkan.h>
#include <renderers/vulkan/vertex.h>
#include <renderers/vulkan/texture.h>

namespace eXngine::Renderers::Vulkan
{
    struct VkGraphicsPipeline
    {
    public:
        const EXUINT32 MAX_FRAMES_IN_FLIGHT = 2;
        const EXUINT32 MAX_TEXTURE_COUNT = 16;

        VkDevice *m_pDevice = EXN_NULL_HANDLE;
        VkDescriptorPool *m_pDescriptorPool = EXN_NULL_HANDLE;
        VkExtent2D *m_pExtent = EXN_NULL_HANDLE;
        VkPipelineLayout m_pLayout = EXN_NULL_HANDLE;
        VkPipeline m_pPipeline = EXN_NULL_HANDLE;
        VkDescriptorSetLayout m_pDescriptorSetLayout = EXN_NULL_HANDLE;
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
        std::vector<VkDescriptorSet> m_DescriptorSets;
        std::vector<VkViewport> m_Viewports;
        std::vector<VkRect2D> m_Scissors;
        std::vector<VkDynamicState> m_DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkGraphicsPipeline(VkDevice *, VkDescriptorPool *, VkExtent2D *);
        // ~VkGraphicsPipeline();

        virtual ~VkGraphicsPipeline()
        {
            vkDestroyPipelineLayout(*m_pDevice, m_pLayout, nullptr);
            vkDestroyPipeline(*m_pDevice, m_pPipeline, nullptr);
            vkDestroyDescriptorSetLayout(*m_pDevice, m_pDescriptorSetLayout, nullptr);
        }

        void SetExtent(VkExtent2D);
        void SetDynamicStates(std::vector<VkDynamicState>);
        void AddViewport(VkViewport);
        void AddScissor(VkRect2D);

        void CreateDescriptorSetLayout();
        void CreateDescriptorSets(VkSampler, VkTexture *, VkBuffer);

        virtual VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyInfo();
        virtual VkPipelineDynamicStateCreateInfo GetDynamicStateInfo();
        virtual VkPipelineViewportStateCreateInfo GetViewportStateInfo();
        virtual VkPipelineRasterizationStateCreateInfo GetRasterizationStateInfo();
        virtual VkPipelineMultisampleStateCreateInfo GetMultisampleStateInfo();
        virtual VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState();
        virtual VkPipelineColorBlendStateCreateInfo GetColorBlendStateInfo();
        virtual VkPipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo();
        virtual VkPushConstantRange *GetPushContantRangeInfo(EXUINT32 &);
        virtual VkPipelineLayoutCreateInfo GetLayoutInfo();

        template <std::derived_from<VkVertex> T>
        inline VkPipelineVertexInputStateCreateInfo GetVertexInputInfo()
        {
            static VkVertexInputBindingDescription bindingDescription = T::GetBindingDescription();
            static std::vector<VkVertexInputAttributeDescription> attributeDescriptions = T::GetAttributeDescriptions();

            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &bindingDescription,
                .vertexAttributeDescriptionCount = static_cast<EXUINT32>(attributeDescriptions.size()),
                .pVertexAttributeDescriptions = attributeDescriptions.data(),
            };
        }

        template <std::derived_from<VkVertex> T>
        inline void CreatePipeline(VkRenderPass renderPass)
        {
            VkPipelineRasterizationStateCreateInfo rasterizer = GetRasterizationStateInfo();
            VkPipelineMultisampleStateCreateInfo multisampling = GetMultisampleStateInfo();
            VkPipelineColorBlendStateCreateInfo colorBlending = GetColorBlendStateInfo();
            VkPipelineDepthStencilStateCreateInfo depthStencil = GetDepthStencilStateInfo();
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputInfo<T>();
            VkPipelineInputAssemblyStateCreateInfo inputAssembly = GetInputAssemblyInfo();
            VkPipelineViewportStateCreateInfo viewportState = GetViewportStateInfo();
            VkPipelineDynamicStateCreateInfo dynamicState = GetDynamicStateInfo();
            VkPipelineLayoutCreateInfo layoutInfo = GetLayoutInfo();

            assert(vkCreatePipelineLayout(*m_pDevice, &layoutInfo, nullptr, &this->m_pLayout) == VK_SUCCESS);

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = (EXUINT32)this->m_ShaderStages.size();
            pipelineInfo.pStages = this->m_ShaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = this->m_pLayout;
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = -1;

            assert(vkCreateGraphicsPipelines(*m_pDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->m_pPipeline) == VK_SUCCESS);
        }
    };

}
#endif // EXN_DISABLE_VULKAN