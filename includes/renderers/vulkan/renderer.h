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

#include <eXngine.h>
#include <utils/mesh.h>
#include <gl/GL.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <renderers/base.h>
#include <renderers/vulkan/texture.h>
#include <renderers/vulkan/vertex.h>
#include <glm/glm.hpp>

#undef EXN_NULL_HANDLE
#define EXN_NULL_HANDLE VK_NULL_HANDLE

namespace eXngine::Renderers::Vulkan
{
	class Renderer;

	typedef void (*OnUpdateUniformBuffersHandler)(void *, EXUINT32);
	typedef void (*OnRenderHandler)(Renderer*, VkCommandBuffer);

	//template <typename T>
	//concept HasToBeDerivedFromVertex = std::derived_from<T, VkVertex>;

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
	
	struct VkModelPushConstants
	{
	public:
		alignas(4) EXUINT32 textureIndex = 0;
		alignas(4) EXUINT32 numTextures = 1;
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
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::string shader;

		VkModelObject(std::vector<eXngine::Utils::Mesh> meshes, std::map<const char *, const char *> texturePaths, const char * pipeline = nullptr) : m_vMeshes(meshes)
		{
			for (auto texturePath : texturePaths)
			{
				if (texturePath.first != EXN_NULL_HANDLE && texturePath.second != EXN_NULL_HANDLE) 
					m_pTextures.emplace(texturePath.first, new VkTextureObject(texturePath.second));
			}

			for (auto& mesh : m_vMeshes)
			{
				mesh.numTextureCount = static_cast<EXUINT32>(m_pTextures.size());
				mesh.textureIndex = 0;
			}

			if (pipeline != nullptr) shader = pipeline;
		}

		void Release(VkDevice device)
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

	struct VkGraphicsPipeline
	{
	public:
		const EXUINT32 MAX_FRAMES_IN_FLIGHT = 2;
		const EXUINT32 MAX_TEXTURE_COUNT = 16;

		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkPipelineLayout m_pLayout = EXN_NULL_HANDLE;
		VkPipeline m_pPipeline = EXN_NULL_HANDLE;
		VkDescriptorPool m_pDescriptorPool = EXN_NULL_HANDLE;
		VkDescriptorSetLayout m_pDescriptorSetLayout = EXN_NULL_HANDLE;
		VkExtent2D m_pExtent = { 0, 0 };
		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
		std::vector<VkViewport> m_Viewports;
		std::vector<VkRect2D> m_Scissors;
		std::vector<VkDynamicState> states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkGraphicsPipeline(VkDevice, VkExtent2D);
		~VkGraphicsPipeline();

		void SetExtent(VkExtent2D);
		void AddViewport(VkViewport);
		void AddScissor(VkRect2D);
		void SetDynamicStates(std::vector<VkDynamicState>);

		void CreateDescriptorSetLayout();
		void CreateDescriptorPool();
		//void CreateDescriptorSets();

		template <std::derived_from<VkVertex> T>
		void CreatePipeline(VkRenderPass renderPass)
		{
			auto bindingDescription = T::GetBindingDescription();
			auto attributeDescriptions = T::GetAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<EXUINT32>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<EXUINT32>(states.size());
			dynamicState.pDynamicStates = states.data();

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = static_cast<EXUINT32>(m_Viewports.size());
			viewportState.pViewports = m_Viewports.data();
			viewportState.scissorCount = static_cast<EXUINT32>(m_Scissors.size());
			viewportState.pScissors = m_Scissors.data();

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_NONE;
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

			VkPushConstantRange debugViewPushConstants{};
			debugViewPushConstants.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			debugViewPushConstants.offset = sizeof(VkModelPushConstants) * 0;
			debugViewPushConstants.size = sizeof(VkModelPushConstants);

			//std::vector<VkPushConstantRange> pushConstants {
			//    //{
			//    //    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			//    //    .offset = sizeof(float) * 0,
			//    //    .size = sizeof(float) * 4,
			//    //},
			//    {
			//        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			//        .offset = sizeof(VkModelPushConstants) * 1,
			//        .size = sizeof(VkModelPushConstants) * 1,
			//    }
			//};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;                            // Optional
			pipelineLayoutInfo.pSetLayouts = &m_pDescriptorSetLayout;         // Optional
			pipelineLayoutInfo.pushConstantRangeCount = 1;// static_cast<EXUINT32>(pushConstants.size());                    // Optional
			pipelineLayoutInfo.pPushConstantRanges = &debugViewPushConstants;// pushConstants.data(); // Optional

			assert(vkCreatePipelineLayout(m_pDevice, &pipelineLayoutInfo, nullptr, &this->m_pLayout) == VK_SUCCESS);

			VkPipelineDepthStencilStateCreateInfo depthStencil{};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // lower depth = closer
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.minDepthBounds = 0.0f; // Optional
			depthStencil.maxDepthBounds = 1.0f; // Optional
			depthStencil.stencilTestEnable = VK_FALSE;
			depthStencil.front = {}; // Optional
			depthStencil.back = {};  // Optional

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

			assert(vkCreateGraphicsPipelines(m_pDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->m_pPipeline) == VK_SUCCESS);
		}
	};

	struct VkShaderModuleObject : public eXngine::Renderers::ShaderModule
	{
	public:
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkShaderModule m_pShader = EXN_NULL_HANDLE;
		VkGraphicsPipeline* m_pPipeline = EXN_NULL_HANDLE;

		void SetPipeline(VkGraphicsPipeline*);
		VkShaderStageFlagBits GetStageFlagBits() const;

