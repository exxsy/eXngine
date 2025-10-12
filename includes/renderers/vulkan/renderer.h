#pragma once

#ifdef _VULKAN
#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <map>

#include <utils/mesh.h>
#include <gl/GL.h>
#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <renderers/renderer.h>
#include <renderers/vulkan/image.h>
#include <glm/glm.hpp>

namespace eXngine::Renderers::Vulkan
{
	typedef void (*OnUpdateUniformBuffersHandler)(void *, uint32_t);


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

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct VkFrameObject
	{
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBuffersMemory;
		VkDescriptorSet descriptorSet;
		void *uniformBuffersMapped;

		void CleanUp(VkDevice device);
	};

	struct VkImageObject
	{
	public:
		const char path[512];
		Image image;

		VkImageObject(const char *p) : path(""), image()
		{
			// strncpy_s((char*)path, p, EX_ARRAYSIZE(path) - 1);
			strncpy_s((char *)path, EX_ARRAYSIZE(path), p, EX_ARRAYSIZE(path) - 1);
		}
	};

	struct VkModelObject
	{
	public:
		VkImageObject *m_pTexture = nullptr;
		std::vector<eXngine::Utils::Mesh> m_vMeshes;

		VkModelObject(std::vector<eXngine::Utils::Mesh> meshes, const char* texturePath = nullptr) : m_pTexture(nullptr), m_vMeshes(meshes)
		{
			if (texturePath != nullptr) m_pTexture = new VkImageObject(texturePath);
		}

		void CleanUp(VkDevice device)
		{
			if (m_pTexture)
			{
				m_pTexture->image.Release(device);
				delete m_pTexture;
				m_pTexture = nullptr;
			}
		}
	};

	class Renderer : public BaseRenderer
	{
	private:
		Size m_frameBufferSize;
		int m_currentFrame = 0;
		uint32_t m_nVerticesCount = 0;
		uint32_t m_nIndicesCount = 0;
		uint32_t m_queueRenderFamily = 0;

		OnUpdateUniformBuffersHandler m_fOnUpdateUniformBuffers;


		std::vector<VkFrameObject> m_pFrameObjects;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		std::vector<VkShaderModule> m_shaderModules;
		std::map<const char *, VkModelObject> m_Models;

		Image m_Depth;
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
		void SelectPhysicalDevice();
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
		void CreateDescriptorSets();
		void CreateTextureSampler();
		void CreateDepthResources();
		void CreateDescriptorSetLayout();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateDescriptorPool();
		void CreateUniformBuffers();
		void CleanupSwapChain();
		void ResetSwapChain();

		void UpdateUniformBuffer(uint32_t currentImage);

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool IsDeviceSuitable(VkPhysicalDevice device);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkShaderModule CreateShaderModule(const std::vector<char> &code);

	public:
		VkDevice m_pDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = VK_NULL_HANDLE;
		VkInstance m_pInstance = VK_NULL_HANDLE;
		VkQueue m_pGraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_pPresentQueue = VK_NULL_HANDLE;
		VkSwapchainKHR m_pSwapChain = VK_NULL_HANDLE;
		VkPipelineLayout m_pPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass m_pRenderPass = VK_NULL_HANDLE;
		VkPipeline m_pGraphicsPipeline = VK_NULL_HANDLE;
		VkCommandPool m_pCommandPool = VK_NULL_HANDLE;
		VkSurfaceKHR m_pSurface = VK_NULL_HANDLE;
		VkDescriptorPool m_pDescriptorPool = VK_NULL_HANDLE;
		VkSampler m_pTextureSampler = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_pDescriptorSetLayout = VK_NULL_HANDLE;
		VkBuffer m_pVertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_pVertexBufferMemory = VK_NULL_HANDLE;
		VkBuffer m_pIndexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_pIndexBufferMemory = VK_NULL_HANDLE;
		const int MAX_FRAMES_IN_FLIGHT = 2;

		Renderer(const char *);
		Renderer(const char *, Size);
		Renderer(const char *, Size, std::vector<const char *>);
		void Initialize() override;
		void OnRender() override;
		void OnExit() override;
		void SetShaders(std::vector<std::tuple<const char *, VkShaderStageFlagBits, std::vector<char>>>);
		void SetSurface(VkSurfaceKHR);
		void SetExtensions(std::vector<const char *>);
		void SetFrameBufferSize(Size);
		void SetUpdateUniformBuffersHandler(OnUpdateUniformBuffersHandler);
		void LoadModel(const char *, std::vector<Utils::Mesh>, const char * = nullptr);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		static QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkSurfaceKHR, VkPhysicalDevice);
		VkCommandBuffer BeginSingleTimeCommands();
		VkInstance GetVulkanInstance();
		VkFormat FindDepthFormat();
	};
}
#endif