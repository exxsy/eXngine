#pragma once

#ifdef _VULKAN
#include <Windows.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <gl/GL.h>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <eXngine.h>
#include "renderer.h"

namespace eXngine
{
	namespace Renderers
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;
		};

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities{};
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		class VulkanRenderer : Renderer
		{
		private:
			Size m_frameBufferSize;
			int m_currentFrame = 0;
			int MAX_FRAMES_IN_FLIGHT = 2;

			/* VULKAN */
			VkPhysicalDevice m_pPhysicalDevice = VK_NULL_HANDLE;
			VkInstance m_pInstance = VK_NULL_HANDLE;
			VkDevice m_pDevice = VK_NULL_HANDLE;
			VkQueue m_pGraphicsQueue = VK_NULL_HANDLE;
			VkQueue m_pPresentQueue = VK_NULL_HANDLE;
			VkSwapchainKHR m_pSwapChain = VK_NULL_HANDLE;
			VkDescriptorSetLayout m_pDescriptorSetLayout = VK_NULL_HANDLE;
			VkDescriptorPool m_pDescriptorPool = VK_NULL_HANDLE;
			VkPipelineLayout m_pPipelineLayout = VK_NULL_HANDLE;
			VkRenderPass m_pRenderPass = VK_NULL_HANDLE;
			VkPipeline m_pGraphicsPipeline = VK_NULL_HANDLE;
			VkCommandPool m_pCommandPool = VK_NULL_HANDLE;
			VkSurfaceKHR m_pSurface = VK_NULL_HANDLE;
			
			uint32_t m_queueRenderFamily = 0;
			
			std::vector<VkCommandBuffer> m_pCommandBuffers;
			std::vector<VkSemaphore> m_imageAvailableSemaphores;
			std::vector<VkSemaphore> m_renderFinishedSemaphores;
			std::vector<VkFence> m_inFlightFences;

			std::vector<VkImage> m_swapChainImages;
			std::vector<VkImageView> m_swapChainImageViews;
			std::vector<VkFramebuffer> m_swapChainFramebuffers;
			std::vector<VkShaderModule> m_shaderModules;

			VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
			VkExtent2D m_swapChainExtent{};

#ifdef NDEBUG
			const bool m_enableValidationLayers = false;
#else
			const bool m_enableValidationLayers = true;
#endif

			std::vector<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			std::vector<const char *> m_extensions = {"VK_KHR_win32_surface"};
			std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
			std::vector<const char *> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
			std::vector<std::tuple<const char *, VkShaderStageFlagBits, std::vector<char>>> m_shaders = {};
			/* VULKAN */
		private:
			void CreateInstance(std::vector<const char *>);
			void CreateSurface();
			void CreateLogicalDevice();
			void CreateSwapChain();
			void CreateImageViews();
			void CreateRenderPass();
			void CreateGraphicsPipeline();
			void CreateFramebuffers();
			void CreateCommandPool();
			void CreateCommandBuffers();
			void CreateSyncObjects();

			void SelectPhysicalDevice();
			void ResetSwapChain();
			void CleanupSwapChain();

			void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
			bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
			bool IsDeviceSuitable(VkPhysicalDevice device);
			SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
			QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkPhysicalDevice device);
			VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
			VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
			VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
			VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
			VkFormat FindDepthFormat();
			VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
			VkShaderModule CreateShaderModule(const std::vector<char> &code);

		public:
			VulkanRenderer(const char *);
			VulkanRenderer(const char *, Size);
			VulkanRenderer(const char *, Size, std::vector<const char *>);
			bool Initialize() override;
			void SetShaders(std::vector<std::tuple<const char *, VkShaderStageFlagBits, std::vector<char>>>);
			void OnRender() override;
			void OnExit() override;
			VkInstance GetVulkanInstance();
			void SetSurface(VkSurfaceKHR);
			void SetExtensions(std::vector<const char *>);
			void SetFrameBufferSize(Size);
			void SetMaxFramesInFlight(int);

		};
	}
}
#endif