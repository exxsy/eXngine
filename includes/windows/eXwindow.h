#pragma once

#include <windows.h>
#include <concepts>
#include <cassert>

#include <eXngine.h>
#include <eXtypes.h>
#include <eXdebug.h>
#include <windows/window.h>

namespace eXngine::Applications
{
#ifdef _WIN32
    typedef HWND EXWND;
#endif
    // template <std::derived_from<sWindowData> T>B
    class EXNEXPORT eXapplication : public Application
    {
    public:
        eXapplication(const EXCHAR *name, Point position, Size size, bool maximized);
        ~eXapplication();

        virtual bool Initialize() override;
        virtual int Run() override;
        virtual void ProcessInput(eXkey, bool) override;
        virtual void *GetHandle() override { return m_pHandle; }

        void SetClassName(const EXCHAR *);
        void SetInstance(HINSTANCE hInstance) { m_hInstance = hInstance; }

    private:
        EXCHAR m_szClassName[256] = {0};
        HINSTANCE m_hInstance = EXN_NULL_HANDLE;
        EXVOIDPTR m_pParentWindowHandle = EXN_NULL_HANDLE;
        EXVOIDPTR m_pApplicationData = EXN_NULL_HANDLE;
        EXDWORD m_dwExWindowStyle = 0;
        EXDWORD m_dwWindowStyle = WS_OVERLAPPEDWINDOW;

        void RegisterWindow();
    };
}