#pragma once

#include <eXngine.h>
#include <types/vector.h>
#include <types/quat.h>

namespace eXngine::Types
{
    struct eXtransform
    {
        eXvec3 Position;
        eXquat<float> Rotation;
        eXvec3 Scale;

        eXtransform() : Position(0.0f, 0.0f, 0.0f), Rotation(0.0f, 0.0f, 0.0f, 1.0f), Scale(1.0f, 1.0f, 1.0f)
        {}

        eXtransform(const eXvec3& position, const eXquat<float>& rotation, const eXvec3& scale)
            : Position(position), Rotation(rotation), Scale(scale)
        {}
    };
} // namespace eXngine::Types