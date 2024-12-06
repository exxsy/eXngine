#include "glfw.h"
#include <cassert>

eXngine::Applications::GLFWApplication::
	GLFWApplication(const char* name, Point position, Size size, bool maximized): 
	Application(name, position, size, maximized), m_window(nullptr) { }

bool eXngine::Applications::GLFWApplication::Initialize()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, (int)this->IsMaximized());
	glfwWindowHint(GLFW_POSITION_X, (int)this->GetPosition().X);
	glfwWindowHint(GLFW_POSITION_Y, (int)this->GetPosition().Y);

	this->m_window = glfwCreateWindow(this->GetSize().W, this->GetSize().H, 
		this->GetName(), nullptr, nullptr);

	assert(this->m_window != nullptr);

	return true;
}

int eXngine::Applications::GLFWApplication::Run()
{
	bool initialized = this->Initialize();
	return this->Loop();
}

int eXngine::Applications::GLFWApplication::Loop()
{
	while (!glfwWindowShouldClose(this->m_window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(this->m_window);

	return 0;
}
