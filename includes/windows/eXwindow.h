#pragma once

#include <windows.h>
#include <concepts>
#include <cassert>

#include <eXngine.h>
#include <eXtypes.h>
#include <eXdebug.h>
#include <windows/window.h>

namespace eXngine::Windows
{
#ifdef _WIN32
    typedef HWND EXWND;
    typedef LRESULT (*WINAPI EXPROC)(EXWND, EXUINT, WPARAM, LPARAM);
#endif
    // template <std::derived_from<sWindowData> T>B
    class EXNEXPORT eXwindow : public Window
    {
    public:
        eXwindow(const EXCHAR *name, Point position, Size size, bool maximized);
        ~eXwindow();

        virtual EXBOOL Initialize() override;
        virtual EXINT Run() override;
        virtual EXVOIDPTR GetHandle() override;
        virtual void ProcessInput(eXkey, bool) override;

    private:
        EXN_PROPERTY(HINSTANCE, Instance, hInstance, EXN_NULL_HANDLE);
        EXN_PROPERTY(EXWND, ParentWindow, pParentWindowHandle, EXN_NULL_HANDLE);
        EXN_PROPERTY(HMENU, Menu, hMenu, EXN_NULL_HANDLE);
        EXN_PROPERTY(EXPROC, WndProcHandler, pWndProcHandler, EXN_NULL_HANDLE);
        EXN_PROPERTY(EXDWORD, ExWindowStyle, dwExWindowStyle, 0);
        EXN_PROPERTY(EXDWORD, WindowStyle, dwWindowStyle, WS_OVERLAPPEDWINDOW);
        EXN_PROPERTY(EXCHAR *, ClassName, szClassName, EXN_NULL_HANDLE);

    private:
        EXVOIDPTR m_pApplicationData = EXN_NULL_HANDLE;
        EXWND m_pHandle = EXN_NULL_HANDLE;

        void Register();
    };
}