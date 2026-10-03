#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <eXngine.h>
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <renderers/vulkan/pipelines/graphics.h>

// Pipeline (and shader name prefix: "canvas.vertex" / "canvas.fragment") of VkCanvas.
#define EXN_CANVAS_PIPELINE "canvas"
// Render pass VkCanvas records into: after the scene, keeps its image, no depth.
#define EXN_CANVAS_RENDERPASS "canvas"

namespace eXngine::Renderers::Vulkan
{
    class Renderer;
    class VkTexture;
    class VkMaterial;
    class VkFont;

    // RGBA8 color for the canvas, in sRGB like image files and color pickers.
    struct VkCanvasColor
    {
        EXUINT8 r = 255, g = 255, b = 255, a = 255;
    };

    struct VkCanvasRect
    {
        EXFLOAT x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
    };

    // Vertex of the canvas pipelines: 2D position, texture coordinates, RGBA8 color.
    struct EXNEXPORT VkCanvasVertex
    {
        EXFLOAT x, y;
        EXFLOAT u, v;
        VkCanvasColor color;

        static VkVertexInputBindingDescription GetBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };

    // Per batch data (push constants) of the canvas pipelines, see shaders/canvas.vert.
    //   transform  2D position -> clip space (the screen or a world camera)
    //   params     free for custom pipelines (time, camera, ...); unused by "canvas"
    struct VkCanvasPushConstants
    {
        alignas(16) EXMATH::mat4 transform = EXMATH::mat4(1.0f);
        alignas(16) EXMATH::vec4 params[2] = {EXMATH::vec4(0.0f), EXMATH::vec4(0.0f)};
        alignas(4) EXUINT32 textureIndex = 0;
    };

    // Alpha blended 2D drawing on top of the scene: no depth, no culling, canvas push
    // constants. Custom canvas pipelines (e.g. an animated background) use it too:
    //   renderer->AllocatePipeline<VkCanvasPipeline>("sea");
    //   renderer->LoadShader("sea.vertex", ...); renderer->LoadShader("sea.fragment", ...);
    //   renderer->Initialize(); canvas.Create();
    //   renderer->CreatePipeline<VkCanvasVertex>("sea", EXN_CANVAS_RENDERPASS);
    struct VkCanvasPipeline : public VkGraphicsPipeline
    {
        using VkGraphicsPipeline::VkGraphicsPipeline;

        VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState() override
        {
            return {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                // RGB only: the window's alpha stays what the scene pass cleared it to (opaque),
                // so the desktop compositor never shows anything through translucent 2D layers.
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
            };
        }

        VkPipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo() override
        {
            VkPipelineDepthStencilStateCreateInfo info = VkGraphicsPipeline::GetDepthStencilStateInfo();
            info.depthTestEnable = VK_FALSE;
            info.depthWriteEnable = VK_FALSE;
            return info;
        }

        VkPushConstantRange *GetPushContantRangeInfo(EXUINT32 &count) override
        {
            m_PushConstants = {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(VkCanvasPushConstants),
            };
            count = 1;
            return &m_PushConstants;
        }

    private:
        VkPushConstantRange m_PushConstants{};
    };

    // Immediate mode 2D drawing: sprites (parts of textures), shapes, text and custom
    // shader quads, collected every frame and recorded in one render pass, batched by
    // pipeline, texture, clip rectangle and transform. Coordinates are pixels from the
    // top-left corner of the window (y down) unless a transform is set.
    //
    // Setup (once):
    //   VkCanvas::Prepare(renderer, canvasVertSpv, canvasFragSpv);   // before Initialize()
    //   renderer->Initialize();
    //   VkCanvas canvas(renderer); canvas.Create();                   // after Initialize()
    // Every frame:
    //   canvas.Begin();
    //   canvas.DrawRect({10, 10, 100, 40}, {200, 40, 40, 255});
    //   canvas.DrawTexture(sheet, {0, 0, 192, 192}, {300, 200, 192, 192});
    //   canvas.DrawString(font, "Merhaba!", {20, 80}, 18.0f, {255, 255, 255, 255});
    //   renderer->OnRender();   // records what was drawn since Begin()
    //
    // Textures must outlive the frames that draw them: call Forget() before destroying one.
    class EXNEXPORT VkCanvas
    {
    public:
        // Allocates the canvas pipeline and loads its shaders (SPIR-V of shaders/canvas.*).
        static void Prepare(Renderer *, const std::vector<char> &vertexSpirv, const std::vector<char> &fragmentSpirv);

        explicit VkCanvas(Renderer *);
        ~VkCanvas();
        VkCanvas(const VkCanvas &) = delete;
        VkCanvas &operator=(const VkCanvas &) = delete;

        // Creates the render pass, the pipeline and a white texture. After Initialize().
        void Create();
        void Release();

        // Starts a new frame: forgets everything drawn before.
        void Begin();

        // ---- State (affects what is drawn afterwards) ----
        // Maps drawn positions to the screen; Screen() = pixels, y down. Use for a 2D camera:
        //   canvas.SetTransform(canvas.Screen() * cameraMatrix);
        void SetTransform(const EXMATH::mat4 &);
        const EXMATH::mat4 &GetTransform() const { return m_Transform; }
        EXMATH::mat4 Screen() const;
        // Only pixels inside the rectangle (screen pixels) are drawn. Nestable.
        void PushClip(const VkCanvasRect &);
        void PopClip();
        // A custom canvas pipeline (and its parameters) for the following draws; null = default.
        void SetPipeline(VkGraphicsPipeline *, const EXMATH::vec4 &params0 = EXMATH::vec4(0.0f),
                         const EXMATH::vec4 &params1 = EXMATH::vec4(0.0f));

