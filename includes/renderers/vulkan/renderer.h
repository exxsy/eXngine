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
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include <eXngine.h>
#include <utils/mesh.h>
#include <types/vector.h>
#include <types/color.h>

#include <renderers/renderer.h>
#include <renderers/vulkan/texture.h>
#include <renderers/vulkan/vertex.h>
#include <renderers/vulkan/mesh.h>
#include <renderers/vulkan/material.h>
#include <renderers/vulkan/renderpass.h>
#include <renderers/vulkan/pipelines/graphics.h>

#undef EXN_NULL_HANDLE
#define EXN_NULL_HANDLE VK_NULL_HANDLE
#define EXN_DEFAULT_PIPELINE "default"
#define EXN_SHAPE_PIPELINE "shapes"

namespace eXngine::Renderers::Vulkan
{
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
	public:
		VkBuffer uniformBuffer;
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
		VkDeviceMemory uniformBuffersMemory;
		VkDescriptorSet descriptorSet;
		EXVOIDPTR uniformBuffersMapped;

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

	class EXNEXPORT Renderer : public eXngine::Renderers::eXrenderer
	{
		friend class VkTexture;
		friend class VkMaterial;
		friend class VkRenderPassObject;

	private:
#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif

		EXINT m_currentFrame = 0;
		EXUINT32 m_queueRenderFamily = 0;
		VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkTexture *m_Depth = nullptr;
		VkTexture *m_DefaultTexture = nullptr;
		VkCommandBuffer m_pCurrentCommandBuffer = EXN_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_pDebugMessenger = EXN_NULL_HANDLE;

		std::vector<VkVertex> m_Vertices;
		std::vector<EXUINT32> m_Indices;
		std::vector<const char *> m_Extensions = {};	   //{"VK_KHR_win32_surface"};
		std::vector<const char *> m_ValidationLayers = {}; //{"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};
		std::vector<const char *> m_DeviceExtensions = {}; //{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME};
		std::vector<VkFrameObject> m_pFrameObjects;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::map<const char *, VkShaderModuleObject *> m_ShaderModules;
		// std::map<const char*, VkModelObject> m_Models;

		VkDescriptorSetLayout m_pGlobalSetLayout = EXN_NULL_HANDLE;	  // set 0: per-frame UBO
		VkDescriptorSetLayout m_pMaterialSetLayout = EXN_NULL_HANDLE; // set 1: material textures
		std::vector<VkRenderPassObject *> m_RenderPasses;			  // recorded in this order
		std::unordered_map<std::string, VkTexture *> m_Textures;
		std::unordered_map<std::string, VkMaterial *> m_Materials;
		std::vector<VkMesh *> m_Meshes;

	private:
		void SelectPhysicalDevice();
		void CreateSurface();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPasses();
		void CreateDefaultGraphicsPipeline();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateTextureSampler();
		void CreateDepthResources();
		void CreateDefaultTexture();
		void CreateDescriptorPool();
		void CreateDescriptorSetLayouts();
		void CreateGlobalDescriptorSets();
		void CreateUniformBuffers();
		void CreateShaders();
		void CleanupSwapChain();
		void ResetSwapChain();
		void DestroyDebugMessenger();
		std::vector<VkDescriptorSetLayout> GetPipelineSetLayouts() const;

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, EXUINT32 imageIndex);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool IsDeviceSuitable(VkPhysicalDevice device);

		void UpdateUniformBuffers();
		EXUINT32 FindMemoryType(EXUINT32 typeFilter, VkMemoryPropertyFlags properties);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	public:
		const EXUINT32 MAX_FRAMES_IN_FLIGHT = 2;
		const EXUINT32 MAX_TEXTURE_COUNT = 16; // texture slots per material
		const EXUINT32 MAX_MATERIAL_COUNT = 64;
		VkCommandBuffer m_pCommandBuffer = EXN_NULL_HANDLE;
		std::unordered_map<std::string, VkGraphicsPipeline *> m_pGraphicPipelines;

