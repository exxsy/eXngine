#pragma once

#include <array>
#include <vector>

#include <eXngine.h>
#include <types/vector.h>

namespace eXngine::Input
{
    enum class eXmouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2,
    };

    enum class eXcursor
    {
        Arrow,
        Hand,
        Crosshair,
        IBeam,
    };

    // Keyboard and mouse state of a window, polled once per frame:
    //   window->SetInput(&input);                 // the window feeds it its messages
    //   ... every frame:
    //   if (input.IsKeyPressed(eXkey_Space)) ...  // went down since the last EndFrame()
    //   if (input.IsMouseDown(eXmouseButton::Left)) ...
    //   look += input.GetMouseDelta();            // raw mouse movement, e.g. for a camera
    //   input.EndFrame();                         // at the end of the frame
    // Key codes are Windows virtual-key codes (eXkey in eXtypes.h). A key pressed and released
    // within one frame still reports IsKeyPressed and IsKeyReleased in that frame.
    class EXNEXPORT eXinput
    {
    public:
        static constexpr EXINT32 kKeyCount = 256;
        static constexpr EXINT32 kButtonCount = 3;

        // Feeds a window message (HWND, UINT, WPARAM, LPARAM). True if it was an input message.
        EXBOOL HandleMessage(EXVOIDPTR window, EXUINT32 message, EXUINT64 wParam, EXINT64 lParam);
        // Forgets the edges (pressed / released), the wheel and the key queue of this frame.
        void EndFrame();
        // Everything up, e.g. when the window loses the focus.
        void Reset();

        EXBOOL IsKeyDown(EXINT32 key) const;
        EXBOOL IsKeyPressed(EXINT32 key) const;
        EXBOOL IsKeyReleased(EXINT32 key) const;
        // Keys that went down this frame, oldest first: one per call, 0 when there is none left.
        EXINT32 GetKeyPressed();

        EXBOOL IsMouseDown(eXmouseButton) const;
        EXBOOL IsMousePressed(eXmouseButton) const;
        EXBOOL IsMouseReleased(eXmouseButton) const;
        // Client area pixels, from the top-left corner.
        Types::eXvec2 GetMousePosition() const { return Types::eXvec2(m_fMouseX, m_fMouseY); }
        // Wheel notches turned this frame (positive = away from the user).
        EXFLOAT GetMouseWheel() const { return m_fWheel; }
        // Mouse movement this frame (right / down = positive), from raw input: no pointer
        // acceleration, and it keeps counting at the screen edge and while the cursor is locked.
        Types::eXvec2 GetMouseDelta() const { return Types::eXvec2(m_fDeltaX, m_fDeltaY); }

        // Cursor shown over the client area of the window.
        void SetCursor(eXcursor cursor) { m_eCursor = cursor; }
        eXcursor GetCursor() const { return m_eCursor; }
        // Hides the cursor and holds it where it is (mouse look); GetMouseDelta still changes.
        // Unlocked by itself when the window loses the focus.
        void SetCursorLocked(EXBOOL locked);
        EXBOOL IsCursorLocked() const { return m_bCursorLocked; }

    private:
        std::array<EXBOOL, kKeyCount> m_Keys{}, m_KeysPressed{}, m_KeysReleased{};
        std::array<EXBOOL, kButtonCount> m_Buttons{}, m_ButtonsPressed{}, m_ButtonsReleased{};
        std::vector<EXINT32> m_KeyQueue;
        EXFLOAT m_fMouseX = 0.0f, m_fMouseY = 0.0f, m_fWheel = 0.0f;
        EXFLOAT m_fDeltaX = 0.0f, m_fDeltaY = 0.0f;
        // Last absolute raw position (remote desktop, tablets), turned into a delta.
        EXFLOAT m_fRawX = 0.0f, m_fRawY = 0.0f;
        EXBOOL m_bHasRawPosition = false;
        // Window the raw mouse input (WM_INPUT) is registered for.
        EXVOIDPTR m_pRawInputWindow = nullptr;
        eXcursor m_eCursor = eXcursor::Arrow;
        EXBOOL m_bCursorLocked = false;
    };
}
