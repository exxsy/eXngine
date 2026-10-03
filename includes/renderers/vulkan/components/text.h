#pragma once

#ifndef EXN_DISABLE_VULKAN
#include <string>
#include <eXngine.h>
#include <assets/font.h>
#include <component/component.h>
#include <types/vector.h>

namespace eXngine::Renderers::Vulkan
{
    class VkFont;

    enum class VkTextSpace
    {
        // In the scene, seen through the camera: Size is in world units.
        World,
        // On top of the screen (HUD): the transform's position is in pixels from the
        // top-left corner of the window, Size is in pixels.
        Screen,
    };

    // Makes an entity show a text: VkTextSystem lays it out with Font and draws it at the
    // entity's eXtransformComponent. Edit the fields at any time; the text is laid out
    // again when one of them changes.
    //   auto *label = world->CreateEntity("Label");
    //   label->AddComponent<eXtransformComponent>()->Position = eXvec3(0.0f, 1.0f, 0.0f);
    //   label->AddComponent<VkTextComponent>(renderer->GetFont("ui"), "Merhaba Dünya!");
    struct VkTextComponent : public Component::eXcomponent
    {
        VkFont *Font = nullptr;
        // UTF-8; '\n' starts a new line.
        std::string Text;
        eXvec3 Color = eXvec3(1.0f, 1.0f, 1.0f);
        // Height of one line of text, without the line gap.
        EXFLOAT Size = 0.2f;
        Assets::TextAlign Align = Assets::TextAlign::Left;
        // Wraps lines wider than this (in the units of Size); 0 never wraps.
        EXFLOAT MaxWidth = 0.0f;
        EXFLOAT LineSpacing = 1.0f;
        // The point of the text's bounding box placed at the transform's position:
        // (0, 0) is the top-left corner, (0.5, 0.5) the center, (1, 1) the bottom-right.
        eXvec2 Pivot = eXvec2(0.5f, 0.5f);
        VkTextSpace Space = VkTextSpace::World;
        EXBOOL Visible = true;

        VkTextComponent() = default;
        VkTextComponent(VkFont *font, const std::string &text) : Font(font), Text(text) {}
    };
}
#endif
