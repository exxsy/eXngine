#include <windows/eXwindow.h>

namespace eXngine::Windows
{
    eXwindow::eXwindow(const EXCHAR *name, Point position, Size size, EXBOOL maximized) : Window(name, position, size, maximized)
    {
        SetInstance(GetModuleHandle(NULL));
        SetExWindowStyle(0);
        SetWindowStyle(WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CAPTION);
    }

    eXwindow::~eXwindow()
    {
        EX_FREE(m_szName);
        EX_FREE(m_szClassName);
    }

    EXBOOL eXwindow::Initialize()
    {
        Register();

        EX_INFO("Windows eXwindow '%s' initialized successfully.", GetName());
        EX_INFO("Native Handle %p", GetHandle());

        return true;
    }

    int eXwindow::Run()
    {
        ShowWindow((HWND)m_pHandle, m_bIsMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL);

        EX_INFO("Entering main application loop.");

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);

            if (m_fnOnLoop)
                m_fnOnLoop(this);

            DispatchMessage(&msg);
        }

        if (m_fnOnCleanup)
            m_fnOnCleanup(this);

        return EXN_SUCCESS;
    }

    EXVOIDPTR eXwindow::GetHandle()
    {
        return m_pHandle;
    }

    void eXwindow::ProcessInput(eXkey key, bool pressed)
    {
        // m_bKeys[key] = pressed;

        EX_INFO("Key %c %s", key, pressed ? "pressed" : "released");
    }

    void eXwindow::Register()
    {
        static auto wndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            // case WM_LBUTTONDOWN:
            // case WM_LBUTTONUP:
            // case WM_RBUTTONDOWN:
            // case WM_RBUTTONUP:
            // case WM_MBUTTONDOWN:
            // case WM_MBUTTONUP:
            // case WM_XBUTTONDOWN:
            // case WM_XBUTTONUP:
            static auto window = reinterpret_cast<eXwindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if (!window) window = reinterpret_cast<eXwindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            switch (uMsg)
            {
            case WM_NCCREATE:
                static LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)createStruct->lpCreateParams);
                return TRUE;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            // case WM_CHAR:
            case WM_UNICHAR:
                if (window)
                {
                    if (wParam == UNICODE_NOCHAR)
                        return DefWindowProc(hwnd, uMsg, wParam, lParam);

                    window->ProcessInput(
                        static_cast<eXkey>(wParam),
                        (
                            uMsg == WM_KEYDOWN || uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
                            uMsg == WM_MBUTTONDOWN || uMsg == WM_SYSKEYDOWN ||
                            uMsg == WM_XBUTTONDOWN || uMsg == WM_CHAR || uMsg == WM_UNICHAR));
                }
                break;
            default:
                break;
            }

            if (window && window->GetWndProcHandler())
                return window->GetWndProcHandler()((HWND)hwnd, uMsg, wParam, lParam);

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };

        WNDCLASS wc = {
            .lpfnWndProc = wndProc,
            .hInstance = this->GetInstance(),
            .lpszClassName = m_szClassName,
        };

        RegisterClass(&wc);

        m_pApplicationData = this;
        m_pHandle = CreateWindowEx(
            this->GetExWindowStyle(),
            this->GetClassNameA(),
            this->GetName(),
            this->GetWindowStyle(),
            m_position.X, m_position.Y, m_size.W, m_size.H,
            this->GetParentWindow(),
            this->GetMenu(),
            this->GetInstance(),
            m_pApplicationData);

        DWORD error = GetLastError();

        EX_FATAL(error == 0, "Failed to create window due to unknown error.");
        EX_FATAL(m_pHandle != EXN_NULL_HANDLE, "Failed to create window.");

        // SetWindowLongPtr((HWND)m_pHandle, GWLP_USERDATA, (LONG_PTR)m_pApplicationData);
    }

}
