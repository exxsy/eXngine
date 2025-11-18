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

        return true;
    }

    int eXapplication::Run()
    {
        ShowWindow((HWND)m_pHandle, m_bMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL);

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return EXN_SUCCESS;
    }

    LRESULT eXapplication::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
            return 0;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void eXapplication::RegisterWindow()
    {
        WNDCLASS wc = {
            .hInstance = m_hInstance,
            .lpszClassName = m_szClassName,
            // .lpfnWndProc = WindowProc,
        };

        RegisterClass(&wc);

        m_pHandle = CreateWindowEx(
            m_dwExWindowStyle, // Optional window styles.
            m_szClassName,     // Window class
            m_szName,          // Window text
            m_dwWindowStyle,   // Window style

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

            (HWND)m_pParentWindowHandle, // Parent window
            NULL,                        // Menu
            m_hInstance,                 // Instance handle
            m_pApplicationData           // Additional application data
        );

        EX_FATAL(m_pHandle != EXN_NULL_HANDLE, "Failed to create window.");
    }

}
