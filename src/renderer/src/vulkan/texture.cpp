#ifndef EXN_DISABLE_VULKAN
#include <cassert>
#include <cstring>
#include <utils/utils.h>
#include <renderers/vulkan/texture.h>
#include <renderers/vulkan/renderer.h>

extern EXINT32 hash(const std::string &);

namespace eXngine::Renderers::Vulkan
{
    void VkTexture::CreateDepthImage(VkExtent2D swapChainExtent, VkFormat depthFormat)
    {
        m_pRenderer->CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_pImage, m_pDeviceMemory);

        m_pView = CreateImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void VkTexture::CreateFromImageData(const unsigned char *imageData, int width, int height)
    {
        const VkDeviceSize imageSize = width * height * 4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        m_pRenderer->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  stagingBuffer, stagingBufferMemory);

        void *data;
        vkMapMemory(m_pRenderer->m_pDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, imageData, imageSize);
        vkUnmapMemory(m_pRenderer->m_pDevice, stagingBufferMemory);

        // create a texture image in GPU memory

        m_pRenderer->CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT /*as a destination for copy from staging buffer*/ | VK_IMAGE_USAGE_SAMPLED_BIT /*as a sampler for shaders*/,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /*Bind to local GPU buffer*/, m_pImage, m_pDeviceMemory);

        m_pRenderer->TransitionImageLayout(m_pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        m_pRenderer->CopyBufferToImage(stagingBuffer, m_pImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        m_pRenderer->TransitionImageLayout(m_pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_pRenderer->m_pDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_pRenderer->m_pDevice, stagingBufferMemory, nullptr);

        m_pView = CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void VkTexture::CreateFromTextureFile(const char *key, const char *texturePath)
    {
        if (this->m_pRenderer->GetImageManager()->LoadImageFromFile(key, texturePath, Images::ImageColorFormat::RGBA))
        {
            const auto image = this->m_pRenderer->GetImageManager()->GetImage(hash(key));
            CreateFromImageData(image->data, image->width, image->height);
        }
    }

    void VkTexture::Release(VkDevice device)
    {
        vkDestroyImageView(device, m_pView, nullptr);
        vkDestroyImage(device, m_pImage, nullptr);
        vkFreeMemory(device, m_pDeviceMemory, nullptr);
        m_pRenderer = EXN_NULL_HANDLE;
    }

    VkImageView VkTexture::CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_pImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        assert(vkCreateImageView(m_pRenderer->m_pDevice, &viewInfo, nullptr, &imageView) == VK_SUCCESS);

        return imageView;
    }

    VkTexture::VkTexture(Renderer * renderer)
    {
        this->m_pRenderer = renderer;
    }

    VkTexture::~VkTexture()
    {
        vkDestroyImageView(m_pRenderer->m_pDevice, m_pView, nullptr);
        vkDestroyImage(m_pRenderer->m_pDevice, m_pImage, nullptr);
    }
}
#endif