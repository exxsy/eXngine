#pragma once

#include <concepts>
#include <eXngine.h>
#include <types/vector.h>
#include <types/transform.h>

using namespace eXngine::Types;

namespace eXngine::Entity
{
    struct EXNEXPORT eXentity
    {
    public:
        EXN_PROPERTY(eXtransform, Transform, transform, eXtransform());
        EXN_PROPERTY(EXUINT, MeshID, MeshID, 0);

        eXentity();
        ~eXentity();
    };
};