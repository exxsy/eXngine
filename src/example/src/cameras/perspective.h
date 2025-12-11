#include <eXngine.h>

#include <types/matrix.h>
#include <types/vector.h>

#include <renderers/camera/camera.h>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace eXngine::Types;

class PerspectiveCamera : public eXngine::Renderers::eXcamera
{
public:
    EXN_PROPERTY(EXFLOAT, MovementSpeed, MovementSpeed, 5.0f);
    EXN_PROPERTY(EXFLOAT, MouseSensitivity, MouseSensitivity, 0.1f);
    EXN_PROPERTY(EXFLOAT, Yaw, Yaw, -90.0f);
    EXN_PROPERTY(EXFLOAT, Pitch, Pitch, 0.0f);

    eXmat4 GetViewMatrix() override
    {
        return EXMATH::lookAt(
            m_Position,
            m_Position + m_Front,
            m_Up);
    };

    eXmat4 GetProjectionMatrix() override
    {
        return EXMATH::perspective(
            m_FieldOfView,
            m_Viewport->GetAspectRatio(),
            m_Near,
            m_Far);
    };

    void MoveForward(EXFLOAT delta)
    {
        m_Position += m_Front * (delta * m_MovementSpeed);
    }

    void MoveBackward(EXFLOAT delta)
    {
        m_Position -= m_Front * (delta * m_MovementSpeed);
    }

    void MoveRight(EXFLOAT delta)
    {
        eXvec3 right = EXMATH::normalize(EXMATH::cross(m_Front, m_Up));
        m_Position += right * (delta * m_MovementSpeed);
    }

    void MoveLeft(EXFLOAT delta)
    {
        eXvec3 right = EXMATH::normalize(EXMATH::cross(m_Front, m_Up));
        m_Position -= right * (delta * m_MovementSpeed);
    }

    void MoveUp(EXFLOAT delta)
    {
        m_Position += m_Up * (delta * m_MovementSpeed);
    }

    void MoveDown(EXFLOAT delta)
    {
        m_Position -= m_Up * (delta * m_MovementSpeed);
    }

    void Rotate(EXFLOAT offsetX, EXFLOAT offsetY)
    {
        offsetX *= m_MouseSensitivity;
        offsetY *= m_MouseSensitivity;

        m_Yaw += offsetX;
        m_Pitch += offsetY;

        // Clamp pitch to avoid flip
        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;
        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;
    }
};