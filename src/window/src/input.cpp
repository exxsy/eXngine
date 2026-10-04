#include <windows.h>
#include <windowsx.h>

#include <input/input.h>
#include <eXdebug.h>

namespace eXngine::Input
{
    namespace
    {
        EXBOOL ValidKey(EXINT32 key) { return key > 0 && key < eXinput::kKeyCount; }

        // Shows `cursor`, or no cursor at all while it is locked.
        void ApplyCursor(eXcursor cursor, EXBOOL locked)
        {
            if (locked)
            {
                ::SetCursor(nullptr);
                return;
            }

            LPCTSTR shape = IDC_ARROW;
            switch (cursor)
            {
            case eXcursor::Hand: shape = IDC_HAND; break;
            case eXcursor::Crosshair: shape = IDC_CROSS; break;
            case eXcursor::IBeam: shape = IDC_IBEAM; break;
            default: break;
            }
            ::SetCursor(LoadCursor(nullptr, shape));
        }
    }

    EXBOOL eXinput::HandleMessage(EXVOIDPTR window, EXUINT32 message, EXUINT64 wParam, EXINT64 lParam)
    {
        // Raw mouse movement (WM_INPUT, for GetMouseDelta) is sent only to a registered window:
        // register the window that passes its messages here.
        if (window != nullptr && window != m_pRawInputWindow)
        {
            m_pRawInputWindow = window;

            const RAWINPUTDEVICE mouse{
                .usUsagePage = 0x01, // generic desktop controls
                .usUsage = 0x02,     // mouse
                .dwFlags = 0,        // only while the window is in the foreground
                .hwndTarget = static_cast<HWND>(window),
            };

            if (!RegisterRawInputDevices(&mouse, 1, sizeof(mouse)))
                EX_WARNING("Raw mouse input is not available (error %lu): the mouse delta stays zero.", GetLastError());
        }

        auto button = [&](EXINT32 index, EXBOOL down)
        {
            if (down && !m_Buttons[index])
                m_ButtonsPressed[index] = true;
            if (!down && m_Buttons[index])
                m_ButtonsReleased[index] = true;
            m_Buttons[index] = down;
            m_fMouseX = static_cast<EXFLOAT>(GET_X_LPARAM(lParam));
            m_fMouseY = static_cast<EXFLOAT>(GET_Y_LPARAM(lParam));
            // Keep getting the mouse while a button is held, also outside the window (drags).
            if (down)
                SetCapture(static_cast<HWND>(window));
            else if (!m_Buttons[0] && !m_Buttons[1] && !m_Buttons[2])
                ReleaseCapture();
        };

        switch (message)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const EXINT32 key = static_cast<EXINT32>(wParam);
            if (!ValidKey(key))
                return false;
            const EXBOOL down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            if (down && !m_Keys[key])
            {
                m_KeysPressed[key] = true;
                m_KeyQueue.push_back(key);
            }
            if (!down && m_Keys[key])
                m_KeysReleased[key] = true;
            m_Keys[key] = down;
            // Alt and F10 would open the (missing) window menu and freeze the loop; Alt+F4 still closes.
            return (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && key != VK_F4;
        }
        case WM_LBUTTONDOWN: button(0, true); return true;
        case WM_LBUTTONUP: button(0, false); return true;
        case WM_RBUTTONDOWN: button(1, true); return true;
        case WM_RBUTTONUP: button(1, false); return true;
        case WM_MBUTTONDOWN: button(2, true); return true;
        case WM_MBUTTONUP: button(2, false); return true;
        case WM_MOUSEMOVE:
            m_fMouseX = static_cast<EXFLOAT>(GET_X_LPARAM(lParam));
            m_fMouseY = static_cast<EXFLOAT>(GET_Y_LPARAM(lParam));
            return true;
        case WM_MOUSEWHEEL:
            m_fWheel += static_cast<EXFLOAT>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return true;
        case WM_INPUT:
        {
            RAWINPUT raw{};
            UINT size = sizeof(raw);

            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
                raw.header.dwType != RIM_TYPEMOUSE)
                return false;

            const RAWMOUSE &mouse = raw.data.mouse;

            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                // Remote desktop, pen tablets, virtual machines: 0..65535 across the screen.
                const EXBOOL desktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
                const EXFLOAT x = mouse.lLastX / 65535.0f * GetSystemMetrics(desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                const EXFLOAT y = mouse.lLastY / 65535.0f * GetSystemMetrics(desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

                if (m_bHasRawPosition)
                {
                    m_fDeltaX += x - m_fRawX;
                    m_fDeltaY += y - m_fRawY;
                }
                m_fRawX = x;
                m_fRawY = y;
                m_bHasRawPosition = true;
            }
            else
            {
                m_fDeltaX += static_cast<EXFLOAT>(mouse.lLastX);
                m_fDeltaY += static_cast<EXFLOAT>(mouse.lLastY);
            }
            // eXwindow still passes it on: DefWindowProc has to clean up after WM_INPUT.
            return true;
        }
        case WM_KILLFOCUS:
            Reset();
            SetCursorLocked(false);
            return false;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                ApplyCursor(m_eCursor, m_bCursorLocked);
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    void eXinput::EndFrame()
    {
        m_KeysPressed.fill(false);
        m_KeysReleased.fill(false);
        m_ButtonsPressed.fill(false);
        m_ButtonsReleased.fill(false);
        m_KeyQueue.clear();
        m_fWheel = 0.0f;
        m_fDeltaX = 0.0f;
        m_fDeltaY = 0.0f;
    }

    void eXinput::SetCursorLocked(EXBOOL locked)
    {
        if (locked == m_bCursorLocked)
            return;

        m_bCursorLocked = locked;

        if (locked)
        {
            // A one pixel clip rectangle holds the cursor in place; raw input still reports
            // the movement of the mouse.
            POINT point{};
            GetCursorPos(&point);
            const RECT clip{point.x, point.y, point.x + 1, point.y + 1};
            ClipCursor(&clip);
        }
        else
        {
            ClipCursor(nullptr);
        }

        // Now: the window does not get WM_SETCURSOR while it captures the mouse (a held button).
        ApplyCursor(m_eCursor, locked);
    }

    void eXinput::Reset()
    {
        for (EXINT32 key = 0; key < kKeyCount; ++key)
            if (m_Keys[key])
                m_KeysReleased[key] = true;
        for (EXINT32 b = 0; b < kButtonCount; ++b)
            if (m_Buttons[b])
                m_ButtonsReleased[b] = true;
        m_Keys.fill(false);
        m_Buttons.fill(false);
    }

    EXBOOL eXinput::IsKeyDown(EXINT32 key) const { return ValidKey(key) && m_Keys[key]; }
    EXBOOL eXinput::IsKeyPressed(EXINT32 key) const { return ValidKey(key) && m_KeysPressed[key]; }
    EXBOOL eXinput::IsKeyReleased(EXINT32 key) const { return ValidKey(key) && m_KeysReleased[key]; }

    EXINT32 eXinput::GetKeyPressed()
    {
        if (m_KeyQueue.empty())
            return 0;
        const EXINT32 key = m_KeyQueue.front();
        m_KeyQueue.erase(m_KeyQueue.begin());
        return key;
    }

    EXBOOL eXinput::IsMouseDown(eXmouseButton b) const { return m_Buttons[static_cast<EXINT32>(b)]; }
    EXBOOL eXinput::IsMousePressed(eXmouseButton b) const { return m_ButtonsPressed[static_cast<EXINT32>(b)]; }
    EXBOOL eXinput::IsMouseReleased(eXmouseButton b) const { return m_ButtonsReleased[static_cast<EXINT32>(b)]; }
}
