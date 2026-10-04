#pragma once

#include <algorithm>
#include <cmath>

#include <eXngine.h>
#include <input/input.h>
#include <renderers/camera/camera.h>

#include <glm/glm.hpp>

using namespace eXngine::Types;

// Flies a camera around like the scene view of an editor:
//   right mouse button (hold)    look around; the cursor hides and stays where it is
//   W A S D                      forward / left / back / right, along the view direction
//   E / Q                        up / down, along the world up axis (+Y)
//   Shift                        faster (FastMultiplier)
//   mouse wheel                  forward / back; while looking: movement speed
//   middle mouse button (drag)   pan
//
//   controller.SetCamera(&camera);
//   ... every frame, before input.EndFrame():
//   controller.Update(input, deltaTime);
//
// Only the position and the front vector of the camera change.
class FreeCameraController
{
public:
    // Units per second.
    EXN_PROPERTY(EXFLOAT, MoveSpeed, fMoveSpeed, 2.0f);
    // Degrees per pixel of mouse movement.
    EXN_PROPERTY(EXFLOAT, LookSensitivity, fLookSensitivity, 0.15f);
    // Speed factor while Shift is held.
    EXN_PROPERTY(EXFLOAT, FastMultiplier, fFastMultiplier, 4.0f);
    // Mouse up looks down.
    EXN_PROPERTY(EXBOOL, InvertY, bInvertY, false);

public:
    // The camera to control (null = none); yaw and pitch start from its front vector.
    void SetCamera(eXngine::Renderers::eXcamera *camera)
    {
        m_pCamera = camera;

        if (camera == EXN_NULL_HANDLE)
            return;

        const EXMATH::vec3 front = camera->GetFront();

        if (EXMATH::length(front) < 1e-6f)
        {
            SetRotation(m_fYaw, m_fPitch);
            return;
        }

        const EXMATH::vec3 direction = EXMATH::normalize(front);
        SetRotation(EXMATH::degrees(std::atan2(direction.z, direction.x)), EXMATH::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f))));
    }

    eXngine::Renderers::eXcamera *GetCamera() const { return m_pCamera; }

    // View direction in degrees: yaw -90 looks down -Z, +90 pitch straight up (kept within
    // +-89, where the front vector is not parallel to the up vector yet).
    void SetRotation(EXFLOAT yaw, EXFLOAT pitch)
    {
        m_fYaw = std::remainder(yaw, 360.0f); // -180..180
        m_fPitch = std::clamp(pitch, -89.0f, 89.0f);

        if (m_pCamera != EXN_NULL_HANDLE)
            m_pCamera->SetFront(FrontFromAngles(m_fYaw, m_fPitch));
    }

    EXFLOAT GetYaw() const { return m_fYaw; }
    EXFLOAT GetPitch() const { return m_fPitch; }

    // The right mouse button turns the camera.
    EXBOOL IsLooking() const { return m_bLooking; }
    // The middle mouse button drags the camera.
    EXBOOL IsPanning() const { return m_bPanning; }

    // Moves and turns the camera with this frame's input. mouseAvailable / keyboardAvailable
    // are false while a UI uses them (ImGui: !io.WantCaptureMouse, !io.WantCaptureKeyboard).
    // A look or pan that already started goes on until its button is released, and the
    // keys always move the camera while looking.
    void Update(eXngine::Input::eXinput &input, EXFLOAT deltaTime, EXBOOL mouseAvailable = true, EXBOOL keyboardAvailable = true)
    {
        using eXngine::Input::eXmouseButton;

        if (m_pCamera == EXN_NULL_HANDLE)
            return;

        // After a long frame (a dragged window, a breakpoint) a held key would jump far.
        deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);

        // Looking and panning start over the scene only and end when their button is
        // released, wherever the mouse is then.
        if (!m_bLooking && !m_bPanning && mouseAvailable && input.IsMousePressed(eXmouseButton::Right))
        {
            m_bLooking = true;
            input.SetCursorLocked(true);
        }
        else if (m_bLooking && !input.IsMouseDown(eXmouseButton::Right))
        {
            m_bLooking = false;
            input.SetCursorLocked(false);
        }

        if (!m_bLooking && !m_bPanning && mouseAvailable && input.IsMousePressed(eXmouseButton::Middle))
            m_bPanning = true;
        else if (m_bPanning && !input.IsMouseDown(eXmouseButton::Middle))
            m_bPanning = false;

        const eXvec2 mouse = input.GetMouseDelta();

        if (m_bLooking)
            SetRotation(m_fYaw + mouse.x * m_fLookSensitivity, m_fPitch + (m_bInvertY ? mouse.y : -mouse.y) * m_fLookSensitivity);

        const EXMATH::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const EXMATH::vec3 front = FrontFromAngles(m_fYaw, m_fPitch);
        const EXMATH::vec3 right = EXMATH::normalize(EXMATH::cross(front, worldUp));
        const EXMATH::vec3 up = EXMATH::cross(right, front);

        const EXBOOL keyboard = keyboardAvailable || m_bLooking;
        const EXFLOAT speed = m_fMoveSpeed * (keyboard && input.IsKeyDown(eXngine::eXkey_Shift) ? m_fFastMultiplier : 1.0f);
        EXMATH::vec3 position = m_pCamera->GetPosition();

        if (keyboard)
        {
            EXMATH::vec3 direction(0.0f);

            if (input.IsKeyDown(eXngine::eXkey_W)) direction += front;
            if (input.IsKeyDown(eXngine::eXkey_S)) direction -= front;
            if (input.IsKeyDown(eXngine::eXkey_D)) direction += right;
            if (input.IsKeyDown(eXngine::eXkey_A)) direction -= right;
            if (input.IsKeyDown(eXngine::eXkey_E)) direction += worldUp;
            if (input.IsKeyDown(eXngine::eXkey_Q)) direction -= worldUp;

            // Diagonals are not faster.
            if (EXMATH::length(direction) > 0.0f)
                position += EXMATH::normalize(direction) * (speed * deltaTime);
        }

        if (const EXFLOAT wheel = input.GetMouseWheel(); wheel != 0.0f)
        {
            if (m_bLooking)
                m_fMoveSpeed = std::clamp(m_fMoveSpeed * std::pow(1.25f, wheel), 0.01f, 1000.0f);
            else if (mouseAvailable)
                position += front * (wheel * speed * 0.1f); // a notch: a tenth of a second of flight
        }

        // Grabs the scene: it follows the mouse, the camera moves the other way.
        if (m_bPanning)
            position += (up * mouse.y - right * mouse.x) * (speed * 0.005f);

        m_pCamera->SetPosition(position);
    }

private:
    eXngine::Renderers::eXcamera *m_pCamera = EXN_NULL_HANDLE;
    EXFLOAT m_fYaw = -90.0f, m_fPitch = 0.0f;
    EXBOOL m_bLooking = false, m_bPanning = false;

    static EXMATH::vec3 FrontFromAngles(EXFLOAT yaw, EXFLOAT pitch)
    {
        const EXFLOAT y = EXMATH::radians(yaw), p = EXMATH::radians(pitch);
        return EXMATH::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p));
    }
};
