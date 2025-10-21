#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <eXngine.h>
#include <vulkan/vulkan_core.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;

    class EXNEXPORT VkTexture
    {
    public:
		Renderer * m_pRenderer = EXN_NULL_HANDLE;

        void CreateDepthImage(VkExtent2D, VkFormat);
        void CreateFromImageData(const unsigned char *, int, int);
        void CreateFromTextureFile(const char *, const char *);
        void Release(VkDevice);

        VkTexture() = default;
        VkTexture(Renderer);
        ~VkTexture();

        VkImage m_pImage = EXN_NULL_HANDLE;
        VkDeviceMemory m_pDeviceMemory = EXN_NULL_HANDLE;
        VkImageView m_pView = EXN_NULL_HANDLE;
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