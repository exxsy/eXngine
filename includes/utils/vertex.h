
#pragma once

#include <vector>
#include <eXngine.h>
#include <renderers/defines.h>
#include <vulkan/vulkan_core.h>
#include <types/vector.h>

using namespace eXngine::Types;

namespace eXngine::Utils
{
    struct Vertex
    {
    public:
        eXvec3 color;
        eXvec3 coordinates;
    };
}
