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
        eXvec3(const EXMATH::vec3& other) 
        {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
        }
        eXvec3 operator=(const EXMATH::vec3& other) const
        {
            return eXvec3(other.x, other.y, other.z);
        }
    };
}