		VkExtent2D m_szSwapChainExtent;
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = EXN_NULL_HANDLE;
		VkDescriptorPool m_pDescriptorPool = EXN_NULL_HANDLE;
		VkInstance m_pInstance = EXN_NULL_HANDLE;
		VkQueue m_pGraphicsQueue = EXN_NULL_HANDLE;
		VkQueue m_pPresentQueue = EXN_NULL_HANDLE;
		VkSwapchainKHR m_pSwapChain = EXN_NULL_HANDLE;
		VkCommandPool m_pCommandPool = EXN_NULL_HANDLE;
		VkSurfaceKHR m_pSurface = EXN_NULL_HANDLE;
		VkSampler m_pTextureSampler = EXN_NULL_HANDLE;

		Renderer(const EXCHAR *);
		Renderer(const EXCHAR *, eXvec2);
		
		void CreateInstance();
		void CreateDebugPipeline();

		void Initialize() override;
		void OnRender() override;
		void OnExit() override;

		bool LoadShader(const char *, const std::vector<char> &, ShaderTypes) override;
		void UseShader(const char *) override;
		void DestroyShader(const char *) override;

		void SetSurface(VkSurfaceKHR);
		void SetExtensions(std::vector<const char *>);

		void AddExtension(const char *);
		void AddValidationLayer(const char *);
		void AddDeviceExtension(const char *);

		void DrawLine(eXvec2, eXvec2, eXcolor) override;
		void DrawTriangle(eXvec2, eXvec2, eXvec2, eXcolor) override;
		void DrawRectangle(eXvec2, eXvec2, eXvec2, eXvec2, eXcolor) override;
		void DrawCircle(eXvec2, EXFLOAT, eXcolor) override;

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage &, VkDeviceMemory &);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CopyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
		void TransitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		VkCommandBuffer BeginSingleTimeCommands();
		VkInstance GetVulkanInstance();
		VkFormat FindDepthFormat();
		VkSurfaceKHR CreateSurface(EXVOIDPTR handle);

		// Render passes run in creation order. The "scene" pass (clear color + depth) always
		// exists and is first; passes created before Initialize() are built during it.
		VkRenderPassObject *CreateRenderPass(const std::string &, const VkRenderPassDescription & = {});
		VkRenderPassObject *GetRenderPass(const std::string & = EXN_SCENE_RENDERPASS);

		// Resources below are owned by the renderer and released in OnExit().
		// Textures, materials and meshes need a device: create them after Initialize().
		VkTexture *LoadTexture(const char *path);
		VkTexture *CreateTexture(const char *name, const unsigned char *rgba, EXINT32 width, EXINT32 height);
		VkTexture *GetTexture(const char *name);
		VkTexture *GetDefaultTexture() const { return m_DefaultTexture; }
		VkMaterial *CreateMaterial(const std::string &name, const std::string &pipeline = EXN_DEFAULT_PIPELINE);
		VkMaterial *GetMaterial(const std::string &name);
		VkMesh *CreateMesh(const std::vector<eXngine::Utils::Vertex> &, const std::vector<EXUINT32> &);
		VkMesh *CreateMesh(const eXngine::Utils::Mesh &);

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

		// A pipeline is only usable inside the render pass it was created for
		// (or one with the same attachments).
		template <typename T>
		inline void CreatePipeline(std::string name, const std::string &renderPass = EXN_SCENE_RENDERPASS)
		{
			const auto it = m_pGraphicPipelines.find(name);
			const auto pass = GetRenderPass(renderPass);

			if (it == m_pGraphicPipelines.end() || pass == EXN_NULL_HANDLE)
			{
				EX_WARNING("CreatePipeline: unknown pipeline '%s' or render pass '%s'.", name.c_str(), renderPass.c_str());
				return;
			}

			it->second->CreatePipeline<T>(pass->GetHandle(), GetPipelineSetLayouts());
		}

		static QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkSurfaceKHR, VkPhysicalDevice);
	};
}
#endif