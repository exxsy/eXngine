#include <malloc.h>
#include <string.h>

#include <windows/window.h>
#include <types/vector.h>

using namespace eXngine::Types;

namespace eXngine::Windows
{
	Window::Window(const EXCHAR *name, eXvec2 position, eXvec2 size, bool maximized) : m_position(position), m_size(size), m_bIsMaximized(maximized)
	{
		this->SetName(const_cast<EXCHAR *>(name));
	}

	Window::Window(const EXCHAR *name, eXvec2 position, eXvec2 size) : m_position(position), m_size(size), m_bIsMaximized(false)
	{
		this->SetName(const_cast<EXCHAR *>(name));
	}

	Window::~Window()
	{
		EX_FREE(this->m_szName);
	}

	void Window::SetOnInitializeHandler(OnInitializeHandler handler)
	{
		this->m_fnOnInitialize = handler;
	}

	void Window::SetOnLoopHandler(OnLoopHandler handler)
	{
		this->m_fnOnLoop = handler;
	}

	void Window::SetOnCleanupHandler(OnCleanupHandler handler)
	{
		this->m_fnOnCleanup = handler;
	}

	void Window::SetOnResizeHandler(OnResizeHandler handler)
	{
		this->m_fnOnResize = handler;
	}
}
