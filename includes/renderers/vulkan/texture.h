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
        Renderer *m_pRenderer = EXN_NULL_HANDLE;

        void CreateDepthImage(VkExtent2D, VkFormat);
        void CreateFromImageData(const unsigned char *, int, int);
        void CreateFromTextureFile(const char *, const char *);
        void Release(VkDevice);

        VkTexture() = default;
        VkTexture(Renderer *);
        ~VkTexture();

        VkImage m_pImage = EXN_NULL_HANDLE;
        VkDeviceMemory m_pDeviceMemory = EXN_NULL_HANDLE;
        VkImageView m_pView = EXN_NULL_HANDLE;

        // Size in pixels (0 for depth images).
        EXINT32 GetWidth() const { return m_nWidth; }
        EXINT32 GetHeight() const { return m_nHeight; }

    private:
        EXINT32 m_nWidth = 0, m_nHeight = 0;

    private:
        VkImageView CreateImageView(VkFormat, VkImageAspectFlags);
    };
}
#endif