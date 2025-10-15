#pragma once

#ifdef EXN_USE_VULKAN
#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <type_traits>

#include <utils/mesh.h>
#include <gl/GL.h>
#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <renderers/renderer.h>
#include <renderers/vulkan/texture.h>
#include <renderers/vulkan/vertex.h>
#include <glm/glm.hpp>

#undef EXN_NULL_HANDLE
#define EXN_NULL_HANDLE VK_NULL_HANDLE

namespace eXngine::Renderers::Vulkan
{
	typedef void (*OnUpdateUniformBuffersHandler)(void *, EXUINT32);
	typedef void (*OnRenderHandler)(BaseRenderer*, VkCommandBuffer);

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

	struct UniformBufferObject
	{
		alignas(16) EXMAT4 model;
		alignas(16) EXMAT4 view;
		alignas(16) EXMAT4 proj;
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

		void CleanUp(VkDevice device) 
		{
			vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
			vkDestroyFence(device, inFlightFence, nullptr);
			vkDestroyBuffer(device, uniformBuffer, nullptr);
			vkFreeMemory(device, uniformBuffersMemory, nullptr);
		}
	};

	struct VkTextureObject
	{
	public:
		const char path[512];
		VkTexture * texture = EXN_NULL_HANDLE;

		VkTextureObject(const char *p) : path(""), texture(nullptr)
		{
			strncpy_s((char *)path, EX_ARRAYSIZE(path), p, EX_ARRAYSIZE(path) - 1);
		}
	};

	struct VkModelObject
	{
	public:
		std::map<const char *, VkTextureObject*> m_pTextures;
		std::vector<eXngine::Utils::Mesh> m_vMeshes;
		std::vector<VkDescriptorSet> descriptorSets;

		VkModelObject(std::vector<eXngine::Utils::Mesh> meshes, std::map<const char *, const char *> texturePaths) : m_vMeshes(meshes)
		{
			for (auto texturePath : texturePaths)
			{
				if (texturePath.first != EXN_NULL_HANDLE && texturePath.second != EXN_NULL_HANDLE) 
					m_pTextures.emplace(texturePath.first, new VkTextureObject(texturePath.second));
			}
		}

		void CleanUp(VkDevice device)
		{
			if (!m_pTextures.empty())
			{
				for (auto& tex : m_pTextures)
				{
					auto& texObj = tex.second;

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

	class Renderer : public BaseRenderer
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
		std::vector<VkShaderModule> m_shaderModules;
		std::map<const char *, VkModelObject> m_Models;

		VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkTexture * m_Depth = nullptr;
		VkTexture * m_DefaultTexture = nullptr;

#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif

		std::vector<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		std::vector<const char *> m_extensions = {"VK_KHR_win32_surface"};
		std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};
		std::vector<const char *> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
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
		VkShaderModule CreateShaderModule(const std::vector<char> &code);

	public:
		VkExtent2D m_szSwapChainExtent;
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = EXN_NULL_HANDLE;
		VkInstance m_pInstance = EXN_NULL_HANDLE;
		VkQueue m_pGraphicsQueue = EXN_NULL_HANDLE;
		VkQueue m_pPresentQueue = EXN_NULL_HANDLE;
		VkSwapchainKHR m_pSwapChain = EXN_NULL_HANDLE;
		VkPipelineLayout m_pPipelineLayout = EXN_NULL_HANDLE;
		VkRenderPass m_pRenderPass = EXN_NULL_HANDLE;
		VkPipeline m_pGraphicsPipeline = EXN_NULL_HANDLE;
		VkCommandPool m_pCommandPool = EXN_NULL_HANDLE;
		VkSurfaceKHR m_pSurface = EXN_NULL_HANDLE;
		VkDescriptorPool m_pDescriptorPool = EXN_NULL_HANDLE;
		VkSampler m_pTextureSampler = EXN_NULL_HANDLE;
		VkDescriptorSetLayout m_pDescriptorSetLayout = EXN_NULL_HANDLE;
		VkBuffer m_pVertexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pVertexBufferMemory = EXN_NULL_HANDLE;
		VkBuffer m_pIndexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pIndexBufferMemory = EXN_NULL_HANDLE;
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
		void SetOnRenderHandler(OnRenderHandler);

		void LoadModel(const char*, std::vector<Utils::Mesh>, const char*, const char*);
		void LoadModel(const char *, std::vector<Utils::Mesh>, std::map<const char*, const char *>);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		static QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkSurfaceKHR, VkPhysicalDevice);
		VkCommandBuffer BeginSingleTimeCommands();
		VkInstance GetVulkanInstance();
		VkFormat FindDepthFormat();

		// Base drawing functions
		//void BeginFrame() override;
		//void EndFrame() override;

		//void DrawLine(const EXVEC3& start, const EXVEC3& end, const EXVEC3& color = EXVEC3(1.0f)) override;
		//void DrawTriangle(const EXVEC3& v1, const EXVEC3& v2, const EXVEC3& v3, const EXVEC3& color = EXVEC3(1.0f)) override;
		//void DrawQuad(const EXVEC3& position, const EXVEC2& size, const EXVEC3& color = EXVEC3(1.0f)) override;

		//void BeginBatch(eXngine::PrimitiveTypes type) override;
		//void AddVertex(const Utils::Vertex& vertex) override;
		//void EndBatch() override;

		//EXUINT32 CreateTexture(const void* data, EXUINT32 width, EXUINT32 height) override;
		//void DeleteTexture(EXUINT32 textureId) override;
		//void BindTexture(EXUINT32 textureId) override;

		//void SetViewport(int x, int y, int width, int height) override;
		//void EnableDepthTest(bool enable) override;
		//void EnableBlending(bool enable) override;
		//
		//void PushMatrix() override;
		//void PopMatrix() override;
		//void Translate(const EXVEC3& offset) override;
		//void Rotate(float angle, const EXVEC3& axis) override;
		//void Scale(const EXVEC3& scale) override;
	};
}
#endif