        // ---- Primitives ----
        // Any quad: corners clockwise from top-left, with their texture coordinates.
        void DrawQuad(VkTexture *, const std::array<EXMATH::vec2, 4> &positions, const std::array<EXMATH::vec2, 4> &uvs,
                      const std::array<VkCanvasColor, 4> &colors);
        void DrawTriangle(EXMATH::vec2 a, EXMATH::vec2 b, EXMATH::vec2 c, VkCanvasColor);
        // Filled convex polygon around center: a fan from center through points (closed).
        void DrawFan(EXMATH::vec2 center, const std::vector<EXMATH::vec2> &points, VkCanvasColor centerColor, VkCanvasColor edgeColor);

        // ---- Sprites ----
        // Part `source` (pixels) of the texture into `destination`, rotated (degrees,
        // clockwise) around `origin` (pixels from the destination's top-left corner, which
        // is placed at destination.x / y).
        void DrawTexture(VkTexture *, const VkCanvasRect &source, const VkCanvasRect &destination,
                         EXMATH::vec2 origin = EXMATH::vec2(0.0f), EXFLOAT rotation = 0.0f,
                         VkCanvasColor tint = {});
        void DrawTexture(VkTexture *, EXMATH::vec2 position, VkCanvasColor tint = {});

        // ---- Shapes ----
        void DrawRect(const VkCanvasRect &, VkCanvasColor);
        void DrawRectGradientV(const VkCanvasRect &, VkCanvasColor top, VkCanvasColor bottom);
        void DrawRectGradientH(const VkCanvasRect &, VkCanvasColor left, VkCanvasColor right);
        void DrawRectLines(const VkCanvasRect &, EXFLOAT thickness, VkCanvasColor);
        void DrawLine(EXMATH::vec2 a, EXMATH::vec2 b, EXFLOAT thickness, VkCanvasColor);
        void DrawCircle(EXMATH::vec2 center, EXFLOAT radius, VkCanvasColor);
        void DrawCircleLines(EXMATH::vec2 center, EXFLOAT radius, EXFLOAT thickness, VkCanvasColor);
        void DrawEllipse(EXMATH::vec2 center, EXFLOAT radiusX, EXFLOAT radiusY, VkCanvasColor);
        void DrawEllipseLines(EXMATH::vec2 center, EXFLOAT radiusX, EXFLOAT radiusY, EXFLOAT thickness, VkCanvasColor);
        // Part of a ring between two radii; angles in degrees, clockwise from +x.
        void DrawRing(EXMATH::vec2 center, EXFLOAT innerRadius, EXFLOAT outerRadius, EXFLOAT startAngle, EXFLOAT endAngle,
                      VkCanvasColor);

        // ---- Text (UTF-8) ----
        // (Not "DrawText": <Windows.h> turns that name into a macro.)
        // `size`: line height in pixels; position: top-left corner of the text.
        void DrawString(VkFont *, const std::string &utf8, EXMATH::vec2 position, EXFLOAT size, VkCanvasColor);
        EXMATH::vec2 MeasureString(VkFont *, const std::string &utf8, EXFLOAT size) const;

        // Call before destroying a texture the canvas has drawn (its material is reused).
        void Forget(VkTexture *);

        VkTexture *GetWhiteTexture() const { return m_pWhite; }

    private:
        struct Batch
        {
            VkGraphicsPipeline *Pipeline = EXN_NULL_HANDLE;
            VkMaterial *Material = EXN_NULL_HANDLE;
            VkRect2D Clip{};
            VkCanvasPushConstants Constants;
            EXUINT32 FirstIndex = 0, IndexCount = 0;
        };

        struct FrameBuffers
        {
            VkBuffer Vertices = EXN_NULL_HANDLE, Indices = EXN_NULL_HANDLE;
            VkDeviceMemory VertexMemory = EXN_NULL_HANDLE, IndexMemory = EXN_NULL_HANDLE;
            VkDeviceSize VertexCapacity = 0, IndexCapacity = 0;
            void *VertexMapped = EXN_NULL_HANDLE, *IndexMapped = EXN_NULL_HANDLE;
        };

        // Room for `vertices` more vertices in a batch drawing with this texture.
        EXUINT32 Reserve(VkTexture *, EXUINT32 vertices);
        VkMaterial *MaterialFor(VkTexture *);
        void Record(VkCommandBuffer, EXUINT32 frameIndex);
        void Grow(VkDeviceSize needed, VkBufferUsageFlags, VkBuffer &, VkDeviceMemory &, VkDeviceSize &, void *&);
        VkRect2D CurrentClip() const;
        void Ellipse(EXMATH::vec2 center, EXFLOAT rx, EXFLOAT ry, EXFLOAT thickness, VkCanvasColor, EXBOOL filled);

        Renderer *m_pRenderer = EXN_NULL_HANDLE;
        VkGraphicsPipeline *m_pDefault = EXN_NULL_HANDLE;
        VkGraphicsPipeline *m_pPipeline = EXN_NULL_HANDLE;
        EXMATH::vec4 m_Params[2] = {EXMATH::vec4(0.0f), EXMATH::vec4(0.0f)};
        VkTexture *m_pWhite = EXN_NULL_HANDLE;
        EXMATH::mat4 m_Transform = EXMATH::mat4(1.0f);
        std::vector<VkCanvasRect> m_Clips;

        std::vector<VkCanvasVertex> m_Vertices;
        std::vector<EXUINT32> m_Indices;
        std::vector<Batch> m_Batches;
        std::vector<FrameBuffers> m_Frames;

        std::unordered_map<VkTexture *, VkMaterial *> m_Materials;
        std::vector<VkMaterial *> m_FreeMaterials;
        EXUINT32 m_nMaterialCount = 0;
    };
}
#endif
