#include <malloc.h>
#include <string.h>

#include <windows/window.h>

namespace eXngine::Applications
{
	Application::Application(const char * name, Point position, Size size, bool maximized) : 
		m_szName(const_cast<char*>(name)), m_szPosition(position), m_szSize(size), m_bMaximized(maximized)
	{
		name = static_cast<EXINT8 *>(EX_ALLOC(strlen(name) + 1));
		strcpy_s(const_cast<char *>(name), strlen(name) + 1, name);
	}
	
	Application::~Application()
	{
		EX_FREE(this->m_szName);
	}
	
	bool Application::IsMaximized()
	{
		return this->m_bMaximized;
	}
	
	void Application::SetSize(Size size)
	{
		this->m_szSize = size;
	}
	
	void Application::SetPosition(Point position)
	{
		this->m_szPosition = position;
	}
	
	void Application::SetMaximized(bool maximized)
	{
		this->m_bMaximized = maximized;
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

	const char* Application::GetName()
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

	EXUINTPTR Application::GetInstance()
	{
		return this->m_pInstance;
	}
}

