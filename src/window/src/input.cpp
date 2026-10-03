#include <windows.h>
#include <windowsx.h>

#include <input/input.h>

namespace eXngine::Input
{
    namespace
    {
        EXBOOL ValidKey(EXINT32 key) { return key > 0 && key < eXinput::kKeyCount; }
    }

    EXBOOL eXinput::HandleMessage(EXVOIDPTR window, EXUINT32 message, EXUINT64 wParam, EXINT64 lParam)
    {
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
        case WM_KILLFOCUS:
            Reset();
            return false;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                LPCTSTR shape = IDC_ARROW;
                switch (m_eCursor)
                {
                case eXcursor::Hand: shape = IDC_HAND; break;
                case eXcursor::Crosshair: shape = IDC_CROSS; break;
                case eXcursor::IBeam: shape = IDC_IBEAM; break;
                default: break;
                }
                ::SetCursor(LoadCursor(nullptr, shape));
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
