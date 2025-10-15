
#pragma once

#include <eXngine.h>
#include <array>
#include <vulkan/vulkan_core.h>

namespace eXngine::Utils
{
    struct Vertex
    {
        EXVEC3 position;
        EXVEC2 texture_coordinates;
    };
}
