#include <windows/window.h>
#include <malloc.h>
#include <string.h>

eXngine::Applications::Application::Application(const char * name, Point position, Size size, bool maximized):
	m_name(const_cast<char*>(name)), m_position(position), m_size(size), maximized(maximized)
{}

eXngine::Applications::Application::~Application()
{
	//delete this->m_name;
}

const char* eXngine::Applications::Application::GetName()
{
	return this->m_name;
}

eXngine::Size eXngine::Applications::Application::GetSize()
{
	return this->m_size;
}

eXngine::Point eXngine::Applications::Application::GetPosition()
{
	return this->m_position;
}

bool eXngine::Applications::Application::IsMaximized()
{
	return this->maximized;
}

void eXngine::Applications::Application::SetRenderer(BaseRenderer*renderer)
{
	this->m_pRenderer = renderer;
}

void eXngine::Applications::Application::SetSize(Size size)
{
	this->m_size = size;
}

void eXngine::Applications::Application::SetPosition(Point position)
{
	this->m_position = position;
}

void eXngine::Applications::Application::SetMaximized(bool maximized)
{
	this->maximized = maximized;
}
