#pragma once
#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/pipelines/graphics.h>

namespace eXngine::Renderers::Vulkan
{
    // Alpha blending for text and see-through sprites: color = src * a + dst * (1 - a).
    // It still tests depth, so opaque objects in front hide it, but does not write depth:
    // what is behind a transparent pixel must stay visible. Blending needs the background
    // to be drawn first, so draw transparent objects in a pass after the opaque ones, e.g.
    //   renderer->CreateRenderPass("transparent", {.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
    //                                              .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD});
    // The renderer creates one as EXN_TRANSPARENT_PIPELINE with the default shaders, whose
    // fragment shader already takes the alpha from the texture.
    struct VkTransparentPipeline : public VkGraphicsPipeline
    {
        using VkGraphicsPipeline::VkGraphicsPipeline;

        VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState() override
        {
            return {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };
        }

        VkPipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo() override
        {
            VkPipelineDepthStencilStateCreateInfo info = VkGraphicsPipeline::GetDepthStencilStateInfo();
            info.depthWriteEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            return info;
        }
    };
}
#endif // EXN_DISABLE_VULKAN
