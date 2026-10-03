#ifndef EXN_DISABLE_VULKAN
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <array>
#include <string_view>
#include <vector>
#include <ranges>
#include <filesystem>

#include <eXngine.h>
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    void Renderer::Initialize()
    {
        m_pFrameObjects.resize(MAX_FRAMES_IN_FLIGHT + 1);

        SelectPhysicalDevice();
        CreateInstance();
        CreateSurface();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateCommandPool();
        CreateImageViews();
        CreateDepthResources();
        CreateRenderPasses();
        CreateShaders();
        CreateTextureSampler();
        CreateDefaultTexture();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSetLayouts();
        CreateGlobalDescriptorSets();
        CreateCommandBuffers();
        CreateSyncObjects();

        CreatePipeline<VkVertex>(EXN_DEFAULT_PIPELINE);

        EX_INFO("Vulkan renderer initialized.");
    }

    void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, EXUINT32 imageIndex)
    {
        m_pCommandBuffer = commandBuffer;

        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        // EX_ERROR(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer for recording.");

        for (size_t i = 0; i < m_RenderPasses.size(); ++i)
        {
            VkRenderPassObject *pass = m_RenderPasses[i];

            pass->Begin(commandBuffer, imageIndex);
            pass->RecordDraws(commandBuffer, m_currentFrame);

            // The render handler (e.g. ImGui) draws on top of everything, inside the last pass.
            if (i + 1 == m_RenderPasses.size() && m_fOnRender != EXN_NULL_HANDLE)
                m_fOnRender(this);

            pass->End(commandBuffer);
        }

        vkEndCommandBuffer(commandBuffer);
    }

    void Renderer::UpdateUniformBuffers()
    {
        if (m_pMainCamera == EXN_NULL_HANDLE)
            return;

        UniformBufferObject ubo;
        ubo.model = EXMATH::mat4(1.0f);
        ubo.view = m_pMainCamera->GetViewMatrix();
        ubo.proj = m_pMainCamera->GetProjectionMatrix();
        ubo.proj[1][1] *= -1;

        memcpy(m_pFrameObjects[m_currentFrame].uniformBuffersMapped, &ubo, sizeof(UniformBufferObject));
    }

    EXUINT32 Renderer::FindMemoryType(EXUINT32 typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_pPhysicalDevice, &memProperties);

        for (EXUINT32 i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        EX_FATAL(false, "Failed to find suitable memory type for buffer allocation.");
        return 0;
    }

    void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used by graphics queue

        EX_FATAL(vkCreateBuffer(m_pDevice, &bufferInfo, nullptr, &buffer) == VK_SUCCESS, "Failed to create buffer.");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_pDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        EX_FATAL(vkAllocateMemory(m_pDevice, &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Failed to allocate buffer memory.");

        vkBindBufferMemory(m_pDevice, buffer, bufferMemory, 0);
    }

    void Renderer::CreateDescriptorSetLayouts()
    {
        const VkDescriptorSetLayoutBinding uboBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        const VkDescriptorSetLayoutCreateInfo globalInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboBinding,
        };

        VkResult result = vkCreateDescriptorSetLayout(m_pDevice, &globalInfo, nullptr, &m_pGlobalSetLayout);
        EX_FATAL(result == VK_SUCCESS, "Failed to create global descriptor set layout.");

        const VkDescriptorSetLayoutBinding texturesBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        const VkDescriptorSetLayoutCreateInfo materialInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &texturesBinding,
        };

        result = vkCreateDescriptorSetLayout(m_pDevice, &materialInfo, nullptr, &m_pMaterialSetLayout);
        EX_FATAL(result == VK_SUCCESS, "Failed to create material descriptor set layout.");
    }

    void Renderer::CreateGlobalDescriptorSets()
    {
        const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_pGlobalSetLayout);
        std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);

        const VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_pDescriptorPool,
            .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts.data(),
        };

        const VkResult result = vkAllocateDescriptorSets(m_pDevice, &allocInfo, descriptorSets.data());
        EX_FATAL(result == VK_SUCCESS, "Failed to allocate global descriptor sets.");

        for (EXUINT32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            auto &frame = m_pFrameObjects[i];
            frame.descriptorSet = descriptorSets[i];

            const VkDescriptorBufferInfo bufferInfo{
                .buffer = frame.uniformBuffer,
                .offset = 0,
                .range = sizeof(UniformBufferObject),
            };

            const VkWriteDescriptorSet write{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = frame.descriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo,
            };

            vkUpdateDescriptorSets(m_pDevice, 1, &write, 0, nullptr);
        }
    }

    std::vector<VkDescriptorSetLayout> Renderer::GetPipelineSetLayouts() const
    {
        return {m_pGlobalSetLayout, m_pMaterialSetLayout};
    }

    VkCommandBuffer Renderer::BeginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_pCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        const VkResult allocateResult = vkAllocateCommandBuffers(m_pDevice, &allocInfo, &commandBuffer);
        EX_FATAL(allocateResult == VK_SUCCESS, "Failed to allocate command buffer.");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        const VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        EX_FATAL(result == VK_SUCCESS, "Failed to begin single-time command buffer.");

        return commandBuffer;
    }

    void Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_pGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_pGraphicsQueue);
        vkFreeCommandBuffers(m_pDevice, m_pCommandPool, 1, &commandBuffer);
    }

    void Renderer::OnRender()
    {
        EXUINT32 imageIndex = 0;
        VkFrameObject &frameObject = m_pFrameObjects[m_currentFrame];
        vkWaitForFences(m_pDevice, 1, &frameObject.inFlightFence, VK_TRUE, UINT64_MAX);
        VkResult result = vkAcquireNextImageKHR(m_pDevice, m_pSwapChain, UINT64_MAX, frameObject.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            EX_INFO("Swap chain out of date, recreating...");
            ResetSwapChain();
            return;
        }
        else
        {
            EX_ERROR(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire next image from swap chain.");
        }

        vkResetFences(m_pDevice, 1, &frameObject.inFlightFence);
        UpdateUniformBuffers();
        // m_pCurrentCommandBuffer = frameObject.commandBuffer;
        // m_fOnUpdateUniformBuffers(frameObject.uniformBuffersMapped, m_currentFrame);
        RecordCommandBuffer(frameObject.commandBuffer, imageIndex);

        VkSemaphore waitSemaphores[] = {frameObject.imageAvailableSemaphore};
        VkSemaphore signalSemaphores[] = {m_pFrameObjects[imageIndex].renderFinishedSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = EX_ARRAYSIZE(waitSemaphores);
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &frameObject.commandBuffer;
        submitInfo.signalSemaphoreCount = EX_ARRAYSIZE(signalSemaphores);
        submitInfo.pSignalSemaphores = signalSemaphores;

        const VkResult queueSubmitResult = vkQueueSubmit(m_pGraphicsQueue, 1, &submitInfo, frameObject.inFlightFence);
        EX_ERROR(queueSubmitResult == VK_SUCCESS, "Failed to submit draw command buffer to graphics queue.");

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

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            EX_INFO("Swap chain out of date, recreating...");
            ResetSwapChain();
            return;
        }
        else
        {
            EX_ERROR(result == VK_SUCCESS, "Failed to present swap chain image.");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        // m_pCurrentCommandBuffer = EXN_NULL_HANDLE;
    }

    void Renderer::OnExit()
    {
        vkDeviceWaitIdle(m_pDevice);

        if (m_fOnCleanup)
            m_fOnCleanup();

        CleanupSwapChain();

        for (auto &pipeline : m_pGraphicPipelines)
        {
            delete pipeline.second;
        }

        m_pGraphicPipelines.clear();

        for (auto &[name, material] : m_Materials)
            delete material;

        for (auto *mesh : m_Meshes)
            delete mesh;

        for (auto &[name, texture] : m_Textures)
            delete texture;

        for (auto *pass : m_RenderPasses)
            delete pass;

        m_Materials.clear();
        m_Meshes.clear();
        m_Textures.clear();
        m_RenderPasses.clear();

        vkDestroySurfaceKHR(m_pInstance, m_pSurface, nullptr);
        // vkDestroyPipeline(m_pDevice, m_pDefaultGraphicsPipeline->m_pPipeline, nullptr);
        // vkDestroyPipelineLayout(m_pDevice, m_pDefaultGraphicsPipeline->m_pLayout, nullptr);
        vkDestroySampler(m_pDevice, m_pTextureSampler, nullptr);
        vkDestroyDescriptorSetLayout(m_pDevice, m_pGlobalSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_pDevice, m_pMaterialSetLayout, nullptr);
        vkDestroyDescriptorPool(m_pDevice, m_pDescriptorPool, nullptr);

        m_Depth->Release(m_pDevice);
        m_DefaultTexture->Release(m_pDevice);

        for (auto &shader : m_Shaders)
        {
            delete shader.second;
        }

        for (auto &frame : m_pFrameObjects)
        {
            frame.Release(this->m_pDevice);
        }

        vkDestroyCommandPool(m_pDevice, m_pCommandPool, nullptr);
        vkDestroyDevice(m_pDevice, nullptr);
        DestroyDebugMessenger();
        vkDestroyInstance(m_pInstance, nullptr);
    }

    bool Renderer::LoadShader(const char *name, const std::vector<char> &data, const ShaderTypes type)
    {
        if (data.empty())
            return false;

        std::string pipeline = "default";
        std::string entrypoint = name;

        if (strstr(name, ".") != nullptr)
        {
            auto parts = std::views::split(std::string(name), '.');

            EXUINT32 index = 0;
            for (auto &&part : parts)
            {
                if (index == 0)
                    pipeline = std::string(part.begin(), part.end());
                else if (index == 1)
                    entrypoint = std::string(part.begin(), part.end());

                index++;
            }
        }

        auto shaderObject = new VkShaderModuleObject(this->m_pDevice, nullptr);
        shaderObject->type = type;
        shaderObject->code = data;

        if (!pipeline.empty())
        {
            const auto it = this->m_pGraphicPipelines.find(pipeline);

            if (it != this->m_pGraphicPipelines.end())
            {
                shaderObject->m_pPipeline = it->second;
            }
        }

        this->m_Shaders.emplace(name, shaderObject);

        return true;
    }

    void Renderer::DestroyShader(const char *name)
    {
        const auto shader_module = this->m_Shaders.find(name);

        if (this->m_Shaders.find(name) == this->m_Shaders.end())
            return;

        const auto shader = reinterpret_cast<VkShaderModuleObject *>(shader_module->second);

        vkDestroyShaderModule(this->m_pDevice, shader->m_pShader, nullptr);

        if (shader->m_pPipeline != EXN_NULL_HANDLE)
            vkDestroyPipeline(this->m_pDevice, shader->m_pPipeline->m_pPipeline, nullptr);
    }

    void Renderer::UseShader(const char *name)
    {
        if (m_pCurrentCommandBuffer == EXN_NULL_HANDLE)
            return;

        const auto shader_module = this->m_Shaders.find(name);

        if (this->m_Shaders.find(name) == this->m_Shaders.end())
            return;

        const auto shader = reinterpret_cast<VkShaderModuleObject *>(shader_module->second);
        const auto pipeline = shader->m_pPipeline->m_pPipeline;

        vkCmdBindPipeline(m_pCurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    VkInstance Renderer::GetVulkanInstance()
    {
        return this->m_pInstance;
    }

    void Renderer::SetSurface(VkSurfaceKHR surface)
    {
        this->m_pSurface = surface;
    }

    void Renderer::SetExtensions(std::vector<const char *> ex)
    {
        this->m_Extensions = ex;
    }

    void Renderer::CreateInstance()
    {
        if (this->m_pInstance != EXN_NULL_HANDLE)
        {
            EX_TRACE("Vulkan instance already created.");
            return;
        }

        char name[256] = {0};

#ifdef UNICODE
        size_t out_size;
        wcstombs_s(&out_size, name, (const wchar_t *)this->m_szName, sizeof(name));
#else
        strncpy_s(name, this->m_szName, sizeof(name));
#endif

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name;
        appInfo.pEngineName = EXENGINE;
        appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        {
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = static_cast<EXUINT32>(m_Extensions.size());
            createInfo.ppEnabledExtensionNames = m_Extensions.data();

            EXUINT32 layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char *layerName : m_ValidationLayers)
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
                createInfo.enabledLayerCount = static_cast<EXUINT32>(m_ValidationLayers.size());
                createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
            }
            else
            {
                createInfo.enabledLayerCount = 0;
            }

            const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_pInstance);

            EX_FATAL(result == VK_SUCCESS, "Failed to create Vulkan instance!");
        }
    }

    void Renderer::CreateDebugPipeline()
    {
        if (!m_enableValidationLayers || m_pInstance == EXN_NULL_HANDLE)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        // Debug callback lambda
        createInfo.pfnUserCallback = [](
                                         VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                         void *pUserData) -> VkBool32
        {
            EX_INFO("Vulkan Debug Message: %s\n", pCallbackData->pMessage);
            // Format message based on severity
            // const char* severityStr = "UNKNOWN";
            // if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            //     severityStr = "VERBOSE";
            // else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            //     severityStr = "INFO";
            // else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            //     severityStr = "WARNING";
            // else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            //     severityStr = "ERROR";

            // // Log using the engine's logging system
            // if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            //     EX_ERROR(false, string::format("[Vulkan %s] %s", severityStr, pCallbackData->pMessage).c_str());
            // else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            //     EX_WARNING(string::format("[Vulkan %s] %s", severityStr, pCallbackData->pMessage).c_str());
            // else
            //     EX_INFO(string::format("[Vulkan %s] %s", severityStr, pCallbackData->pMessage).c_str());

            return VK_FALSE;
        };

        // Get the function pointer for creating debug messenger
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_pInstance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr)
        {
            VkResult result = func(m_pInstance, &createInfo, nullptr, &m_pDebugMessenger);
            if (result == VK_SUCCESS)
            {
                EX_INFO("Vulkan debug messenger created successfully");
            }
            else
            {
                EX_WARNING("Failed to create Vulkan debug messenger");
            }
        }
        else
        {
            EX_WARNING("vkCreateDebugUtilsMessengerEXT extension not available");
        }
    }

    void Renderer::DestroyDebugMessenger()
    {
        if (m_pDebugMessenger == EXN_NULL_HANDLE || m_pInstance == EXN_NULL_HANDLE)
            return;

        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_pInstance, "vkDestroyDebugUtilsMessengerEXT");

        if (func != nullptr)
        {
            func(m_pInstance, m_pDebugMessenger, nullptr);
            m_pDebugMessenger = EXN_NULL_HANDLE;
            EX_INFO("Vulkan debug messenger destroyed");
        }
    }

    void Renderer::CreateUniformBuffers()
    {
        const VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            auto &frame = m_pFrameObjects[i];

            CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         frame.uniformBuffer, frame.uniformBuffersMemory);

            vkMapMemory(m_pDevice, frame.uniformBuffersMemory, 0, bufferSize, 0, &frame.uniformBuffersMapped);
        }
    }

    void Renderer::CreateSurface()
    {
        EXUINT32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        EX_INFO("Available Vulkan extensions:");

        for (const auto &extension : extensions)
        {
            EX_INFO("\t%s", extension.extensionName);
        }
    }

    void Renderer::CreateDefaultGraphicsPipeline()
    {
        this->m_pGraphicPipelines.emplace(EXN_DEFAULT_PIPELINE, new VkGraphicsPipeline(&m_pDevice, &m_pDescriptorPool, &m_szSwapChainExtent));
    }

    void Renderer::CreateSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < m_pFrameObjects.size(); i++)
        {
            EX_FATAL(vkCreateSemaphore(m_pDevice, &semaphoreInfo, nullptr, &m_pFrameObjects[i].imageAvailableSemaphore) == VK_SUCCESS, "Failed to create image-available semaphore.");
            EX_FATAL(vkCreateSemaphore(m_pDevice, &semaphoreInfo, nullptr, &m_pFrameObjects[i].renderFinishedSemaphore) == VK_SUCCESS, "Failed to create render-finished semaphore.");
            EX_FATAL(vkCreateFence(m_pDevice, &fenceInfo, nullptr, &m_pFrameObjects[i].inFlightFence) == VK_SUCCESS, "Failed to create in-flight fence.");
        }
    }

    void Renderer::CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamiliesWithSurfaces(m_pSurface, m_pPhysicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        EX_FATAL(vkCreateCommandPool(m_pDevice, &poolInfo, nullptr, &m_pCommandPool) == VK_SUCCESS, "Failed to create command pool.");
    }

    void Renderer::CreateCommandBuffers()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_pCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        for (EXUINT32 i = 0; i < m_pFrameObjects.size(); ++i)
        {
            EX_FATAL(vkAllocateCommandBuffers(m_pDevice, &allocInfo, &m_pFrameObjects[i].commandBuffer) == VK_SUCCESS, "Failed to allocate command buffer for frame object.");
        }
    }

    void Renderer::CreateTextureSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_pPhysicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR; // how to interpolate texels that are magnified
        samplerInfo.minFilter = VK_FILTER_LINEAR; // how to interpolate texels that are minified
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // Repeat the texture when going beyond the image dimensions.
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE; // the texels are addressed using the [0, 1) range on all axes
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        EX_FATAL(vkCreateSampler(m_pDevice, &samplerInfo, nullptr, &m_pTextureSampler) == VK_SUCCESS, "Failed to create texture sampler.");
    }

    void Renderer::CreateDepthResources()
    {
        if (m_Depth != EXN_NULL_HANDLE)
            delete m_Depth;

        m_Depth = new VkTexture(this);
        m_Depth->CreateDepthImage(this->m_szSwapChainExtent, this->FindDepthFormat());
    }

    void Renderer::CreateDefaultTexture()
    {
        m_DefaultTexture = new VkTexture(this);
        m_DefaultTexture->CreateFromImageData(EXN_DUMMY_TEXTURE, 1, 1);
    }

    void Renderer::CreateDescriptorPool()
    {
        constexpr int maxCount = 10;

        // Every material holds MAX_TEXTURE_COUNT samplers per frame in flight; the extra
        // slots are for users of the pool outside the renderer (e.g. ImGui's font texture).
        const EXUINT32 materialSamplerCount = MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT * MAX_MATERIAL_COUNT + MAX_TEXTURE_COUNT;

        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialSamplerCount},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT + MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxCount},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, maxCount},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxCount}};

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * EX_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = static_cast<EXUINT32>(EX_ARRAYSIZE(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;

        EX_FATAL(vkCreateDescriptorPool(m_pDevice, &pool_info, nullptr, &m_pDescriptorPool) == VK_SUCCESS, "Failed to create descriptor pool.");
    }

    void Renderer::CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamiliesWithSurfaces(m_pSurface, m_pPhysicalDevice);
        {
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::vector<EXUINT32> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            if (indices.graphicsFamily)
            {
                m_queueRenderFamily = *indices.graphicsFamily;
            }

            float queuePriority = 1.0f;
            for (EXUINT32 queueFamily : uniqueQueueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            {
                VkPhysicalDeviceDescriptorIndexingFeatures supported{};
                supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

                VkPhysicalDeviceFeatures2 features2{};
                features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features2.pNext = &supported;

                vkGetPhysicalDeviceFeatures2(m_pPhysicalDevice, &features2);

                if (!supported.runtimeDescriptorArray || !supported.shaderSampledImageArrayNonUniformIndexing)
                    throw std::runtime_error("Descriptor indexing not supported on this GPU");
            }

            VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
            indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            indexingFeatures.runtimeDescriptorArray = VK_TRUE;
            indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
            indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.shaderSampledImageArrayDynamicIndexing = indexingFeatures.shaderSampledImageArrayNonUniformIndexing;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<EXUINT32>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.pNext = &indexingFeatures;
            createInfo.enabledExtensionCount = static_cast<EXUINT32>(m_DeviceExtensions.size());
            createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

            if (m_enableValidationLayers)
            {
                createInfo.enabledLayerCount = static_cast<EXUINT32>(m_ValidationLayers.size());
                createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
            }
            else
            {
                createInfo.enabledLayerCount = 0;
            }

            EX_FATAL(vkCreateDevice(m_pPhysicalDevice, &createInfo, nullptr, &m_pDevice) == VK_SUCCESS, "Failed to create logical Vulkan device.");

            vkGetDeviceQueue(m_pDevice, indices.graphicsFamily.value(), 0, &m_pGraphicsQueue);
            vkGetDeviceQueue(m_pDevice, indices.presentFamily.value(), 0, &m_pPresentQueue);
        }
    }

    void Renderer::CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_pPhysicalDevice);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        EXUINT32 imageCount = swapChainSupport.capabilities.minImageCount + 1;

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
        const auto res = vkCreateSwapchainKHR(m_pDevice, &createInfo, nullptr, &m_pSwapChain);
        EX_FATAL(res == VK_SUCCESS, "Failed to create swap chain.");

        vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_szSwapChainExtent = extent;

        for (auto &pipeline : m_pGraphicPipelines)
        {
            // pipeline.second->m_Scissors.clear();
            // pipeline.second->m_Viewports.clear();
            // pipeline.second->m_Scissors.push_back({ {0, 0}, m_szSwapChainExtent });
            // pipeline.second->m_Viewports.push_back({ 0.0f, 0.0f, (float)m_szSwapChainExtent.width, (float)m_szSwapChainExtent.height, 0.0f, 1.0f });
            pipeline.second->SetExtent(m_szSwapChainExtent);
        }
    }

    void Renderer::CreateImageViews()
    {
        m_swapChainImageViews.clear();
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            m_swapChainImageViews[i] = CreateImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void Renderer::CreateRenderPasses()
    {
        // The scene pass clears color and depth, so it has to run first.
        if (GetRenderPass(EXN_SCENE_RENDERPASS) == EXN_NULL_HANDLE)
            m_RenderPasses.insert(m_RenderPasses.begin(), new VkRenderPassObject(this, EXN_SCENE_RENDERPASS, {}));

        for (auto *pass : m_RenderPasses)
        {
            if (pass->GetHandle() == EXN_NULL_HANDLE)
                pass->Create();
        }
    }

    VkRenderPassObject *Renderer::CreateRenderPass(const std::string &name, const VkRenderPassDescription &description)
    {
        if (GetRenderPass(name) != EXN_NULL_HANDLE)
        {
            EX_WARNING("Render pass '%s' already exists.", name.c_str());
            return GetRenderPass(name);
        }

        auto *pass = new VkRenderPassObject(this, name, description);
        m_RenderPasses.push_back(pass);

        // Before Initialize() there is no swap chain yet; CreateRenderPasses() builds it then.
        if (m_pSwapChain != EXN_NULL_HANDLE)
            pass->Create();

        return pass;
    }

    VkRenderPassObject *Renderer::GetRenderPass(const std::string &name)
    {
        for (auto *pass : m_RenderPasses)
        {
            if (pass->GetName() == name)
                return pass;
        }

        return EXN_NULL_HANDLE;
    }

    VkTexture *Renderer::LoadTexture(const char *path)
    {
        if (path == EXN_NULL_HANDLE)
            return EXN_NULL_HANDLE;

        if (VkTexture *texture = GetTexture(path))
            return texture;

        if (!std::filesystem::exists(path))
        {
            EX_WARNING("Texture file not found: %s", path);
            return EXN_NULL_HANDLE;
        }

        auto *texture = new VkTexture(this);
        texture->CreateFromTextureFile(path, path);

        if (texture->m_pView == EXN_NULL_HANDLE)
        {
            EX_WARNING("Failed to load texture: %s", path);
            delete texture;
            return EXN_NULL_HANDLE;
        }

        m_Textures.emplace(path, texture);
        return texture;
    }

    VkTexture *Renderer::CreateTexture(const char *name, const unsigned char *rgba, EXINT32 width, EXINT32 height)
    {
        if (name == EXN_NULL_HANDLE || rgba == EXN_NULL_HANDLE || width <= 0 || height <= 0 || m_pDevice == EXN_NULL_HANDLE)
        {
            EX_WARNING("CreateTexture: invalid arguments, or the renderer is not initialized.");
            return EXN_NULL_HANDLE;
        }

        if (GetTexture(name) != EXN_NULL_HANDLE)
        {
            EX_WARNING("Texture '%s' already exists.", name);
            return GetTexture(name);
        }

        auto *texture = new VkTexture(this);
        texture->CreateFromImageData(rgba, width, height);

        m_Textures.emplace(name, texture);
        return texture;
    }

    VkTexture *Renderer::GetTexture(const char *name)
    {
        const auto it = m_Textures.find(name);
        return it != m_Textures.end() ? it->second : EXN_NULL_HANDLE;
    }

    VkMaterial *Renderer::CreateMaterial(const std::string &name, const std::string &pipeline)
    {
        if (VkMaterial *material = GetMaterial(name))
        {
            EX_WARNING("Material '%s' already exists.", name.c_str());
            return material;
        }

        if (m_Materials.size() >= MAX_MATERIAL_COUNT)
        {
            EX_WARNING("Cannot create material '%s': MAX_MATERIAL_COUNT (%u) reached.", name.c_str(), MAX_MATERIAL_COUNT);
            return EXN_NULL_HANDLE;
        }

        const auto it = m_pGraphicPipelines.find(pipeline);

        if (it == m_pGraphicPipelines.end())
        {
            EX_WARNING("Cannot create material '%s': unknown pipeline '%s'.", name.c_str(), pipeline.c_str());
            return EXN_NULL_HANDLE;
        }

        auto *material = new VkMaterial(this, name, it->second);
        m_Materials.emplace(name, material);

        return material;
    }

    VkMaterial *Renderer::GetMaterial(const std::string &name)
    {
        const auto it = m_Materials.find(name);
        return it != m_Materials.end() ? it->second : EXN_NULL_HANDLE;
    }

    VkMesh *Renderer::CreateMesh(const std::vector<eXngine::Utils::Vertex> &vertices, const std::vector<EXUINT32> &indices)
    {
        if (m_pDevice == EXN_NULL_HANDLE)
        {
            EX_WARNING("CreateMesh: the renderer is not initialized.");
            return EXN_NULL_HANDLE;
        }

        auto *mesh = new VkMesh(this);
        mesh->Create(vertices, indices);

        m_Meshes.push_back(mesh);
        return mesh;
    }

    VkMesh *Renderer::CreateMesh(const eXngine::Utils::Mesh &mesh)
    {
        const std::vector<EXUINT32> indices(mesh.indices.begin(), mesh.indices.end());
        return CreateMesh(mesh.vertices, indices);
    }

    bool Renderer::DestroyTexture(const char *name)
    {
        const auto it = name != EXN_NULL_HANDLE ? m_Textures.find(name) : m_Textures.end();

        if (it == m_Textures.end())
            return false;

        VkTexture *texture = it->second;

        // Frames in flight may still sample it.
        vkDeviceWaitIdle(m_pDevice);

        for (auto &[materialName, material] : m_Materials)
        {
            for (EXUINT32 slot = 0; slot < MAX_TEXTURE_COUNT; ++slot)
            {
                if (material->GetTexture(slot) == texture)
                    material->SetTexture(slot, static_cast<VkTexture *>(EXN_NULL_HANDLE));
            }
        }

        delete texture;
        m_Textures.erase(it);

        return true;
    }

    void Renderer::DestroyMesh(VkMesh *mesh)
    {
        const auto it = std::find(m_Meshes.begin(), m_Meshes.end(), mesh);

        if (it == m_Meshes.end())
            return;

        vkDeviceWaitIdle(m_pDevice);

        delete mesh;
        m_Meshes.erase(it);
    }

    void Renderer::CreateShaders()
    {
        for (const auto &shader : m_Shaders)
        {
            const auto &[name, data] = shader;
            const auto shaderModuleObject = reinterpret_cast<VkShaderModuleObject *>(data);

            VkShaderModule shaderModule = EXN_NULL_HANDLE;

            VkShaderModuleCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = data->code.size(),
                .pCode = reinterpret_cast<const EXUINT32 *>(data->code.data()),
            };

            if (vkCreateShaderModule(m_pDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                EX_ERROR(false, "Failed to create shader module for shader: %s", name.c_str());

                continue;
            }

            shaderModuleObject->m_pShader = shaderModule;
            shaderModuleObject->m_pDevice = m_pDevice;

            const auto stageInfo = shaderModuleObject
                                       ->GetStageCreateInfo(shaderModule, name.c_str(), shaderModuleObject->GetStageFlagBits());

            if (shaderModuleObject->m_pPipeline != EXN_NULL_HANDLE)
                shaderModuleObject->m_pPipeline->m_ShaderStages.push_back(stageInfo);
        }
    }

    void Renderer::CleanupSwapChain()
    {
        for (auto *pass : m_RenderPasses)
        {
            pass->DestroyFramebuffers();
        }

        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_pDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_pDevice, m_pSwapChain, nullptr);
    }

    void Renderer::ResetSwapChain()
    {
        vkDeviceWaitIdle(m_pDevice);
        CleanupSwapChain();
        CreateSwapChain();
        CreateImageViews();
        CreateDepthResources();

        for (auto *pass : m_RenderPasses)
        {
            pass->CreateFramebuffers();
        }
    }

    void Renderer::SelectPhysicalDevice()
    {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        EXUINT32 deviceCount = 0;

        vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, nullptr);
        EX_FATAL(deviceCount > 0, "No Vulkan-compatible physical devices found.");

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

        EX_FATAL(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable physical device.");
        m_pPhysicalDevice = physicalDevice;
    }

    void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(commandBuffer);
    }

    bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        EXUINT32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    bool Renderer::IsDeviceSuitable(VkPhysicalDevice device)
    {
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

        bool isSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                          deviceFeatures.geometryShader && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

        EX_INFO("Device Found: %s", deviceProperties.deviceName);
        EX_INFO("\tDevice ID: %d", deviceProperties.deviceID);
        EX_INFO("\tVendor ID: %d", deviceProperties.vendorID);
        EX_INFO("\tDriver Version: %d.%d.%d",
                EXENGINE_GET_PATCH_VERSION(deviceProperties.driverVersion),
                EXENGINE_GET_MAJOR_VERSION(deviceProperties.driverVersion),
                EXENGINE_GET_MINOR_VERSION(deviceProperties.driverVersion));
        EX_INFO("\tAPI Version: %d.%d.%d",
                EXENGINE_GET_PATCH_VERSION(deviceProperties.apiVersion),
                EXENGINE_GET_MAJOR_VERSION(deviceProperties.apiVersion),
                EXENGINE_GET_MINOR_VERSION(deviceProperties.apiVersion));
        EX_INFO("\tDiscrete GPU: %s", deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Yes" : "No");
        EX_INFO("\tGeometry Shader Support: %s", deviceFeatures.geometryShader ? "Yes" : "No");
        EX_INFO("\tSwap Chain Support: %s", swapChainAdequate ? "Yes" : "No");
        EX_INFO("\tExtensions Supported: %s", extensionsSupported ? "Yes" : "No");
        EX_INFO("\tDevice Selected: %s", isSuitable ? "Yes" : "No");

        return isSuitable;
    }

    SwapChainSupportDetails
    Renderer::QuerySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_pSurface, &details.capabilities);

        EXUINT32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_pSurface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_pSurface, &formatCount, details.formats.data());
        }

        EXUINT32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_pSurface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_pSurface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    QueueFamilyIndices
    Renderer::FindQueueFamiliesWithSurfaces(VkSurfaceKHR surface, VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        EXUINT32 queueFamilyCount = 0;
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
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            i++;
        }

        return indices;
    }

    VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
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

    VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
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

    VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<EXUINT32>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent =
                {
                    static_cast<EXUINT32>(this->m_szFrameBufferSize.x),
                    static_cast<EXUINT32>(this->m_szFrameBufferSize.y)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // void Renderer::LoadModel(const char *path, std::vector<Utils::Mesh> meshes, std::map<const char *, const char *> texturePaths, const char * pipeline)
    //{
    //     m_Models.emplace(path, VkModelObject(meshes, texturePaths, pipeline));
    //     for (const auto &mesh : meshes)
    //     {
    //         m_nVerticesCount += static_cast<EXUINT32>(mesh.vertices.size());
    //         m_nIndicesCount += static_cast<EXUINT32>(mesh.indices.size());
    //     }
    // }

    // void Renderer::LoadModel(const char* path, std::vector<Utils::Mesh> meshes, const char * textureKey, const char* texturePath, const char* pipeline = nullptr)
    //{
    //     m_Models.emplace(path, VkModelObject(meshes, { {textureKey, texturePath} }, pipeline));
    //     for (const auto& mesh : meshes)
    //     {
    //         m_nVerticesCount += static_cast<EXUINT32>(mesh.vertices.size());
    //         m_nIndicesCount += static_cast<EXUINT32>(mesh.indices.size());
    //     }
    // }

    VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
        EX_FATAL(vkCreateImageView(m_pDevice, &viewInfo, nullptr, &imageView) == VK_SUCCESS, "Failed to create image view.");

        return imageView;
    }

    VkFormat Renderer::FindDepthFormat()
    {
        return FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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

        EX_FATAL(false, "No supported format found for depth/stencil attachments.");
        return VkFormat{};
    }

    void Renderer::AddExtension(const char *extension)
    {
        m_Extensions.push_back(extension);
    }

    void Renderer::AddValidationLayer(const char *layer)
    {
        m_ValidationLayers.push_back(layer);
    }

    void Renderer::AddDeviceExtension(const char *extension)
    {
        m_DeviceExtensions.push_back(extension);
    }

    void Renderer::DrawLine(eXvec2 start, eXvec2 end, eXcolor color)
    {
        m_Indices.push_back(static_cast<EXUINT32>(m_Indices.size()));

        m_Vertices.push_back(VkVertex{{.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f},
                                       .coordinates = {start.x, start.y, 0.0f}}});

        m_Vertices.push_back(VkVertex{{.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f},
                                       .coordinates = {end.x, end.y, 0.0f}}});

        m_Indices.push_back(static_cast<EXUINT32>(m_Indices.size()));
    }

    void Renderer::DrawTriangle(eXvec2 pos1, eXvec2 pos2, eXvec2 pos3, eXcolor color)
    {
        const auto indexStart = static_cast<EXUINT32>(m_Indices.size());

        DrawLine(pos1, pos2, color);
        DrawLine(pos2, pos3, color);
        DrawLine(pos3, pos1, color);

        m_Indices.push_back(static_cast<EXUINT32>(m_Indices.size()));
        m_Indices.push_back(indexStart);
    }

    void Renderer::DrawRectangle(eXvec2 pos1, eXvec2 pos2, eXvec2 pos3, eXvec2 pos4, eXcolor color)
    {
        const auto indexStart = static_cast<EXUINT32>(m_Indices.size());

        DrawLine(pos1, pos2, color);
        DrawLine(pos2, pos3, color);
        DrawLine(pos3, pos4, color);
        DrawLine(pos4, pos1, color);

        m_Indices.push_back(static_cast<EXUINT32>(m_Indices.size()));
        m_Indices.push_back(indexStart);
    }

    void Renderer::DrawCircle(eXvec2 center, EXFLOAT radius, eXcolor color)
    {
        // const int segments = 36;
        // const EXFLOAT increment = 2.0f * 3.14159265f / static_cast<EXFLOAT>(segments);
        // EXFLOAT theta = 0.0f;
        // eXvec<float, 2> firstPoint = {center.x + radius * cos(0.0f), center.y + radius * sin(0.0f)};
        // eXvec<float, 2> previousPoint = firstPoint;
        // for (int i = 1; i <= segments; ++i)
        // {
        //     theta += increment;
        //     eXvec<float, 2> newPoint = {center.x + radius * cos(theta), center.y + radius * sin(theta)};
        //     DrawLine(previousPoint, newPoint, color);
        //     previousPoint = newPoint;
        // }
        // DrawLine(previousPoint, firstPoint, color); // Close the circle
    }

    void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1; // no mipmaps levels
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1; // image is not an array

        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            assert(false); // unsupported layout transition
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        EndSingleTimeCommands(commandBuffer);
    }

    void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        EndSingleTimeCommands(commandBuffer);
    }

    void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                               VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional

        // Not inside assert(): the calls would be compiled out in Release builds.
        const VkResult imageResult = vkCreateImage(m_pDevice, &imageInfo, nullptr, &image);
        EX_FATAL(imageResult == VK_SUCCESS, "Failed to create image.");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_pDevice, image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        const VkResult memoryResult = vkAllocateMemory(m_pDevice, &allocInfo, nullptr, &imageMemory);
        EX_FATAL(memoryResult == VK_SUCCESS, "Failed to allocate image memory.");
        vkBindImageMemory(m_pDevice, image, imageMemory, 0);
    }

    VkSurfaceKHR Renderer::CreateSurface(EXVOIDPTR handle)
    {
        VkWin32SurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(NULL),
            .hwnd = static_cast<HWND>(handle),
        };

        vkCreateWin32SurfaceKHR(
            m_pInstance,
            &createInfo,
            nullptr,
            &m_pSurface);

        EX_FATAL(m_pSurface != VK_NULL_HANDLE, "Failed to create window surface.");

        return m_pSurface;
    };

    Renderer::Renderer(const EXCHAR *name) : eXrenderer(name, eXvec2(0, 0)), m_Depth()
    {
        CreateDefaultGraphicsPipeline();
    }

    Renderer::Renderer(const EXCHAR *name, eXvec2 sz) : eXrenderer(name, sz), m_Depth()
    {
        CreateDefaultGraphicsPipeline();
    }
}
#endif