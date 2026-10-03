#pragma once
#ifndef EXN_DISABLE_VULKAN
#include <cassert>
#include <vector>
#include <concepts>

#include <eXngine.h>
#include <vulkan/vulkan.h>
#include <renderers/vulkan/vertex.h>
#include <renderers/vulkan/texture.h>
#include <types/matrix.h>

namespace eXngine::Renderers::Vulkan
{
    // Per-draw data, pushed before every draw call (see shaders/shader.vert / shader.frag).
    struct VkModelPushConstants
    {
    public:
        alignas(16) EXMAT4 model = EXMAT4(1.0f);
        alignas(4) EXUINT32 textureIndex = 0;
    };

    struct VkGraphicsPipeline
    {
    public:
        VkDevice *m_pDevice = EXN_NULL_HANDLE;
        VkDescriptorPool *m_pDescriptorPool = EXN_NULL_HANDLE;
        VkExtent2D *m_pExtent = EXN_NULL_HANDLE;
        VkPipelineLayout m_pLayout = EXN_NULL_HANDLE;
        VkPipeline m_pPipeline = EXN_NULL_HANDLE;
        // Shared by every pipeline: set 0 = per-frame globals (UBO), set 1 = material textures.
        std::vector<VkDescriptorSetLayout> m_SetLayouts;
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
        std::vector<VkViewport> m_Viewports;
        std::vector<VkRect2D> m_Scissors;
        std::vector<VkDynamicState> m_DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        // VkGraphicsPipeline(VkDevice *, VkDescriptorPool *, VkExtent2D *);
        // ~VkGraphicsPipeline();

        VkGraphicsPipeline(VkDevice *device, VkDescriptorPool *pool, VkExtent2D *extent)
        {
            m_pDevice = device;
            m_pDescriptorPool = pool;
            m_pExtent = extent;
            m_ShaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
        };

        virtual ~VkGraphicsPipeline()
        {
            vkDestroyPipelineLayout(*m_pDevice, m_pLayout, nullptr);
            vkDestroyPipeline(*m_pDevice, m_pPipeline, nullptr);
        }

        void SetExtent(VkExtent2D);
        void SetDynamicStates(std::vector<VkDynamicState>);
        void AddViewport(VkViewport);
        void AddScissor(VkRect2D);

        // virtual VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyInfo();
        // virtual VkPipelineDynamicStateCreateInfo GetDynamicStateInfo();
        // virtual VkPipelineViewportStateCreateInfo GetViewportStateInfo();
        // virtual VkPipelineRasterizationStateCreateInfo GetRasterizationStateInfo();
        // virtual VkPipelineMultisampleStateCreateInfo GetMultisampleStateInfo();
        // virtual VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState();
        // virtual VkPipelineColorBlendStateCreateInfo GetColorBlendStateInfo();
        // virtual VkPipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo();
        // virtual VkPushConstantRange *GetPushContantRangeInfo(EXUINT32 &);
        // virtual VkPipelineLayoutCreateInfo GetLayoutInfo();

        VkPipelineInputAssemblyStateCreateInfo virtual GetInputAssemblyInfo()
        {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };
        }

        VkPipelineDynamicStateCreateInfo virtual GetDynamicStateInfo()
        {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<EXUINT32>(m_DynamicStates.size()),
                .pDynamicStates = m_DynamicStates.data(),
            };
        }

        VkPipelineViewportStateCreateInfo virtual GetViewportStateInfo()
        {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = static_cast<EXUINT32>(m_Viewports.size()),
                .pViewports = m_Viewports.data(),
                .scissorCount = static_cast<EXUINT32>(m_Scissors.size()),
                .pScissors = m_Scissors.data(),
            };
        }

        VkPipelineRasterizationStateCreateInfo virtual GetRasterizationStateInfo()
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

        VkPipelineMultisampleStateCreateInfo virtual GetMultisampleStateInfo()
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

        VkPipelineColorBlendAttachmentState virtual GetColorBlendAttachmentState()
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

        VkPipelineColorBlendStateCreateInfo virtual GetColorBlendStateInfo()
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

        VkPipelineDepthStencilStateCreateInfo virtual GetDepthStencilStateInfo()
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

        virtual VkPushConstantRange * GetPushContantRangeInfo(EXUINT32 &size)
        {
            static std::vector<VkPushConstantRange> pushConstants{
                //{
                //    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                //    .offset = sizeof(float) * 0,
                //    .size = sizeof(float) * 4,
                //},
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(VkModelPushConstants),
                }};

            size = static_cast<EXUINT32>(pushConstants.size());

            /*static VkPushConstantRange* pushConstants = new VkPushConstantRange();
            pushConstants->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstants->offset = 0;
            pushConstants->size = sizeof(VkModelPushConstants);

            size = 1;*/

            static VkPushConstantRange *ptr = pushConstants.data();

            return ptr;
        }

        VkPipelineLayoutCreateInfo virtual GetLayoutInfo()
        {
            EXUINT32 pushContantsSize = 0;
            auto pushConstants = GetPushContantRangeInfo(pushContantsSize);

            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = static_cast<EXUINT32>(m_SetLayouts.size()),
                .pSetLayouts = m_SetLayouts.data(),
                .pushConstantRangeCount = pushContantsSize,
                .pPushConstantRanges = pushConstants,
            };
        }

        template <typename T>
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

        template <typename T>
        inline void CreatePipeline(VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout> &setLayouts)
        {
            m_SetLayouts = setLayouts;

            VkPipelineRasterizationStateCreateInfo rasterizer = GetRasterizationStateInfo();
            VkPipelineMultisampleStateCreateInfo multisampling = GetMultisampleStateInfo();
            VkPipelineColorBlendStateCreateInfo colorBlending = GetColorBlendStateInfo();
            VkPipelineDepthStencilStateCreateInfo depthStencil = GetDepthStencilStateInfo();
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputInfo<T>();
            VkPipelineInputAssemblyStateCreateInfo inputAssembly = GetInputAssemblyInfo();
            VkPipelineViewportStateCreateInfo viewportState = GetViewportStateInfo();
            VkPipelineDynamicStateCreateInfo dynamicState = GetDynamicStateInfo();
            VkPipelineLayoutCreateInfo layoutInfo = GetLayoutInfo();

            // Not inside assert(): the call would be compiled out in Release builds.
            const VkResult layoutResult = vkCreatePipelineLayout(*m_pDevice, &layoutInfo, nullptr, &this->m_pLayout);
            EX_FATAL(layoutResult == VK_SUCCESS, "Failed to create pipeline layout.");

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

            const VkResult pipelineResult = vkCreateGraphicsPipelines(*m_pDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->m_pPipeline);
            EX_FATAL(pipelineResult == VK_SUCCESS, "Failed to create graphics pipeline.");
        }
    };

}
#endif // EXN_DISABLE_VULKAN