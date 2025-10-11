#pragma once

#ifdef _VULKAN
#include <vulkan/vulkan_core.h>

namespace eXngine::Renderers::Vulkan
{
    class Image
    {
    public:
        void CreateDepthImage(VkExtent2D, VkFormat);
        void CreateFromImageData(const unsigned char *, int, int);
        void CreateFromTextureFile(const char *);
        void Release(VkDevice);

        Image();
        Image(VkDevice, VkPhysicalDevice, VkCommandPool, VkQueue);
        ~Image();

        VkImage m_pImage = VK_NULL_HANDLE;
        VkDeviceMemory m_pDeviceMemory = VK_NULL_HANDLE;
        VkImageView m_pView = VK_NULL_HANDLE;

		VkQueue m_pGraphicsQueue = VK_NULL_HANDLE;
		VkCommandPool m_pCommandPool = VK_NULL_HANDLE;
		VkDevice m_pDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_pPhysicalDevice = VK_NULL_HANDLE;
    private:
        void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
        void TransitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
        void CopyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
        VkImageView CreateImageView(VkFormat, VkImageAspectFlags);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer);
        uint32_t FindMemoryType(uint32_t, VkMemoryPropertyFlags);
        void CreateBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
    };
}
#endif