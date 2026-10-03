#ifndef EXN_DISABLE_VULKAN
#include <algorithm>
#include <vector>

#include <component/transform.h>
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/systems/text.h>

namespace eXngine::Renderers::Vulkan
{
    namespace
    {
        // Font pixels -> units of VkTextComponent::Size.
        EXFLOAT GetScale(const VkTextComponent &text)
        {
            const EXFLOAT pixelHeight = text.Font->GetFont().GetSettings().PixelHeight;
            return pixelHeight > 0.0f ? text.Size / pixelHeight : 1.0f;
        }

        // Whether the glyph rectangles of a and b differ. Size, Space and the transform only
        // change the matrix, except that MaxWidth is measured in the units of Size.
        EXBOOL NeedsLayout(const VkTextComponent &a, const VkTextComponent &b)
        {
            return a.Font != b.Font || a.Text != b.Text || a.Color != b.Color || a.Align != b.Align ||
                   a.MaxWidth != b.MaxWidth || a.LineSpacing != b.LineSpacing || a.Pivot != b.Pivot ||
                   (a.MaxWidth > 0.0f && a.Size != b.Size);
        }
    }

    VkTextSystem::VkTextSystem(Renderer *renderer, const std::string &renderPass)
        : m_pRenderer(renderer), m_RenderPass(renderPass)
    {
    }

    void VkTextSystem::OnUpdate(World::eXworld &world, EXFLOAT)
    {
        VkRenderPassObject *pass = m_pRenderer->GetRenderPass(m_RenderPass);

        if (pass == EXN_NULL_HANDLE)
            return;

        // Screen space: pixels -> clip space is an orthographic projection. The shaders
        // always apply the camera (proj * view * model), so the model matrix starts with
        // the inverse of the camera to cancel it out.
        EXMAT4 screenToWorld(1.0f);
        EXBOOL hasScreenMatrix = false;

        if (eXcamera *camera = m_pRenderer->GetMainCamera())
        {
            const VkExtent2D extent = m_pRenderer->m_szSwapChainExtent;
            EXMATH::mat4 projection = camera->GetProjectionMatrix();
            projection[1][1] *= -1; // as in the uniform buffer

            // x: 0..width -> -1..1, y: 0..height -> -1..1 (Vulkan's y points down, like the
            // pixels), z: just behind the near plane, in front of the scene. Not exactly on
            // it: the rounding errors of the inverse could push it in front and clip it away.
            EXMATH::mat4 orthographic(1.0f);
            orthographic[0][0] = 2.0f / static_cast<EXFLOAT>(std::max(extent.width, 1u));
            orthographic[1][1] = 2.0f / static_cast<EXFLOAT>(std::max(extent.height, 1u));
            orthographic[2][2] = 0.0f;
            orthographic[3] = EXMATH::vec4(-1.0f, -1.0f, 0.001f, 1.0f);

            screenToWorld = EXMATH::inverse(projection * EXMATH::mat4(camera->GetViewMatrix())) * orthographic;
            hasScreenMatrix = true;
        }

        std::unordered_map<EXUINT, DrawnText> drawn;
        drawn.reserve(m_Texts.size());

        for (auto &[id, text] : world.GetComponents<VkTextComponent>())
        {
            if (!text.Visible || text.Font == EXN_NULL_HANDLE || (text.Space == VkTextSpace::Screen && !hasScreenMatrix))
                continue;

            DrawnText entry;
            const auto it = m_Texts.find(id);

            if (it != m_Texts.end())
            {
                entry = std::move(it->second);
                m_Texts.erase(it);
            }
            else
            {
                entry.Mesh = m_pRenderer->CreateDynamicMesh();

                if (entry.Mesh == EXN_NULL_HANDLE)
                    continue;

                entry.Command = pass->Draw(entry.Mesh, text.Font->GetMaterial());
            }

            if (!entry.HasMesh || NeedsLayout(entry.Built, text))
                BuildMesh(entry, text);

            const auto *entity = world.GetEntity(id);
            const auto *transform = entity != EXN_NULL_HANDLE ? entity->GetComponent<Component::eXtransformComponent>() : EXN_NULL_HANDLE;
            const EXMATH::mat4 placement = transform != EXN_NULL_HANDLE ? EXMATH::mat4(transform->GetMatrix()) : EXMATH::mat4(1.0f);
            const EXFLOAT scale = GetScale(text);

            // The layout's y grows downwards: right for the screen, flipped for the world (y up).
            if (text.Space == VkTextSpace::Screen)
                entry.Command->model = screenToWorld * placement * EXMATH::scale(EXMATH::mat4(1.0f), EXMATH::vec3(scale, scale, 1.0f));
            else
                entry.Command->model = placement * EXMATH::scale(EXMATH::mat4(1.0f), EXMATH::vec3(scale, -scale, scale));

            entry.Command->mesh = entry.Mesh;
            entry.Command->material = text.Font->GetMaterial();
            entry.Command->textureIndex = 0;

            drawn.emplace(id, std::move(entry));
        }

        // What is left belongs to entities that were destroyed, hidden or lost their text.
        for (auto &[id, entry] : m_Texts)
            Release(entry, pass);

        m_Texts = std::move(drawn);
    }

