
#pragma once

#include <vector>
#include <utils/vertex.h>

namespace eXngine::Utils
{
    struct Mesh
    {
        std::vector<eXngine::Utils::Vertex> vertices;
        std::vector<EXUINT16> indices;
        EXUINT32 textureIndex = 0;
        EXUINT32 numTextureCount = 1;
    };
}
