
#pragma once

#include <eXngine.h>
#include <array>
#include <vulkan/vulkan_core.h>

namespace eXngine::Utils
{
    struct Vertex
    {
        EXVEC3 pos;
        EXVEC2 texCoordinates;
		EXUINT32 textureIndex;
    };
}
