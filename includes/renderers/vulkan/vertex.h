#pragma once

#include <utils/vertex.h>
#include <vulkan/vulkan_core.h>

namespace eXngine::Renderers::Vulkan
{
    struct EXNEXPORT VkVertex : public eXngine::Utils::Vertex
    {
        static VkVertexInputBindingDescription GetBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };
}
