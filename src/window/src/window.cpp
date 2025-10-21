#include <windows/window.h>
#include <malloc.h>
#include <string.h>

namespace eXngine::Applications
{
	Application::Application(const char * name, Point position, Size size, bool maximized):
		m_name(const_cast<char*>(name)), m_position(position), m_size(size), maximized(maximized)
	{}
	
	Application::~Application()
	{
		delete this->m_name;
	}
	
	const char* Application::GetName()
	{
		return this->m_name;
	}
	
	eXngine::Size Application::GetSize()
	{
		return this->m_size;
	}
	
	eXngine::Point Application::GetPosition()
	{
		return this->m_position;
	}
	
	bool Application::IsMaximized()
	{
		return this->maximized;
	}
	
	void Application::SetRenderer(AbstractRenderer*renderer)
	{
		this->m_pRenderer = renderer;
	}
	
	void Application::SetSize(Size size)
	{
		this->m_size = size;
	}
	
	void Application::SetPosition(Point position)
	{
		this->m_position = position;
	}
	
	void Application::SetMaximized(bool maximized)
	{
		this->maximized = maximized;
	}
}
