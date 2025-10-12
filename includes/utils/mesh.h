
#pragma once

#include <vector>
#include <utils/vertex.h>

namespace eXngine::Utils
{
    struct Mesh
    {
        std::vector<eXngine::Utils::Vertex> vertices;
        std::vector<uint16_t> indices;
    };
}
