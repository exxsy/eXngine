#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>

#include <eXngine.h>
#include <assets/font.h>

namespace eXngine::Renderers::Vulkan
{
    class VkTexture;
    class VkMaterial;

    // A font ready to draw: the baked glyphs (Assets::Font), their atlas on the GPU and a
    // material that samples it with alpha blending (EXN_TRANSPARENT_PIPELINE).
    // Created via Renderer::LoadFont and owned by the renderer, like its texture
    // ("font:<name>") and material ("font:<name>"). Draw text with VkTextComponent.
    class EXNEXPORT VkFont
    {
    public:
        VkFont(const std::string &name, Assets::Font &&font, VkTexture *texture, VkMaterial *material)
            : m_Name(name), m_Font(std::move(font)), m_pTexture(texture), m_pMaterial(material) {}

        const std::string &GetName() const { return m_Name; }
        const Assets::Font &GetFont() const { return m_Font; }
        VkTexture *GetTexture() const { return m_pTexture; }
        VkMaterial *GetMaterial() const { return m_pMaterial; }

    private:
        std::string m_Name;
        Assets::Font m_Font;
        VkTexture *m_pTexture = nullptr;
        VkMaterial *m_pMaterial = nullptr;
    };
}
#endif
