#ifdef _VULKAN

#include <cassert>
#include <iostream>
#include <vector>
#include <eXngine.h>
#include <set>
#include <algorithm>
#include <fstream>
#include <renderers/vulkan.h>

#pragma comment(lib, "vulkan-1.lib")

#undef max

bool eXngine::Renderers::VulkanRenderer::Initialize()
{
    CreateSurface();
    SelectPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();

    return true;
}

void eXngine::Renderers::VulkanRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    assert(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_pRenderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
}

void eXngine::Renderers::VulkanRenderer::OnRender()
{
    vkWaitForFences(m_pDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_pDevice, 1, &m_inFlightFences[m_currentFrame]);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(m_pDevice, m_pSwapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_bFrameBufferResized.store(false);
        this->ResetSwapChain();
        return;
    }

    assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

    vkResetCommandBuffer(m_pCommandBuffers[m_currentFrame], 0);
    RecordCommandBuffer(m_pCommandBuffers[m_currentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_pCommandBuffers[m_currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    assert(vkQueueSubmit(m_pGraphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) == VK_SUCCESS);

    VkPresentInfoKHR presentInfo{};
    VkSwapchainKHR swapChains[] = {m_pSwapChain};

    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_pPresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        this->ResetSwapChain();
        return;
    }

    assert(result == VK_SUCCESS);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void eXngine::Renderers::VulkanRenderer::OnExit()
{
    vkDeviceWaitIdle(m_pDevice);
 
    CleanupSwapChain();
    
    vkDestroySurfaceKHR(m_pInstance, m_pSurface, nullptr);
    vkDestroyPipeline(m_pDevice, m_pGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_pDevice, m_pPipelineLayout, nullptr);
    vkDestroyRenderPass(m_pDevice, m_pRenderPass, nullptr);

    for (auto shaderModule : m_shaderModules)
    {
        vkDestroyShaderModule(m_pDevice, shaderModule, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_pDevice, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_pDevice, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_pDevice, m_inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_pDevice, m_pCommandPool, nullptr);
    vkDestroyDevice(m_pDevice, nullptr);
    vkDestroyInstance(m_pInstance, nullptr);
}

VkInstance eXngine::Renderers::VulkanRenderer::GetVulkanInstance()
{
    return this->m_pInstance;
}

void eXngine::Renderers::VulkanRenderer::SetShaders(const std::vector<std::tuple<const char *, VkShaderStageFlagBits, std::vector<char>>> shaders)
{
    this->m_shaders = shaders;
}

void eXngine::Renderers::VulkanRenderer::SetSurface(VkSurfaceKHR surface)
{
    this->m_pSurface = surface;
}

void eXngine::Renderers::VulkanRenderer::SetExtensions(std::vector<const char *> ex)
{
    this->m_extensions = ex;
}

void eXngine::Renderers::VulkanRenderer::SetMaxFramesInFlight(int max)
{
    this->MAX_FRAMES_IN_FLIGHT = max;
}

void eXngine::Renderers::VulkanRenderer::CreateInstance(std::vector<const char *> extensions)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = this->m_szName;
    appInfo.pEngineName = EXENGINE;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    {
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : m_validationLayers)
        {
            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    break;
                }
            }
        }

        if (m_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_pInstance);
        if (result != VK_SUCCESS)
            std::cout << "m_pInstance is not created due to, (" << result << ") error code." << std::endl;
    }
}

void eXngine::Renderers::VulkanRenderer::CreateSurface()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    printf("Extensions: \n");

    for (const auto &extension : extensions)
    {
        std::cout << '\t\t' << extension.extensionName << '\n';
    }
}

void eXngine::Renderers::VulkanRenderer::CreateGraphicsPipeline()
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    for (const auto &m : m_shaders)
    {
        std::apply(
            [&](const char *name, VkShaderStageFlagBits stage, std::vector<char> code)
            {
                auto shaderModule = this->CreateShaderModule(code);
                this->m_shaderModules.push_back(shaderModule);

                VkPipelineShaderStageCreateInfo shaderStageInfo{};
                shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStageInfo.stage = stage; // VK_SHADER_STAGE_VERTEX_BIT;
                shaderStageInfo.module = shaderModule;
                shaderStageInfo.pName = name; //?

                shaderStages.push_back(shaderStageInfo);
            },
            m);
    }

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainExtent.width;
    viewport.height = (float)m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
    dynamicState.pDynamicStates = m_dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayout pipelineLayout;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    assert(vkCreatePipelineLayout(m_pDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data(); // new VkPipelineShaderStageCreateInfo[2]{ shaderStages[0], shaderStages[1] };//
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = m_pRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    assert(vkCreateGraphicsPipelines(m_pDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pGraphicsPipeline) == VK_SUCCESS);
}

void eXngine::Renderers::VulkanRenderer::CreateSyncObjects()
{
    // VkSemaphoreCreateInfo semaphoreInfo{};
    // semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // VkFenceCreateInfo fenceInfo{};
    // fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        assert(vkCreateSemaphore(m_pDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) == VK_SUCCESS);
        assert(vkCreateSemaphore(m_pDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) == VK_SUCCESS);
        assert(vkCreateFence(m_pDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) == VK_SUCCESS);
    }
}

