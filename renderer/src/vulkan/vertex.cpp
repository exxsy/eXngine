#include <renderers/vulkan/vertex.h>

namespace eXngine::Renderers::Vulkan
{
    VkVertexInputBindingDescription VkVertex::getBindingDescription()
    {
        return {
            .binding = 0,
            .stride = sizeof(VkVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    std::vector<VkVertexInputAttributeDescription> VkVertex::getAttributeDescriptions()
    {
        /*
            float: VK_FORMAT_R32_SFLOAT
            vec2: VK_FORMAT_R32G32_SFLOAT
            vec3: VK_FORMAT_R32G32B32_SFLOAT
            vec4: VK_FORMAT_R32G32B32A32_SFLOAT

            ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
            uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
            double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
         */

        return {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(VkVertex, position),
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(VkVertex, texture_coordinates),
            }
        };
    }
}