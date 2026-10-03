#ifndef EXN_DISABLE_VULKAN
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

#include <renderers/vulkan/canvas.h>
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    namespace
    {
        constexpr EXFLOAT kPi = std::numbers::pi_v<EXFLOAT>;
        constexpr VkDeviceSize kInitialVertices = 4096;

        EXINT32 SegmentsFor(EXFLOAT radius)
        {
            return std::clamp(static_cast<EXINT32>(radius * 0.6f) + 12, 16, 96);
        }

        EXBOOL SameClip(const VkRect2D &a, const VkRect2D &b)
        {
            return a.offset.x == b.offset.x && a.offset.y == b.offset.y && a.extent.width == b.extent.width &&
                   a.extent.height == b.extent.height;
        }
    }

    // ---- Vertex format -------------------------------------------------------------

    VkVertexInputBindingDescription VkCanvasVertex::GetBindingDescription()
    {
        return {.binding = 0, .stride = sizeof(VkCanvasVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    }

    std::vector<VkVertexInputAttributeDescription> VkCanvasVertex::GetAttributeDescriptions()
    {
        return {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(VkCanvasVertex, x)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(VkCanvasVertex, u)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(VkCanvasVertex, color)},
        };
    }

    // ---- Setup -----------------------------------------------------------------------

    void VkCanvas::Prepare(Renderer *renderer, const std::vector<char> &vertexSpirv, const std::vector<char> &fragmentSpirv)
    {
        renderer->AllocatePipeline<VkCanvasPipeline>(EXN_CANVAS_PIPELINE);
        renderer->LoadShader(EXN_CANVAS_PIPELINE ".vertex", vertexSpirv, eXshader_Vertex);
        renderer->LoadShader(EXN_CANVAS_PIPELINE ".fragment", fragmentSpirv, eXshader_Fragment);
    }

    VkCanvas::VkCanvas(Renderer *renderer) : m_pRenderer(renderer) {}

    VkCanvas::~VkCanvas() { Release(); }

    void VkCanvas::Create()
    {
        VkRenderPassObject *pass = m_pRenderer->CreateRenderPass(EXN_CANVAS_RENDERPASS, {
                                                                                             .colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                             .useDepth = false,
                                                                                         });
        m_pRenderer->CreatePipeline<VkCanvasVertex>(EXN_CANVAS_PIPELINE, EXN_CANVAS_RENDERPASS);
        m_pDefault = m_pRenderer->m_pGraphicPipelines[EXN_CANVAS_PIPELINE];
        m_pPipeline = m_pDefault;

        const unsigned char white[4] = {255, 255, 255, 255};
        m_pWhite = m_pRenderer->CreateTexture("canvas:white", white, 1, 1);

        m_Frames.resize(m_pRenderer->MAX_FRAMES_IN_FLIGHT + 1);
        pass->SetRecordHandler([this](VkCommandBuffer commandBuffer, EXUINT32 frameIndex)
                               { Record(commandBuffer, frameIndex); });
    }

    void VkCanvas::Release()
    {
        if (m_pRenderer == EXN_NULL_HANDLE || m_pRenderer->m_pDevice == EXN_NULL_HANDLE)
            return;

        if (VkRenderPassObject *pass = m_pRenderer->GetRenderPass(EXN_CANVAS_RENDERPASS))
            pass->SetRecordHandler(nullptr);

        for (FrameBuffers &frame : m_Frames)
        {
            if (frame.Vertices != EXN_NULL_HANDLE || frame.Indices != EXN_NULL_HANDLE)
                vkDeviceWaitIdle(m_pRenderer->m_pDevice);
            for (auto [buffer, memory] : {std::pair{frame.Vertices, frame.VertexMemory}, std::pair{frame.Indices, frame.IndexMemory}})
            {
                if (buffer != EXN_NULL_HANDLE)
                    vkDestroyBuffer(m_pRenderer->m_pDevice, buffer, nullptr);
                if (memory != EXN_NULL_HANDLE)
                    vkFreeMemory(m_pRenderer->m_pDevice, memory, nullptr);
            }
            frame = {};
        }
        m_Frames.clear();
    }

    // ---- State -----------------------------------------------------------------------

    void VkCanvas::Begin()
    {
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        m_Clips.clear();
        m_Transform = Screen();
        m_pPipeline = m_pDefault;
        m_Params[0] = m_Params[1] = EXMATH::vec4(0.0f);
    }

    EXMATH::mat4 VkCanvas::Screen() const
    {
        // Pixels (0..width, 0..height, y down) -> clip space (-1..1, Vulkan's y also points down).
        const VkExtent2D extent = m_pRenderer->m_szSwapChainExtent;
        EXMATH::mat4 m(1.0f);
        m[0][0] = 2.0f / static_cast<EXFLOAT>(std::max(extent.width, 1u));
        m[1][1] = 2.0f / static_cast<EXFLOAT>(std::max(extent.height, 1u));
        m[3] = EXMATH::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
        return m;
    }

    void VkCanvas::SetTransform(const EXMATH::mat4 &transform) { m_Transform = transform; }

    void VkCanvas::PushClip(const VkCanvasRect &rect)
    {
        VkCanvasRect clip = rect;
        if (!m_Clips.empty())  // nested clips only shrink
        {
            const VkCanvasRect &outer = m_Clips.back();
            const EXFLOAT x0 = std::max(clip.x, outer.x), y0 = std::max(clip.y, outer.y);
            const EXFLOAT x1 = std::min(clip.x + clip.width, outer.x + outer.width);
            const EXFLOAT y1 = std::min(clip.y + clip.height, outer.y + outer.height);
            clip = {x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
        }
        m_Clips.push_back(clip);
    }

    void VkCanvas::PopClip()
    {
        if (!m_Clips.empty())
            m_Clips.pop_back();
    }

    void VkCanvas::SetPipeline(VkGraphicsPipeline *pipeline, const EXMATH::vec4 &params0, const EXMATH::vec4 &params1)
    {
        m_pPipeline = pipeline != EXN_NULL_HANDLE ? pipeline : m_pDefault;
        m_Params[0] = params0;
        m_Params[1] = params1;
    }

    VkRect2D VkCanvas::CurrentClip() const
    {
        const VkExtent2D extent = m_pRenderer->m_szSwapChainExtent;
        if (m_Clips.empty())
            return {{0, 0}, extent};
        const VkCanvasRect &c = m_Clips.back();
        const EXINT32 x0 = std::clamp(static_cast<EXINT32>(std::floor(c.x)), 0, static_cast<EXINT32>(extent.width));
        const EXINT32 y0 = std::clamp(static_cast<EXINT32>(std::floor(c.y)), 0, static_cast<EXINT32>(extent.height));
        const EXINT32 x1 = std::clamp(static_cast<EXINT32>(std::ceil(c.x + c.width)), x0, static_cast<EXINT32>(extent.width));
        const EXINT32 y1 = std::clamp(static_cast<EXINT32>(std::ceil(c.y + c.height)), y0, static_cast<EXINT32>(extent.height));
        return {{x0, y0}, {static_cast<EXUINT32>(x1 - x0), static_cast<EXUINT32>(y1 - y0)}};
    }

    VkMaterial *VkCanvas::MaterialFor(VkTexture *texture)
    {
        if (const auto it = m_Materials.find(texture); it != m_Materials.end())
            return it->second;

        VkMaterial *material = EXN_NULL_HANDLE;
        if (!m_FreeMaterials.empty())
        {
            material = m_FreeMaterials.back();
            m_FreeMaterials.pop_back();
        }
        else
        {
            material = m_pRenderer->CreateMaterial("canvas:" + std::to_string(m_nMaterialCount++), EXN_CANVAS_PIPELINE);
            if (material == EXN_NULL_HANDLE)  // out of materials: draw it white rather than not at all
                return texture == m_pWhite ? EXN_NULL_HANDLE : MaterialFor(m_pWhite);
        }
        material->SetTexture(0, texture);
        m_Materials.emplace(texture, material);
        return material;
    }

    void VkCanvas::Forget(VkTexture *texture)
    {
        const auto it = m_Materials.find(texture);
        if (it == m_Materials.end())
            return;
        it->second->SetTexture(0, m_pWhite);
        m_FreeMaterials.push_back(it->second);
        m_Materials.erase(it);
    }

    EXUINT32 VkCanvas::Reserve(VkTexture *texture, EXUINT32 vertices)
    {
        VkMaterial *material = MaterialFor(texture != EXN_NULL_HANDLE ? texture : m_pWhite);
        const VkRect2D clip = CurrentClip();
        const bool sameState = !m_Batches.empty() && m_Batches.back().Pipeline == m_pPipeline &&
                               m_Batches.back().Material == material && SameClip(m_Batches.back().Clip, clip) &&
                               m_Batches.back().Constants.transform == m_Transform &&
                               m_Batches.back().Constants.params[0] == m_Params[0] &&
                               m_Batches.back().Constants.params[1] == m_Params[1];
        if (!sameState)
        {
            Batch batch;
            batch.Pipeline = m_pPipeline;
            batch.Material = material;
            batch.Clip = clip;
            batch.Constants.transform = m_Transform;
            batch.Constants.params[0] = m_Params[0];
            batch.Constants.params[1] = m_Params[1];
            batch.FirstIndex = static_cast<EXUINT32>(m_Indices.size());
            m_Batches.push_back(batch);
        }
        m_Vertices.reserve(m_Vertices.size() + vertices);
        return static_cast<EXUINT32>(m_Vertices.size());
    }

    // ---- Primitives --------------------------------------------------------------------

    void VkCanvas::DrawQuad(VkTexture *texture, const std::array<EXMATH::vec2, 4> &p, const std::array<EXMATH::vec2, 4> &uv,
                            const std::array<VkCanvasColor, 4> &c)
    {
        const EXUINT32 first = Reserve(texture, 4);
        for (int i = 0; i < 4; ++i)
            m_Vertices.push_back({p[i].x, p[i].y, uv[i].x, uv[i].y, c[i]});
        m_Indices.insert(m_Indices.end(), {first, first + 1, first + 2, first + 2, first + 3, first});
        m_Batches.back().IndexCount += 6;
    }

    void VkCanvas::DrawTriangle(EXMATH::vec2 a, EXMATH::vec2 b, EXMATH::vec2 c, VkCanvasColor color)
    {
        const EXUINT32 first = Reserve(m_pWhite, 3);
        for (const EXMATH::vec2 &p : {a, b, c})
            m_Vertices.push_back({p.x, p.y, 0.5f, 0.5f, color});
        m_Indices.insert(m_Indices.end(), {first, first + 1, first + 2});
        m_Batches.back().IndexCount += 3;
    }

    void VkCanvas::DrawFan(EXMATH::vec2 center, const std::vector<EXMATH::vec2> &points, VkCanvasColor centerColor,
                           VkCanvasColor edgeColor)
    {
        if (points.size() < 2)
            return;
        const EXUINT32 first = Reserve(m_pWhite, static_cast<EXUINT32>(points.size()) + 1);
        m_Vertices.push_back({center.x, center.y, 0.5f, 0.5f, centerColor});
        for (const EXMATH::vec2 &p : points)
            m_Vertices.push_back({p.x, p.y, 0.5f, 0.5f, edgeColor});
        const EXUINT32 n = static_cast<EXUINT32>(points.size());
        for (EXUINT32 i = 0; i < n; ++i)
            m_Indices.insert(m_Indices.end(), {first, first + 1 + i, first + 1 + (i + 1) % n});
        m_Batches.back().IndexCount += 3 * n;
    }

    // ---- Sprites -----------------------------------------------------------------------

    void VkCanvas::DrawTexture(VkTexture *texture, const VkCanvasRect &source, const VkCanvasRect &destination, EXMATH::vec2 origin,
                               EXFLOAT rotation, VkCanvasColor tint)
    {
        if (texture == EXN_NULL_HANDLE || texture->GetWidth() <= 0 || texture->GetHeight() <= 0)
            return;
        const EXFLOAT tw = static_cast<EXFLOAT>(texture->GetWidth()), th = static_cast<EXFLOAT>(texture->GetHeight());
        const EXFLOAT u0 = source.x / tw, v0 = source.y / th;
        const EXFLOAT u1 = (source.x + source.width) / tw, v1 = (source.y + source.height) / th;

        const EXFLOAT radians = rotation * kPi / 180.0f;
        const EXFLOAT cs = std::cos(radians), sn = std::sin(radians);
        auto corner = [&](EXFLOAT x, EXFLOAT y)
        {
            const EXFLOAT dx = x - origin.x, dy = y - origin.y;
            return EXMATH::vec2(destination.x + dx * cs - dy * sn, destination.y + dx * sn + dy * cs);
        };
        const EXFLOAT w = destination.width, h = destination.height;
        DrawQuad(texture, {corner(0, 0), corner(w, 0), corner(w, h), corner(0, h)},
                 {EXMATH::vec2(u0, v0), EXMATH::vec2(u1, v0), EXMATH::vec2(u1, v1), EXMATH::vec2(u0, v1)}, {tint, tint, tint, tint});
    }

    void VkCanvas::DrawTexture(VkTexture *texture, EXMATH::vec2 position, VkCanvasColor tint)
    {
        if (texture == EXN_NULL_HANDLE)
            return;
        const EXFLOAT w = static_cast<EXFLOAT>(texture->GetWidth()), h = static_cast<EXFLOAT>(texture->GetHeight());
        DrawTexture(texture, {0, 0, w, h}, {position.x, position.y, w, h}, EXMATH::vec2(0.0f), 0.0f, tint);
    }

    // ---- Shapes ------------------------------------------------------------------------

    void VkCanvas::DrawRect(const VkCanvasRect &r, VkCanvasColor color) { DrawRectGradientV(r, color, color); }

    void VkCanvas::DrawRectGradientV(const VkCanvasRect &r, VkCanvasColor top, VkCanvasColor bottom)
    {
        const EXMATH::vec2 uv(0.5f);
        DrawQuad(m_pWhite,
                 {EXMATH::vec2(r.x, r.y), EXMATH::vec2(r.x + r.width, r.y), EXMATH::vec2(r.x + r.width, r.y + r.height),
                  EXMATH::vec2(r.x, r.y + r.height)},
                 {uv, uv, uv, uv}, {top, top, bottom, bottom});
    }

    void VkCanvas::DrawRectGradientH(const VkCanvasRect &r, VkCanvasColor left, VkCanvasColor right)
    {
        const EXMATH::vec2 uv(0.5f);
        DrawQuad(m_pWhite,
                 {EXMATH::vec2(r.x, r.y), EXMATH::vec2(r.x + r.width, r.y), EXMATH::vec2(r.x + r.width, r.y + r.height),
                  EXMATH::vec2(r.x, r.y + r.height)},
                 {uv, uv, uv, uv}, {left, right, right, left});
    }

    void VkCanvas::DrawRectLines(const VkCanvasRect &r, EXFLOAT t, VkCanvasColor color)
    {
        t = std::min({t, r.width * 0.5f, r.height * 0.5f});
        DrawRect({r.x, r.y, r.width, t}, color);
        DrawRect({r.x, r.y + r.height - t, r.width, t}, color);
        DrawRect({r.x, r.y + t, t, r.height - 2 * t}, color);
        DrawRect({r.x + r.width - t, r.y + t, t, r.height - 2 * t}, color);
    }

    void VkCanvas::DrawLine(EXMATH::vec2 a, EXMATH::vec2 b, EXFLOAT thickness, VkCanvasColor color)
    {
        const EXMATH::vec2 d = b - a;
        const EXFLOAT length = EXMATH::length(d);
        if (length <= 0.0f)
            return;
        const EXMATH::vec2 n = EXMATH::vec2(-d.y, d.x) / length * (thickness * 0.5f);
        const EXMATH::vec2 uv(0.5f);
        DrawQuad(m_pWhite, {a + n, b + n, b - n, a - n}, {uv, uv, uv, uv}, {color, color, color, color});
    }

    void VkCanvas::Ellipse(EXMATH::vec2 center, EXFLOAT rx, EXFLOAT ry, EXFLOAT thickness, VkCanvasColor color, EXBOOL filled)
    {
        if (rx <= 0.0f || ry <= 0.0f)
            return;
        const EXINT32 segments = SegmentsFor(std::max(rx, ry));
        if (filled)
        {
            std::vector<EXMATH::vec2> points(segments);
            for (EXINT32 i = 0; i < segments; ++i)
            {
                const EXFLOAT a = 2.0f * kPi * i / segments;
                points[i] = center + EXMATH::vec2(std::cos(a) * rx, std::sin(a) * ry);
            }
            DrawFan(center, points, color, color);
            return;
        }
        // Outline: a strip between the outer and the inner ellipse.
        const EXFLOAT h = thickness * 0.5f;
        const EXUINT32 first = Reserve(m_pWhite, static_cast<EXUINT32>(segments) * 2);
        for (EXINT32 i = 0; i < segments; ++i)
        {
            const EXFLOAT a = 2.0f * kPi * i / segments;
            const EXFLOAT c = std::cos(a), s = std::sin(a);
            m_Vertices.push_back({center.x + c * (rx + h), center.y + s * (ry + h), 0.5f, 0.5f, color});
            m_Vertices.push_back({center.x + c * std::max(0.0f, rx - h), center.y + s * std::max(0.0f, ry - h), 0.5f, 0.5f, color});
        }
        for (EXUINT32 i = 0; i < static_cast<EXUINT32>(segments); ++i)
        {
            const EXUINT32 j = (i + 1) % segments;
            const EXUINT32 o0 = first + 2 * i, i0 = o0 + 1, o1 = first + 2 * j, i1 = o1 + 1;
            m_Indices.insert(m_Indices.end(), {o0, o1, i1, i1, i0, o0});
        }
        m_Batches.back().IndexCount += 6 * segments;
    }

    void VkCanvas::DrawCircle(EXMATH::vec2 center, EXFLOAT radius, VkCanvasColor color) { Ellipse(center, radius, radius, 0.0f, color, true); }

    void VkCanvas::DrawCircleLines(EXMATH::vec2 center, EXFLOAT radius, EXFLOAT thickness, VkCanvasColor color)
    {
        Ellipse(center, radius, radius, thickness, color, false);
    }

    void VkCanvas::DrawEllipse(EXMATH::vec2 center, EXFLOAT rx, EXFLOAT ry, VkCanvasColor color) { Ellipse(center, rx, ry, 0.0f, color, true); }

    void VkCanvas::DrawEllipseLines(EXMATH::vec2 center, EXFLOAT rx, EXFLOAT ry, EXFLOAT thickness, VkCanvasColor color)
    {
        Ellipse(center, rx, ry, thickness, color, false);
    }

    void VkCanvas::DrawRing(EXMATH::vec2 center, EXFLOAT inner, EXFLOAT outer, EXFLOAT start, EXFLOAT end, VkCanvasColor color)
    {
        if (outer <= 0.0f || end <= start)
            return;
        const EXFLOAT sweep = (end - start) * kPi / 180.0f;
        const EXINT32 segments = std::max(2, static_cast<EXINT32>(SegmentsFor(outer) * sweep / (2.0f * kPi)) + 1);
        const EXUINT32 first = Reserve(m_pWhite, static_cast<EXUINT32>(segments + 1) * 2);
        for (EXINT32 i = 0; i <= segments; ++i)
        {
            const EXFLOAT a = start * kPi / 180.0f + sweep * i / segments;
            const EXFLOAT c = std::cos(a), s = std::sin(a);
            m_Vertices.push_back({center.x + c * outer, center.y + s * outer, 0.5f, 0.5f, color});
            m_Vertices.push_back({center.x + c * inner, center.y + s * inner, 0.5f, 0.5f, color});
        }
        for (EXUINT32 i = 0; i < static_cast<EXUINT32>(segments); ++i)
        {
            const EXUINT32 o0 = first + 2 * i, i0 = o0 + 1, o1 = o0 + 2, i1 = o0 + 3;
            m_Indices.insert(m_Indices.end(), {o0, o1, i1, i1, i0, o0});
        }
        m_Batches.back().IndexCount += 6 * segments;
    }

    // ---- Text --------------------------------------------------------------------------

    void VkCanvas::DrawString(VkFont *font, const std::string &utf8, EXMATH::vec2 position, EXFLOAT size, VkCanvasColor color)
    {
        if (font == EXN_NULL_HANDLE || utf8.empty())
            return;
        const Assets::Font &f = font->GetFont();
        const EXFLOAT scale = size / std::max(1.0f, f.GetSettings().PixelHeight);
        const Assets::TextLayout layout = f.Layout(utf8);
        for (const Assets::GlyphQuad &q : layout.Quads)
        {
            const EXMATH::vec2 a = position + EXMATH::vec2(q.X0, q.Y0) * scale;
            const EXMATH::vec2 b = position + EXMATH::vec2(q.X1, q.Y1) * scale;
            DrawQuad(font->GetTexture(), {a, EXMATH::vec2(b.x, a.y), b, EXMATH::vec2(a.x, b.y)},
                     {EXMATH::vec2(q.U0, q.V0), EXMATH::vec2(q.U1, q.V0), EXMATH::vec2(q.U1, q.V1), EXMATH::vec2(q.U0, q.V1)},
                     {color, color, color, color});
        }
    }

    EXMATH::vec2 VkCanvas::MeasureString(VkFont *font, const std::string &utf8, EXFLOAT size) const
    {
        if (font == EXN_NULL_HANDLE || utf8.empty())
            return EXMATH::vec2(0.0f, size);
        const Assets::Font &f = font->GetFont();
        const EXFLOAT scale = size / std::max(1.0f, f.GetSettings().PixelHeight);
        const Assets::TextLayout layout = f.Layout(utf8);
        return EXMATH::vec2(layout.Width, layout.Height) * scale;
    }

    // ---- Recording ---------------------------------------------------------------------

    void VkCanvas::Grow(VkDeviceSize needed, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &memory, VkDeviceSize &capacity,
                        void *&mapped)
    {
        if (needed <= capacity)
            return;
        // Only called while recording this frame again: its previous use is finished.
        VkDevice device = m_pRenderer->m_pDevice;
        if (buffer != EXN_NULL_HANDLE)
        {
            vkUnmapMemory(device, memory);
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
        capacity = std::max(needed, capacity * 2);
        m_pRenderer->CreateBuffer(capacity, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory);
        vkMapMemory(device, memory, 0, capacity, 0, &mapped);
    }

    void VkCanvas::Record(VkCommandBuffer commandBuffer, EXUINT32 frameIndex)
    {
        if (m_Indices.empty() || frameIndex >= m_Frames.size())
            return;

        FrameBuffers &frame = m_Frames[frameIndex];
        const VkDeviceSize vertexBytes = m_Vertices.size() * sizeof(VkCanvasVertex);
        const VkDeviceSize indexBytes = m_Indices.size() * sizeof(EXUINT32);
        Grow(std::max(vertexBytes, kInitialVertices * sizeof(VkCanvasVertex)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, frame.Vertices,
             frame.VertexMemory, frame.VertexCapacity, frame.VertexMapped);
        Grow(std::max(indexBytes, kInitialVertices * 2 * sizeof(EXUINT32)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, frame.Indices,
             frame.IndexMemory, frame.IndexCapacity, frame.IndexMapped);
        std::memcpy(frame.VertexMapped, m_Vertices.data(), vertexBytes);
        std::memcpy(frame.IndexMapped, m_Indices.data(), indexBytes);

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &frame.Vertices, &offset);
        vkCmdBindIndexBuffer(commandBuffer, frame.Indices, 0, VK_INDEX_TYPE_UINT32);

        const VkDescriptorSet globalSet = m_pRenderer->m_pFrameObjects[frameIndex].descriptorSet;
        const VkGraphicsPipeline *boundPipeline = EXN_NULL_HANDLE;
        VkMaterial *boundMaterial = EXN_NULL_HANDLE;
        for (const Batch &batch : m_Batches)
        {
            if (batch.IndexCount == 0 || batch.Pipeline == EXN_NULL_HANDLE || batch.Pipeline->m_pPipeline == EXN_NULL_HANDLE ||
                batch.Clip.extent.width == 0 || batch.Clip.extent.height == 0)
                continue;
            if (batch.Pipeline != boundPipeline)
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.Pipeline->m_pPipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.Pipeline->m_pLayout, 0, 1, &globalSet, 0, nullptr);
                boundPipeline = batch.Pipeline;
                boundMaterial = EXN_NULL_HANDLE;
            }
            if (batch.Material != boundMaterial && batch.Material != EXN_NULL_HANDLE)
            {
                batch.Material->Bind(commandBuffer, batch.Pipeline->m_pLayout, frameIndex);
                boundMaterial = batch.Material;
            }
            vkCmdSetScissor(commandBuffer, 0, 1, &batch.Clip);
            vkCmdPushConstants(commandBuffer, batch.Pipeline->m_pLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(VkCanvasPushConstants), &batch.Constants);
            vkCmdDrawIndexed(commandBuffer, batch.IndexCount, 1, batch.FirstIndex, 0, 0);
        }

        // Later recordings in this pass (e.g. the render handler) expect the full image.
        const VkRect2D full{{0, 0}, m_pRenderer->m_szSwapChainExtent};
        vkCmdSetScissor(commandBuffer, 0, 1, &full);
    }
}
#endif
