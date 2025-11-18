#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <concepts>
#include <iostream>

#include <gl/GL.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <renderers/base.h>
#include <renderers/vulkan/texture.h>
#include <renderers/vulkan/vertex.h>
#include <glm/glm.hpp>

#include <eXngine.h>
#include <utils/mesh.h>
#include <renderers/vulkan/pipelines/graphics.h>

#undef EXN_NULL_HANDLE
#define EXN_NULL_HANDLE VK_NULL_HANDLE
#define EXN_DEFAULT_PIPELINE "default"

namespace eXngine::Renderers::Vulkan
{
	class Renderer;

	typedef void (*OnUpdateUniformBuffersHandler)(void *, EXUINT32);
	typedef void (*OnRenderHandler)(Renderer *, VkCommandBuffer);

	// template <typename T>
	// concept HasToBeDerivedFromVertex = std::derived_from<T, VkVertex>;

	struct QueueFamilyIndices
	{
		std::optional<EXUINT32> graphicsFamily;
		std::optional<EXUINT32> presentFamily;
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct EXNEXPORT UniformBufferObject
	{
		alignas(16) EXMAT4 model;
		alignas(16) EXMAT4 view;
		alignas(16) EXMAT4 proj;
	};

	struct EXNEXPORT VkFrameObject
	{
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBuffersMemory;
		VkDescriptorSet descriptorSet;
		void *uniformBuffersMapped;

		void Release(VkDevice device)
		{
			vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
			vkDestroyFence(device, inFlightFence, nullptr);
			vkDestroyBuffer(device, uniformBuffer, nullptr);
			vkFreeMemory(device, uniformBuffersMemory, nullptr);
		}
	};

	struct EXNEXPORT VkTextureObject
	{
	public:
		const char path[512];
		VkTexture *texture = EXN_NULL_HANDLE;

		VkTextureObject(const char *p) : path(""), texture(nullptr)
		{
			strncpy_s((char *)path, EX_ARRAYSIZE(path), p, EX_ARRAYSIZE(path) - 1);
		}
	};

	struct EXNEXPORT VkModelObject
	{
	public:
		std::map<const char *, VkTextureObject *> m_pTextures;
		std::vector<eXngine::Utils::Mesh> m_vMeshes;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::string pipeline;

		VkModelObject(std::vector<eXngine::Utils::Mesh> meshes, std::map<const char *, const char *> texturePaths, const char *pipeline = nullptr) : m_vMeshes(meshes)
		{
			for (auto texturePath : texturePaths)
			{
				if (texturePath.first != EXN_NULL_HANDLE && texturePath.second != EXN_NULL_HANDLE)
					m_pTextures.emplace(texturePath.first, new VkTextureObject(texturePath.second));
			}

			for (auto &mesh : m_vMeshes)
			{
				mesh.numTextureCount = static_cast<EXUINT32>(m_pTextures.size());
				mesh.textureIndex = 0;
			}

			if (pipeline != nullptr)
				this->pipeline = pipeline;
		}

		void Release(VkDevice device)
		{
			if (!m_pTextures.empty())
			{
				for (auto &tex : m_pTextures)
				{
					auto &texObj = tex.second;

					if (texObj->texture != EXN_NULL_HANDLE)
					{
						texObj->texture->Release(device);
						delete texObj->texture;
						texObj->texture = EXN_NULL_HANDLE;
					}
					delete texObj;
				}

				m_pTextures.clear();
			}

			m_vMeshes.clear();
		}
	};

	struct EXNEXPORT VkShaderModuleObject : public eXngine::Renderers::ShaderModule
	{
	public:
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkShaderModule m_pShader = EXN_NULL_HANDLE;
		VkGraphicsPipeline *m_pPipeline = EXN_NULL_HANDLE;

		void SetPipeline(VkGraphicsPipeline *);
		VkShaderStageFlagBits GetStageFlagBits() const;

		VkShaderModuleObject(VkDevice, VkShaderModule);
		~VkShaderModuleObject();

		static VkPipelineShaderStageCreateInfo GetStageCreateInfo(VkShaderModule, const char *, VkShaderStageFlagBits);
	};

	enum PrimitiveTypes : std::underlying_type<eXngine::PrimitiveTypes>::type
	{
		Points = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,

		Lines = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,

		Triangles = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,

		PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		MaxEnum = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
	};

	/*enum ShaderTypes : std::underlying_type<eXngine::eXshader>::type
	{
		Points = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,

		Lines = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,

		Triangles = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,

		PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		MaxEnum = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
	};*/

	class EXNEXPORT Renderer : public BaseRenderer, public IRenderCommands<VkBuffer, EXUINT32>
	{
		friend class VkTexture;

	private:
		Size m_frameBufferSize;
		EXINT m_currentFrame = 0;
		EXUINT32 m_nVerticesCount = 0;
		EXUINT32 m_nIndicesCount = 0;
		EXUINT32 m_queueRenderFamily = 0;
		OnUpdateUniformBuffersHandler m_fOnUpdateUniformBuffers;
		OnRenderHandler m_fOnRender;

