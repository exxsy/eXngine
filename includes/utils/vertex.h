
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
        eXvec<EXUINT8, 3> color;
        eXvec<EXFLOAT, 3> coordinates;
    };
}
