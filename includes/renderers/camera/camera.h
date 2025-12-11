#pragma once

#include <types/vector.h>
#include <types/matrix.h>

#include <renderers/viewport/viewport.h>

using namespace eXngine::Types;

namespace eXngine::Renderers
{
    enum CameraMovement : EXUINT32
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class EXNEXPORT eXcamera
    {
    public:
        EXN_PROPERTY(eXvec3, Position, Position, eXvec3(0.0f, 0.0f, 0.0f));
        EXN_PROPERTY(eXvec3, Up, Up, eXvec3(0.0f, 1.0f, 0.0f));
        EXN_PROPERTY(eXvec3, Front, Front, eXvec3(0.0f, 0.0f, -1.0f));
        EXN_PROPERTY(EXFLOAT, FieldOfView, FieldOfView, 90.0f);
        EXN_PROPERTY(EXFLOAT, Near, Near, 0.1f);
        EXN_PROPERTY(EXFLOAT, Far, Far, 100.0f);
        EXN_PROPERTY(eXviewport *, Viewport, Viewport, EXN_NULL_HANDLE);

        eXmat4 m_ViewMatrix, m_ProjectionMatrix;
    public:
        eXcamera() = default;

        virtual eXmat4 GetViewMatrix() = 0;
        virtual eXmat4 GetProjectionMatrix() = 0;
    };
}