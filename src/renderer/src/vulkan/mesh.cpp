#ifndef EXN_DISABLE_VULKAN
#include <cstring>

#include <renderers/vulkan/mesh.h>
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    VkMesh::VkMesh(Renderer *renderer) : m_pRenderer(renderer) {}

    VkMesh::~VkMesh()
    {
        Release();
    }

    void VkMesh::Create(const std::vector<eXngine::Utils::Vertex> &vertices, const std::vector<EXUINT32> &indices)
    {
        Release();

        if (vertices.empty() || indices.empty())
            return;

        Upload(vertices.data(), sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_pVertexBuffer, m_pVertexBufferMemory);
        Upload(indices.data(), sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_pIndexBuffer, m_pIndexBufferMemory);

        m_nIndexCount = static_cast<EXUINT32>(indices.size());
    }

    void VkMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        const VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_pVertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VkMesh::Release()
    {
        if (m_pRenderer == EXN_NULL_HANDLE || m_pRenderer->m_pDevice == EXN_NULL_HANDLE)
            return;

        const VkDevice device = m_pRenderer->m_pDevice;

        vkDestroyBuffer(device, m_pVertexBuffer, nullptr);
        vkFreeMemory(device, m_pVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, m_pIndexBuffer, nullptr);
        vkFreeMemory(device, m_pIndexBufferMemory, nullptr);

        m_pVertexBuffer = EXN_NULL_HANDLE;
        m_pVertexBufferMemory = EXN_NULL_HANDLE;
        m_pIndexBuffer = EXN_NULL_HANDLE;
        m_pIndexBufferMemory = EXN_NULL_HANDLE;
        m_nIndexCount = 0;
    }

    void VkMesh::Upload(const void *source, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &memory)
    {
        VkBuffer stagingBuffer = EXN_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = EXN_NULL_HANDLE;

        m_pRenderer->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  stagingBuffer, stagingBufferMemory);

        void *data = nullptr;
        vkMapMemory(m_pRenderer->m_pDevice, stagingBufferMemory, 0, size, 0, &data);
        memcpy(data, source, static_cast<size_t>(size));
        vkUnmapMemory(m_pRenderer->m_pDevice, stagingBufferMemory);

        m_pRenderer->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
        m_pRenderer->CopyBuffer(stagingBuffer, buffer, size);

        vkDestroyBuffer(m_pRenderer->m_pDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_pRenderer->m_pDevice, stagingBufferMemory, nullptr);
    }
}
#endif