		std::vector<VkFrameObject> m_pFrameObjects;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		std::map<const char *, VkShaderModuleObject *> m_ShaderModules;
		// std::map<const char*, VkModelObject> m_Models;

		VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkTexture *m_Depth = nullptr;
		VkTexture *m_DefaultTexture = nullptr;
		VkCommandBuffer m_pCurrentCommandBuffer = EXN_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_pDebugMessenger = EXN_NULL_HANDLE;

#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif

		std::vector<const char *> m_Extensions = {};	   //{"VK_KHR_win32_surface"};
		std::vector<const char *> m_ValidationLayers = {}; //{"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};
		std::vector<const char *> m_DeviceExtensions = {}; //{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME};
	private:
		void SelectPhysicalDevice();
		void CreateSurface();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPass();
		void CreateDefaultGraphicsPipeline();
		void CreateGraphicPipelines();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateDescriptorSets();
		void CreateTextureSampler();
		void CreateDepthResources();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateDescriptorPool();
		void CreateUniformBuffers();
		void CreateShaders();
		void CleanupSwapChain();
		void ResetSwapChain();
		void DestroyDebugMessenger();

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, EXUINT32 imageIndex);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool IsDeviceSuitable(VkPhysicalDevice device);

		EXUINT32 FindMemoryType(EXUINT32 typeFilter, VkMemoryPropertyFlags properties);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	public:
		const EXUINT32 MAX_FRAMES_IN_FLIGHT = 2;
		const EXUINT32 MAX_TEXTURE_COUNT = 16;

		void CreateInstance();
		void CreateDebugPipeline();

		// VkGraphicsPipeline* m_pDefaultGraphicsPipeline = EXN_NULL_HANDLE;
		std::unordered_map<std::string, VkGraphicsPipeline *> m_pGraphicPipelines;

		VkExtent2D m_szSwapChainExtent;
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = EXN_NULL_HANDLE;
		VkDescriptorPool m_pDescriptorPool = EXN_NULL_HANDLE;
		VkInstance m_pInstance = EXN_NULL_HANDLE;
		VkQueue m_pGraphicsQueue = EXN_NULL_HANDLE;
		VkQueue m_pPresentQueue = EXN_NULL_HANDLE;
		VkSwapchainKHR m_pSwapChain = EXN_NULL_HANDLE;
		VkRenderPass m_pRenderPass = EXN_NULL_HANDLE;
		VkCommandPool m_pCommandPool = EXN_NULL_HANDLE;
		VkSurfaceKHR m_pSurface = EXN_NULL_HANDLE;
		VkSampler m_pTextureSampler = EXN_NULL_HANDLE;
		VkBuffer m_pVertexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pVertexBufferMemory = EXN_NULL_HANDLE;
		VkBuffer m_pIndexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pIndexBufferMemory = EXN_NULL_HANDLE;

		Renderer(const char *);
		Renderer(const char *, Size);

		void Initialize() override;
		void OnRender() override;
		void OnExit() override;

		bool LoadShader(const char *, const std::vector<char> &, ShaderTypes) override;
		void UseShader(const char *) override;
		void DestroyShader(const char *) override;

		void PushRenderCommand(RenderCommand<VkBuffer, EXUINT32>) override;
		void PopRenderCommand() override;

		void SetSurface(VkSurfaceKHR);
		void SetExtensions(std::vector<const char *>);
		void SetFrameBufferSize(Size);
		void SetUpdateUniformBuffersHandler(OnUpdateUniformBuffersHandler);
		void SetOnRenderHandler(OnRenderHandler);

		void AddExtension(const char *);
		void AddValidationLayer(const char *);
		void AddDeviceExtension(const char *);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CopyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
		void TransitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		VkCommandBuffer BeginSingleTimeCommands();
		VkInstance GetVulkanInstance();
		VkFormat FindDepthFormat();

		template <typename T>
		inline void AllocatePipeline(std::string name)
		{
			if (m_pGraphicPipelines.find(name) != m_pGraphicPipelines.end())
			{
				std::cout << "[WARNING] Pipeline with name '" << name << "' already exists. Skipping creation." << "\n";
				return;
			}

			m_pGraphicPipelines[name] = new T(&m_pDevice, &m_pDescriptorPool, &m_szSwapChainExtent);
		}

		template <typename T>
		inline void CreatePipeline(std::string name)
		{
			const auto pipeline = m_pGraphicPipelines[name]; // new VkGraphicsPipeline(m_pDevice);
			// pipeline->SetExtent(m_szSwapChainExtent);
			// pipeline->m_Scissors.clear();
			// pipeline->m_Viewports.clear();
			// pipeline->m_Scissors.push_back({ {0, 0}, m_szSwapChainExtent });
			// pipeline->m_Viewports.push_back({ 0.0f, 0.0f, (float)m_szSwapChainExtent.width, (float)m_szSwapChainExtent.height, 0.0f, 1.0f });
			pipeline->CreatePipeline<T>(m_pRenderPass);
		}

		static QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkSurfaceKHR, VkPhysicalDevice);
	};
}
#endif