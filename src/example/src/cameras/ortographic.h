#include <eXngine.h>

#include <types/matrix.h>
#include <types/vector.h>

#include <renderers/camera/camera.h>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace eXngine::Types;

class OrtographicCamera : public eXngine::Renderers::eXcamera
{
    eXmat4 GetViewMatrix() override
    {
        return EXMATH::lookAt(
            m_Position,
            m_Position + m_Front,
            m_Up
        );
    };

    eXmat4 GetProjectionMatrix() override
    {
        return EXMATH::ortho(
            -m_Viewport->GetAspectRatio() * m_FieldOfView / 2.0f,
            m_Viewport->GetAspectRatio() * m_FieldOfView / 2.0f,
            -m_FieldOfView / 2.0f,
            m_FieldOfView / 2.0f,
            m_Near,
            m_Far
        );
    };
};