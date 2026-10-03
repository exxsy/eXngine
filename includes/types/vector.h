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
        eXvec3& operator=(const EXMATH::vec3& other)
        {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
            return *this;
        }
    };

    struct EXNEXPORT eXvec2 : public eXvec<float, 2>
    {
        eXvec2() : eXvec<float, 2>() {}
        eXvec2(float x, float y) 
        {
            this->x = x;
            this->y = y;
        }
        eXvec2(const EXMATH::vec2& other) 
        {
            this->x = other.x;
            this->y = other.y;
        }
        eXvec2& operator=(const EXMATH::vec2& other)
        {
            this->x = other.x;
            this->y = other.y;
            return *this;
        }
    };

    struct EXNEXPORT eXvec4 : public eXvec<float, 4>
    {
        eXvec4() : eXvec<float, 4>() {}
        eXvec4(float x, float y, float z, float w) 
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
        eXvec4(const EXMATH::vec4& other) 
        {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
            this->w = other.w;
        }
        eXvec4& operator=(const EXMATH::vec4& other)
        {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
            this->w = other.w;
            return *this;
        }
    };
}