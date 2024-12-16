#include "../Applications/Custom/glfw.h"
#include "../Applications/application.h"
#include <cassert>

eXngine::Applications::GLFWApplication::
	GLFWApplication(const char* name, Point position, Size size, bool maximized): 
	Application(name, position, size, maximized), m_window(nullptr) 
{ 
	assert(Initialize());
}

bool eXngine::Applications::GLFWApplication::Initialize()
{
	assert(glfwVulkanSupported() == GLFW_FALSE);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, (int)IsMaximized());
	glfwWindowHint(GLFW_POSITION_X, (int)GetPosition().X);
	glfwWindowHint(GLFW_POSITION_Y, (int)GetPosition().Y);

	m_window = glfwCreateWindow(GetSize().W, GetSize().H, 
		GetName(), nullptr, nullptr);

	assert(m_window != nullptr);

	glfwSetWindowUserPointer(m_window, m_pRenderer);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
	{
		if (Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window)))
		{
			renderer->SetFrameBufferResize(true);
		}
	});

	return true;
}

int eXngine::Applications::GLFWApplication::Run()
{
	return Loop();
}

GLFWwindow* eXngine::Applications::GLFWApplication::GetWindow()
{
	return m_window;
}

Size eXngine::Applications::GLFWApplication::GetFrameBufferSize()
{
	Size sz(0, 0);
	glfwGetFramebufferSize(this->m_window, &sz.W, &sz.H);

	return sz;
}

std::vector<const char*> eXngine::Applications::GLFWApplication::GetExtensions()
{
	if (!glfwVulkanSupported()) return { };

	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	return std::vector<const char*>(extensions, extensions + extensionCount);
}

void eXngine::Applications::GLFWApplication::SetKeyboardHandler(GLFWKeyboardCallback handler)
{
	assert(m_window != nullptr);
	assert(handler != nullptr);

	m_keyboard = &handler;

	glfwSetKeyCallback(m_window, handler);
}

void eXngine::Applications::GLFWApplication::SetCharacterHandler(GLFWCharacterCallback handler)
{
	assert(m_window != nullptr);
	assert(handler != nullptr);

	m_character = &handler;

	glfwSetCharCallback(m_window, handler);
}

void eXngine::Applications::GLFWApplication::SetMousePosHandler(GLFWMousePosCallback handler)
{
	assert(m_window != nullptr);
	assert(handler != nullptr);

	m_mousePos = &handler;

	glfwSetCursorPosCallback(m_window, handler);
}

void eXngine::Applications::GLFWApplication::SetMouseClickHandler(GLFWMouseClickCallback handler)
{
	assert(m_window != nullptr);
	assert(handler != nullptr);

	m_mouseClick = &handler;

	glfwSetMouseButtonCallback(m_window, handler);
}

int eXngine::Applications::GLFWApplication::Loop()
{
	assert(m_pRenderer->Initialize());

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		if (m_pRenderer) m_pRenderer->OnFrame();
	}

	if (m_pRenderer) m_pRenderer->OnExit();
	glfwDestroyWindow(m_window);
	glfwTerminate();

	return EXN_SUCCESS;
}