    void VkTextSystem::BuildMesh(DrawnText &entry, const VkTextComponent &text)
    {
        Assets::TextLayoutOptions options;
        options.Align = text.Align;
        options.MaxWidth = text.MaxWidth > 0.0f ? text.MaxWidth / GetScale(text) : 0.0f;
        options.LineSpacing = text.LineSpacing;

        const Assets::TextLayout layout = text.Font->GetFont().Layout(text.Text, options);

        // Vertices in font pixels around the pivot; the draw command's matrix scales them.
        const EXFLOAT originX = text.Pivot.x * layout.Width;
        const EXFLOAT originY = text.Pivot.y * layout.Height;

        std::vector<Utils::Vertex> vertices;
        std::vector<EXUINT32> indices;
        vertices.reserve(layout.Quads.size() * 4);
        indices.reserve(layout.Quads.size() * 6);

        for (const auto &quad : layout.Quads)
        {
            const EXUINT32 first = static_cast<EXUINT32>(vertices.size());
            const EXFLOAT x0 = quad.X0 - originX, x1 = quad.X1 - originX;
            const EXFLOAT y0 = quad.Y0 - originY, y1 = quad.Y1 - originY;

            vertices.push_back({.color = text.Color, .coordinates = {x0, y0, 0.0f}, .uv = {quad.U0, quad.V0}});
            vertices.push_back({.color = text.Color, .coordinates = {x1, y0, 0.0f}, .uv = {quad.U1, quad.V0}});
            vertices.push_back({.color = text.Color, .coordinates = {x1, y1, 0.0f}, .uv = {quad.U1, quad.V1}});
            vertices.push_back({.color = text.Color, .coordinates = {x0, y1, 0.0f}, .uv = {quad.U0, quad.V1}});

            indices.insert(indices.end(), {first, first + 1, first + 2, first + 2, first + 3, first});
        }

        // An empty text gives an empty mesh, which the render pass skips.
        entry.Mesh->Update(vertices, indices);
        entry.Built = text;
        entry.HasMesh = true;
    }

    void VkTextSystem::Release(DrawnText &entry, VkRenderPassObject *pass)
    {
        if (pass != EXN_NULL_HANDLE)
            pass->Remove(entry.Command);

        // Waits for the GPU, but only happens when a text goes away.
        m_pRenderer->DestroyMesh(entry.Mesh);

        entry.Command = EXN_NULL_HANDLE;
        entry.Mesh = EXN_NULL_HANDLE;
    }

    void VkTextSystem::OnDetach(World::eXworld &)
    {
        // After Renderer::OnExit the pass and the meshes are already gone: DestroyMesh
        // ignores meshes the renderer does not know anymore.
        VkRenderPassObject *pass = m_pRenderer->GetRenderPass(m_RenderPass);

        for (auto &[id, entry] : m_Texts)
            Release(entry, pass);

        m_Texts.clear();
    }
}
#endif
