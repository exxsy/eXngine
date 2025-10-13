
#pragma once

#include <eXngine.h>
#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

namespace eXngine::Utils
{
    struct Vertex
    {
        EXVEC3 pos;
        EXVEC2 texCoordinates;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };
}
