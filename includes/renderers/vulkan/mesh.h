#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <vector>

#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <utils/vertex.h>

namespace eXngine::Renderers::Vulkan
{
    class Renderer;

    // Geometry for draw calls. Created via Renderer::CreateMesh or CreateDynamicMesh.
    //  - Static meshes are uploaded once to device-local vertex/index buffers (fast to draw).
    //  - Dynamic meshes take new geometry any time with Update(), e.g. a text that changes
    //    every frame. They keep one host-visible buffer pair per frame in flight: the GPU
    //    may still read the buffers of the previous frame while the next one is recorded,
    //    so a frame's buffers are only written in Bind(), when that frame records again
    //    and its earlier use is known to be finished.
    class EXNEXPORT VkMesh
    {
    public:
        VkMesh(Renderer *, EXBOOL dynamic = false);
        ~VkMesh();

        // Static meshes replace their buffers (wait for the GPU first, see Renderer::DestroyMesh).
        // Dynamic meshes keep the data until the next Bind() of every frame in flight.
        void Create(const std::vector<eXngine::Utils::Vertex> &, const std::vector<EXUINT32> &);
        void Update(const std::vector<eXngine::Utils::Vertex> &, const std::vector<EXUINT32> &);
        void Bind(VkCommandBuffer, EXUINT32 frameIndex = 0);
        void Release();

        EXBOOL IsDynamic() const { return m_bDynamic; }
        EXUINT32 GetIndexCount() const;

    private:
        struct Buffer
        {
            VkBuffer Handle = EXN_NULL_HANDLE;
            VkDeviceMemory Memory = EXN_NULL_HANDLE;
            VkDeviceSize Capacity = 0;
            void *Mapped = EXN_NULL_HANDLE;
        };

        // The buffers of one frame in flight, and the version of the data they hold.
        struct FrameBuffers
        {
            Buffer Vertices, Indices;
            EXUINT32 IndexCount = 0;
            EXUINT64 Version = 0;
        };

        void Upload(const void *, VkDeviceSize, VkBufferUsageFlags, VkBuffer &, VkDeviceMemory &);
        void Write(Buffer &, const void *, VkDeviceSize, VkBufferUsageFlags);
        void ReleaseBuffer(Buffer &);

        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        EXBOOL m_bDynamic = false;

        // Static
        VkBuffer m_pVertexBuffer = EXN_NULL_HANDLE;
        VkDeviceMemory m_pVertexBufferMemory = EXN_NULL_HANDLE;
        VkBuffer m_pIndexBuffer = EXN_NULL_HANDLE;
        VkDeviceMemory m_pIndexBufferMemory = EXN_NULL_HANDLE;
        EXUINT32 m_nIndexCount = 0;

        // Dynamic: the latest data, copied to a frame's buffers when that frame binds the mesh.
        std::vector<eXngine::Utils::Vertex> m_Vertices;
        std::vector<EXUINT32> m_Indices;
        EXUINT64 m_nVersion = 0;
        std::vector<FrameBuffers> m_Frames;
    };
}
#endif
