#ifndef EXN_DISABLE_VULKAN
#include <cstring>

#include <renderers/vulkan/mesh.h>
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    VkMesh::VkMesh(Renderer *renderer, EXBOOL dynamic) : m_pRenderer(renderer), m_bDynamic(dynamic)
    {
        if (m_bDynamic)
            m_Frames.resize(m_pRenderer->MAX_FRAMES_IN_FLIGHT);
    }

    VkMesh::~VkMesh()
    {
        Release();
    }

    void VkMesh::Create(const std::vector<eXngine::Utils::Vertex> &vertices, const std::vector<EXUINT32> &indices)
    {
        if (m_bDynamic)
        {
            Update(vertices, indices);
            return;
        }

        Release();

        if (vertices.empty() || indices.empty())
            return;

        Upload(vertices.data(), sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_pVertexBuffer, m_pVertexBufferMemory);
        Upload(indices.data(), sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_pIndexBuffer, m_pIndexBufferMemory);

        m_nIndexCount = static_cast<EXUINT32>(indices.size());
    }

    void VkMesh::Update(const std::vector<eXngine::Utils::Vertex> &vertices, const std::vector<EXUINT32> &indices)
    {
        if (!m_bDynamic)
        {
            Create(vertices, indices);
            return;
        }

        m_Vertices = vertices;
        m_Indices = indices;
        ++m_nVersion;
    }

    EXUINT32 VkMesh::GetIndexCount() const
    {
        return m_bDynamic ? static_cast<EXUINT32>(m_Indices.size()) : m_nIndexCount;
    }

    void VkMesh::Bind(VkCommandBuffer commandBuffer, EXUINT32 frameIndex)
    {
        const VkDeviceSize offset = 0;

        if (!m_bDynamic)
        {
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_pVertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            return;
        }

        FrameBuffers &frame = m_Frames[frameIndex % m_Frames.size()];

        // The renderer waited for this frame's fence before recording, so the GPU is done
        // with these buffers and they can be rewritten (or replaced by bigger ones).
        if (frame.Version != m_nVersion)
        {
            if (!m_Indices.empty())
            {
                Write(frame.Vertices, m_Vertices.data(), sizeof(m_Vertices[0]) * m_Vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                Write(frame.Indices, m_Indices.data(), sizeof(m_Indices[0]) * m_Indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            frame.IndexCount = static_cast<EXUINT32>(m_Indices.size());
            frame.Version = m_nVersion;
        }

        if (frame.IndexCount == 0)
            return;

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &frame.Vertices.Handle, &offset);
        vkCmdBindIndexBuffer(commandBuffer, frame.Indices.Handle, 0, VK_INDEX_TYPE_UINT32);
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

        for (auto &frame : m_Frames)
        {
            ReleaseBuffer(frame.Vertices);
            ReleaseBuffer(frame.Indices);
            frame = {};
        }

        m_Vertices.clear();
        m_Indices.clear();
        ++m_nVersion;
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

    void VkMesh::Write(Buffer &buffer, const void *source, VkDeviceSize size, VkBufferUsageFlags usage)
    {
        if (size > buffer.Capacity)
        {
            ReleaseBuffer(buffer);

            // Grows in powers of two, so a text that keeps getting longer reallocates rarely.
            VkDeviceSize capacity = 256;

            while (capacity < size)
                capacity *= 2;

            // Host-visible and coherent: the CPU writes it directly, no staging copy needed.
            m_pRenderer->CreateBuffer(capacity, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      buffer.Handle, buffer.Memory);
            vkMapMemory(m_pRenderer->m_pDevice, buffer.Memory, 0, capacity, 0, &buffer.Mapped);
            buffer.Capacity = capacity;
        }

        memcpy(buffer.Mapped, source, static_cast<size_t>(size));
    }

    void VkMesh::ReleaseBuffer(Buffer &buffer)
    {
        const VkDevice device = m_pRenderer->m_pDevice;

        // Freeing the memory also unmaps it.
        vkDestroyBuffer(device, buffer.Handle, nullptr);
        vkFreeMemory(device, buffer.Memory, nullptr);
        buffer = {};
    }
}
#endif
