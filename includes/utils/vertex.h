
#pragma once

#include <vector>
#include <eXngine.h>
#include <renderers/defines.h>
#include <vulkan/vulkan_core.h>

namespace eXngine::Utils
{
    struct Vertex
    {
        EXVEC3 position;
        EXVEC2 texture_coordinates;
    };
}
