#ifndef EXN_DISABLE_VULKAN
#include <array>

#include <renderers/vulkan/renderpass.h>
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    VkRenderPassObject::VkRenderPassObject(Renderer *renderer, const std::string &name, const VkRenderPassDescription &description)
        : m_pRenderer(renderer), m_Name(name), m_Description(description)
    {
    }

    VkRenderPassObject::~VkRenderPassObject()
    {
        Release();
    }

    void VkRenderPassObject::Create()
    {
        const bool loadColor = m_Description.colorLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD;
        const bool loadDepth = m_Description.depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD;

        std::vector<VkAttachmentDescription> attachments{
            {
                .format = m_pRenderer->m_swapChainImageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = m_Description.colorLoadOp,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                // Every pass leaves the swap chain image ready to present; a pass that
                // loads the image picks it up in that layout.
                .initialLayout = loadColor ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
        };

        if (m_Description.useDepth)
        {
            attachments.push_back({
                .format = m_pRenderer->FindDepthFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = m_Description.depthLoadOp,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // a later pass may load it
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = loadDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            });
        }

        const VkAttachmentReference colorRef{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depthRef{.attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        const VkSubpassDescription subpass{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
            .pDepthStencilAttachment = m_Description.useDepth ? &depthRef : nullptr,
        };

        // Wait for attachment writes of the previous pass (and of the previous frame, which
        // shares the depth buffer) before this pass loads, clears or writes its attachments.
        constexpr VkPipelineStageFlags attachmentStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        const VkSubpassDependency dependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = attachmentStages,
            .dstStageMask = attachmentStages,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };

        const VkRenderPassCreateInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<EXUINT32>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
        };

        const VkResult result = vkCreateRenderPass(m_pRenderer->m_pDevice, &renderPassInfo, nullptr, &m_pRenderPass);
        EX_FATAL(result == VK_SUCCESS, "Failed to create render pass.");

        CreateFramebuffers();
    }

    void VkRenderPassObject::CreateFramebuffers()
    {
        DestroyFramebuffers();

        const auto &imageViews = m_pRenderer->m_swapChainImageViews;
        const VkExtent2D extent = m_pRenderer->m_szSwapChainExtent;

        m_Framebuffers.resize(imageViews.size(), EXN_NULL_HANDLE);

        for (size_t i = 0; i < imageViews.size(); ++i)
        {
            std::vector<VkImageView> attachments{imageViews[i]};

            if (m_Description.useDepth)
                attachments.push_back(m_pRenderer->m_Depth->m_pView);

            const VkFramebufferCreateInfo framebufferInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_pRenderPass,
                .attachmentCount = static_cast<EXUINT32>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = extent.width,
                .height = extent.height,
                .layers = 1,
            };

            const VkResult result = vkCreateFramebuffer(m_pRenderer->m_pDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]);
            EX_FATAL(result == VK_SUCCESS, "Failed to create framebuffer for swap chain image.");
        }
    }

    void VkRenderPassObject::DestroyFramebuffers()
    {
        for (const auto framebuffer : m_Framebuffers)
            vkDestroyFramebuffer(m_pRenderer->m_pDevice, framebuffer, nullptr);

        m_Framebuffers.clear();
    }

    void VkRenderPassObject::Release()
    {
        if (m_pRenderer->m_pDevice == EXN_NULL_HANDLE)
            return;

        DestroyFramebuffers();

        vkDestroyRenderPass(m_pRenderer->m_pDevice, m_pRenderPass, nullptr);
        m_pRenderPass = EXN_NULL_HANDLE;
    }

    void VkRenderPassObject::Begin(VkCommandBuffer commandBuffer, EXUINT32 imageIndex)
    {
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = m_Description.clearColor;
        clearValues[1].depthStencil = {1.0f, 0};

        const VkExtent2D extent = m_pRenderer->m_szSwapChainExtent;

        const VkRenderPassBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = m_pRenderPass,
            .framebuffer = m_Framebuffers[imageIndex],
            .renderArea = {{0, 0}, extent},
            .clearValueCount = m_Description.useDepth ? 2u : 1u,
            .pClearValues = clearValues.data(),
        };

        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Viewport and scissor are dynamic in every pipeline: default to the whole image.
        const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
        const VkRect2D scissor{{0, 0}, extent};

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VkRenderPassObject::RecordDraws(VkCommandBuffer commandBuffer, EXUINT32 frameIndex)
    {
        const VkDescriptorSet globalSet = m_pRenderer->m_pFrameObjects[frameIndex].descriptorSet;

        const VkGraphicsPipeline *boundPipeline = EXN_NULL_HANDLE;
        VkMaterial *boundMaterial = EXN_NULL_HANDLE;
        const VkMesh *boundMesh = EXN_NULL_HANDLE;

        for (const auto &draw : m_DrawCommands)
        {
            if (draw.mesh == EXN_NULL_HANDLE || draw.material == EXN_NULL_HANDLE || draw.mesh->GetIndexCount() == 0)
                continue;

            VkGraphicsPipeline *pipeline = draw.material->GetPipeline();

            if (pipeline == EXN_NULL_HANDLE || pipeline->m_pPipeline == EXN_NULL_HANDLE)
                continue;

            // Only rebind what changed between consecutive draws.
            if (pipeline != boundPipeline)
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_pPipeline);

                if (!pipeline->m_Viewports.empty())
                    vkCmdSetViewport(commandBuffer, 0, static_cast<EXUINT32>(pipeline->m_Viewports.size()), pipeline->m_Viewports.data());

                if (!pipeline->m_Scissors.empty())
                    vkCmdSetScissor(commandBuffer, 0, static_cast<EXUINT32>(pipeline->m_Scissors.size()), pipeline->m_Scissors.data());

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_pLayout, 0, 1, &globalSet, 0, nullptr);

                boundPipeline = pipeline;
                boundMaterial = EXN_NULL_HANDLE;
            }

            if (draw.material != boundMaterial)
            {
                draw.material->Bind(commandBuffer, pipeline->m_pLayout, frameIndex);
                boundMaterial = draw.material;
            }

            if (draw.mesh != boundMesh)
            {
                draw.mesh->Bind(commandBuffer, frameIndex);
                boundMesh = draw.mesh;
            }

            const VkModelPushConstants constants{
                .model = draw.model,
                .textureIndex = draw.textureIndex,
            };

            vkCmdPushConstants(commandBuffer, pipeline->m_pLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(constants), &constants);
            vkCmdDrawIndexed(commandBuffer, draw.mesh->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    void VkRenderPassObject::End(VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }

    VkDrawCommand *VkRenderPassObject::Draw(VkMesh *mesh, VkMaterial *material, const EXMAT4 &model, EXUINT32 textureIndex)
    {
        m_DrawCommands.push_back({
            .mesh = mesh,
            .material = material,
            .model = model,
            .textureIndex = textureIndex,
        });

        return &m_DrawCommands.back();
    }

    void VkRenderPassObject::Remove(VkDrawCommand *command)
    {
        m_DrawCommands.remove_if([command](const VkDrawCommand &draw)
                                 { return &draw == command; });
    }

    void VkRenderPassObject::ClearDraws()
    {
        m_DrawCommands.clear();
    }
}
#endif