		VkShaderModuleObject(VkDevice, VkShaderModule);
		~VkShaderModuleObject();

		static VkPipelineShaderStageCreateInfo GetStageCreateInfo(VkShaderModule, const char*, VkShaderStageFlagBits);
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

	//enum ShaderTypes : std::underlying_type<eXngine::eXshader>::type
	//{
	//	Points = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	//	PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,

	//	Lines = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	//	LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	//	LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	//	LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
	//	LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,

	//	Triangles = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	//	TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	//	TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
	//	TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
	//	TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,

	//	PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
	//	MaxEnum = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
	//};

	class Renderer : public BaseRenderer, public IRenderCommands<VkBuffer, EXUINT32>
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
		std::map<const char*, VkShaderModuleObject*> m_ShaderModules;
		std::map<const char*, VkModelObject> m_Models;

		VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkTexture * m_Depth = nullptr;
		VkTexture * m_DefaultTexture = nullptr;
		VkCommandBuffer m_pCurrentCommandBuffer = EXN_NULL_HANDLE;

#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif

		std::vector<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		std::vector<const char *> m_extensions = {"VK_KHR_win32_surface"};
		std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};
		std::vector<const char *> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
		//std::vector<std::tuple<const char *, VkShaderStageFlagBits, std::vector<char>>> m_shaders = {};
		/* VULKAN */
	private:
		void SelectPhysicalDevice();
		void CreateInstance(std::vector<const char *>);
		void CreateSurface();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPass();
		void CreateDefaultGraphicsPipeline();
		//void CreateGraphicsPipeline();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateDescriptorSets();
		void CreateTextureSampler();
		void CreateDepthResources();
		//void CreateDescriptorSetLayout();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		//void CreateDescriptorPool();
		void CreateUniformBuffers();
		void CreateShaders();
		void CleanupSwapChain();
		void ResetSwapChain();

		void RecordCommandBuffer(EXUINT32 imageIndex);
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

		VkGraphicsPipeline* m_pDefaultGraphicsPipeline = EXN_NULL_HANDLE;
		std::unordered_map<std::string, VkGraphicsPipeline *> m_pGraphicPipelines;
		
		/*
		VkPipeline m_pGraphicsPipeline = EXN_NULL_HANDLE;
		VkPipelineLayout m_pPipelineLayout = EXN_NULL_HANDLE;*/

		VkExtent2D m_szSwapChainExtent;
		VkDevice m_pDevice = EXN_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = EXN_NULL_HANDLE;
		VkInstance m_pInstance = EXN_NULL_HANDLE;
		VkQueue m_pGraphicsQueue = EXN_NULL_HANDLE;
		VkQueue m_pPresentQueue = EXN_NULL_HANDLE;
		VkSwapchainKHR m_pSwapChain = EXN_NULL_HANDLE;
		VkRenderPass m_pRenderPass = EXN_NULL_HANDLE;
		VkCommandPool m_pCommandPool = EXN_NULL_HANDLE;
		VkSurfaceKHR m_pSurface = EXN_NULL_HANDLE;
		//VkDescriptorPool m_pDescriptorPool = EXN_NULL_HANDLE;
		//VkDescriptorSetLayout m_pDescriptorSetLayout = EXN_NULL_HANDLE;
		VkSampler m_pTextureSampler = EXN_NULL_HANDLE;
		VkBuffer m_pVertexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pVertexBufferMemory = EXN_NULL_HANDLE;
		VkBuffer m_pIndexBuffer = EXN_NULL_HANDLE;
		VkDeviceMemory m_pIndexBufferMemory = EXN_NULL_HANDLE;

		Renderer(const char *);
		Renderer(const char *, Size);
		Renderer(const char *, Size, std::vector<const char *>);

		void Initialize() override;
		void OnRender() override;
		void OnExit() override;

		bool LoadShader(const char*, const std::vector<char>&, eXshader) override;
		void UseShader(const char*) override;
		void DestroyShader(const char*) override;

		void PushRenderCommand(RenderCommand<VkBuffer, EXUINT32>) override;
		void PopRenderCommand() override;

		void SetSurface(VkSurfaceKHR);
		void SetExtensions(std::vector<const char *>);
		void SetFrameBufferSize(Size);
		void SetUpdateUniformBuffersHandler(OnUpdateUniformBuffersHandler);
		void SetOnRenderHandler(OnRenderHandler);

		//void LoadModel(const char*, std::vector<Utils::Mesh>, const char*, const char*, const char* = nullptr);
		void LoadModel(const char *, std::vector<Utils::Mesh>, std::map<const char*, const char *>, const char* = nullptr);

		/*template <std::derived_from<VkVertex> T>
		void CreatePipeline(std::string);*/

		template<std::derived_from<VkVertex> T>
		inline void CreatePipeline(std::string name)
		{
			const auto pipeline = m_pGraphicPipelines[name];//new VkGraphicsPipeline(m_pDevice);
			pipeline->SetExtent(m_szSwapChainExtent);
			pipeline->CreatePipeline<T>(m_pRenderPass);
		}

		void AllocatePipeline(std::string);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		static QueueFamilyIndices FindQueueFamiliesWithSurfaces(VkSurfaceKHR, VkPhysicalDevice);
		VkCommandBuffer BeginSingleTimeCommands();
		VkInstance GetVulkanInstance();
		VkFormat FindDepthFormat();
		//VkShaderModule CreateShaderModule(const std::vector<char>& code);

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