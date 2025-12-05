#include <windows/eXwindow.h>

namespace eXngine::Applications
{
    eXapplication::eXapplication(const EXCHAR *name, Point position, Size size, bool maximized) : Application(name, position, size, maximized) {}
    eXapplication::~eXapplication()
    {
        EX_FREE(m_szName);
        EX_FREE(m_szClassName);
    }

    bool eXapplication::Initialize()
    {
        RegisterWindow();

        EX_INFO("Windows eXapplication '%s' initialized successfully.", GetName());
        EX_INFO("Native Handle %p", GetHandle());

        return true;
    }

    int eXapplication::Run()
    {
        ShowWindow((HWND)m_pHandle, m_bIsMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL);

        EX_INFO("Entering main application loop.");

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return EXN_SUCCESS;
    }

    void eXapplication::ProcessInput(eXkey key, bool pressed)
    {
        // m_bKeys[key] = pressed;

        EX_INFO("Key %c %s", key, pressed ? "pressed" : "released");
    }

    void eXapplication::SetClassName(const EXCHAR *className)
    {
        EX_FATAL(className != EXN_NULL_HANDLE, "Invalid class name.");
        strncpy_s(m_szClassName, className, sizeof(m_szClassName) - 1);
    }

    void eXapplication::RegisterWindow()
    {
        static auto proc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            // case WM_LBUTTONDOWN:
            // case WM_LBUTTONUP:
            // case WM_RBUTTONDOWN:
            // case WM_RBUTTONUP:
            // case WM_MBUTTONDOWN:
            // case WM_MBUTTONUP:
            // case WM_XBUTTONDOWN:
            // case WM_XBUTTONUP:

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
            case WM_CHAR:
            case WM_UNICHAR:
                static auto window = reinterpret_cast<eXapplication *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

                if (window)
                {
                    bool m_bPressed =
                        (uMsg == WM_KEYDOWN || uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
                         uMsg == WM_MBUTTONDOWN || uMsg == WM_SYSKEYDOWN ||
                         uMsg == WM_XBUTTONDOWN || uMsg == WM_CHAR || uMsg == WM_UNICHAR);

                    if (wParam == UNICODE_NOCHAR)
                        return DefWindowProc(hwnd, uMsg, wParam, lParam);

                    window->ProcessInput(
                        static_cast<eXkey>(wParam),
                        m_bPressed);
                }
                break;
            default:
                break;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };

        if (m_szClassName[0] == '\0')
            strncpy_s(m_szClassName, m_szName, sizeof(m_szName) - 1);

        WNDCLASS wc = {
            .lpfnWndProc = proc,
            .hInstance = m_hInstance,
            .lpszClassName = m_szClassName,
        };

        RegisterClass(&wc);

        m_pApplicationData = this; // Pass 'this' pointer to window data
        m_pHandle = CreateWindowEx(
            m_dwExWindowStyle, // Optional window styles.
            m_szClassName,     // Window class
            m_szName,          // Window text
            m_dwWindowStyle,   // Window style
            m_szPosition.X, m_szPosition.Y, m_szSize.W, m_szSize.H,
            (HWND)m_pParentWindowHandle, // Parent window
            NULL,                        // Menu
            m_hInstance,                 // Instance handle
            EXN_NULL_HANDLE);

        EX_FATAL(m_pHandle != EXN_NULL_HANDLE, "Failed to create window.");

        SetWindowLongPtr((HWND)m_pHandle, GWLP_USERDATA, (LONG_PTR)m_pApplicationData);
    }

}