void eXngine::Renderers::VulkanRenderer::CreateFramebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {m_swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_pRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        assert(vkCreateFramebuffer(m_pDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) == VK_SUCCESS);
    }
}

void eXngine::Renderers::VulkanRenderer::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamiliesWithSurfaces(m_pPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    assert(vkCreateCommandPool(m_pDevice, &poolInfo, nullptr, &m_pCommandPool) == VK_SUCCESS);
}

void eXngine::Renderers::VulkanRenderer::CreateCommandBuffers()
{
    m_pCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_pCommandBuffers.size();

    assert(vkAllocateCommandBuffers(m_pDevice, &allocInfo, m_pCommandBuffers.data()) == VK_SUCCESS);
}

VkShaderModule eXngine::Renderers::VulkanRenderer::CreateShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    assert(vkCreateShaderModule(m_pDevice, &createInfo, nullptr, &shaderModule) == VK_SUCCESS);

    return shaderModule;
}

void eXngine::Renderers::VulkanRenderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamiliesWithSurfaces(m_pPhysicalDevice);
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily)
        {
            m_queueRenderFamily = *indices.graphicsFamily;
        }

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

        if (m_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        assert(vkCreateDevice(m_pPhysicalDevice, &createInfo, nullptr, &m_pDevice) == VK_SUCCESS);

        vkGetDeviceQueue(m_pDevice, indices.graphicsFamily.value(), 0, &m_pGraphicsQueue);
        vkGetDeviceQueue(m_pDevice, indices.presentFamily.value(), 0, &m_pPresentQueue);
    }
}

void eXngine::Renderers::VulkanRenderer::CreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_pPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_pSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    assert(vkCreateSwapchainKHR(m_pDevice, &createInfo, nullptr, &m_pSwapChain) == VK_SUCCESS);

    vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void eXngine::Renderers::VulkanRenderer::CreateImageViews()
{
    if (m_swapChainImageViews.empty() == false)
    {
        for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
        {
            vkDestroyImageView(m_pDevice, m_swapChainImageViews[i], nullptr);
        }
        m_swapChainImageViews.clear();
    }

    m_swapChainImageViews.clear();
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] = CreateImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void eXngine::Renderers::VulkanRenderer::CreateRenderPass()
{
    // Create a simple render pass with a single color attachment.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    const VkAttachmentDescription attachments[] = {colorAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(sizeof(attachments) / sizeof(attachments[0]));
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    const VkResult result = vkCreateRenderPass(m_pDevice, &renderPassInfo, nullptr, &m_pRenderPass);
    assert(result == VK_SUCCESS);
}

void eXngine::Renderers::VulkanRenderer::CleanupSwapChain()
{
    for (auto framebuffer : m_swapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_pDevice, framebuffer, nullptr);
    }

    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_pDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_pDevice, m_pSwapChain, nullptr);
}

void eXngine::Renderers::VulkanRenderer::ResetSwapChain()
{
    vkDeviceWaitIdle(m_pDevice);

    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
}

void eXngine::Renderers::VulkanRenderer::SelectPhysicalDevice()
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, nullptr);
    assert(deviceCount > 0);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    assert(physicalDevice != VK_NULL_HANDLE);
    m_pPhysicalDevice = physicalDevice;
}

bool eXngine::Renderers::VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool eXngine::Renderers::VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice device)
{
    return true;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    const bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

eXngine::Renderers::SwapChainSupportDetails
eXngine::Renderers::VulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_pSurface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_pSurface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_pSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_pSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_pSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

eXngine::Renderers::QueueFamilyIndices
eXngine::Renderers::VulkanRenderer::FindQueueFamiliesWithSurfaces(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_pSurface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        i++;
    }

    return indices;
}

VkSurfaceFormatKHR eXngine::Renderers::VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR eXngine::Renderers::VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D eXngine::Renderers::VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent =
            {
                static_cast<uint32_t>(this->m_frameBufferSize.W),
                static_cast<uint32_t>(this->m_frameBufferSize.H)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkImageView eXngine::Renderers::VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    assert(vkCreateImageView(m_pDevice, &viewInfo, nullptr, &imageView) == VK_SUCCESS);

    return imageView;
}

VkFormat eXngine::Renderers::VulkanRenderer::FindDepthFormat()
{
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat eXngine::Renderers::VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_pPhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    assert(false);
    return VkFormat{};
}

eXngine::Renderers::VulkanRenderer::VulkanRenderer(const char *name) : Renderer(name), m_frameBufferSize(0, 0)
{
    CreateInstance(this->m_extensions);
}

eXngine::Renderers::VulkanRenderer::VulkanRenderer(const char* name, Size sz) : Renderer(name), m_frameBufferSize(sz)
{
    CreateInstance(this->m_extensions);
}


eXngine::Renderers::VulkanRenderer::VulkanRenderer(const char *name, Size sz, std::vector<const char *> extensions) : Renderer(name), m_frameBufferSize(sz)
{
    m_extensions.insert(m_extensions.end(), extensions.begin(), extensions.end());
    CreateInstance(this->m_extensions);
}
#endif