#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace eXngine::Types
{
    template <typename T>
    struct EXNEXPORT eXquat : public EXMATH::qua<T, EXMATH::highp>{
        eXquat() : EXMATH::qua<T, EXMATH::highp>() {}
        eXquat(T x, T y, T z, T w) 
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
    };
}