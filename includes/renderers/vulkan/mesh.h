#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <vector>

#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <utils/vertex.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;

    // Geometry uploaded to device-local vertex/index buffers. Created via Renderer::CreateMesh.
    class EXNEXPORT VkMesh
    {
    public:
        VkMesh(Renderer *);
        ~VkMesh();

        void Create(const std::vector<eXngine::Utils::Vertex> &, const std::vector<EXUINT32> &);
        void Bind(VkCommandBuffer) const;
        void Release();

        EXUINT32 GetIndexCount() const { return m_nIndexCount; }

    private:
        void Upload(const void *, VkDeviceSize, VkBufferUsageFlags, VkBuffer &, VkDeviceMemory &);

        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        VkBuffer m_pVertexBuffer = EXN_NULL_HANDLE;
        VkDeviceMemory m_pVertexBufferMemory = EXN_NULL_HANDLE;
        VkBuffer m_pIndexBuffer = EXN_NULL_HANDLE;
        VkDeviceMemory m_pIndexBufferMemory = EXN_NULL_HANDLE;
        EXUINT32 m_nIndexCount = 0;
    };
}
#endif
