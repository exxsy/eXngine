#pragma once

#include <glm/glm.hpp>

namespace eXngine::Types
{
    template <typename T, std::size_t N>
    struct EXNEXPORT eXvec : public EXMATH::vec<N, T, EXMATH::highp>{};

    struct EXNEXPORT eXvec3 : public eXvec<float, 3>
    {
        eXvec3() : eXvec<float, 3>() {}
        eXvec3(float x, float y, float z) 
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }
    };
}