#include <malloc.h>
#include <string.h>

#include <windows/window.h>

namespace eXngine::Applications
{
	Application::Application(const EXCHAR *name, Point position, Size size, bool maximized) : m_szPosition(position), m_szSize(size), m_bIsMaximized(maximized)
	{
#ifdef UNICODE
		m_szName = static_cast<EXCHAR *>(EX_ALLOC(wcslen(name) + 1));
		wcscpy_s(m_szName, wcslen(name) + 1, name);
#else
		m_szName = static_cast<EXCHAR *>(EX_ALLOC(strlen(name) + 1));
		strcpy_s(m_szName, strlen(name) + 1, name);
#endif
	}

	Application::~Application()
	{
		EX_FREE(this->m_szName);
	}

	bool Application::IsMaximized()
	{
		return this->m_bIsMaximized;
	}

	void Application::SetSize(Size size)
	{
		this->m_szSize = size;
	}

	void Application::SetPosition(Point position)
	{
		this->m_szPosition = position;
	}

	void Application::SetIsMaximized(bool maximized)
	{
		this->m_bIsMaximized = maximized;
	}

	void Application::SetOnInitializeHandler(OnInitializeHandler handler)
	{
		this->m_fnOnInitialize = handler;
	}

	void Application::SetOnLoopHandler(OnLoopHandler handler)
	{
		this->m_fnOnLoop = handler;
	}

	void Application::SetOnCleanupHandler(OnCleanupHandler handler)
	{
		this->m_fnOnCleanup = handler;
	}

	const EXCHAR *Application::GetName()
	{
		return this->m_szName;
	}

	Size Application::GetSize()
	{
		return this->m_szSize;
	}

	Point Application::GetPosition()
	{
		return this->m_szPosition;
	}

	void *Application::GetHandle()
	{
		return this->m_pHandle;
	}
}
