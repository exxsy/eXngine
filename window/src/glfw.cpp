#include <windows/glfw.h>
#include <windows/window.h>
#include <cassert>

#pragma comment(lib, "glfw3.lib")

eXngine::Applications::GLFWApplication::
	GLFWApplication(const char* name, Point position, Size size, bool maximized): 
	Application(name, position, size, maximized), m_pWindow(nullptr) 
{ 
	assert(Initialize());
}

bool eXngine::Applications::GLFWApplication::Initialize()
{
	// assert(glfwVulkanSupported() == GLFW_FALSE);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, (int)IsMaximized());
	glfwWindowHint(GLFW_POSITION_X, (int)GetPosition().X);
	glfwWindowHint(GLFW_POSITION_Y, (int)GetPosition().Y);

	m_pWindow = glfwCreateWindow(GetSize().W, GetSize().H, GetName(), nullptr, nullptr);

	assert(m_pWindow != nullptr);

	glfwSetWindowUserPointer(m_pWindow, m_pRenderer.value());
	glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* window, int width, int height)
	{
		if (BaseRenderer* renderer = reinterpret_cast<BaseRenderer*>(glfwGetWindowUserPointer(window)))
		{
			//renderer->SetFrameBufferSize(Size(width, height));
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
	return m_pWindow;
}

Size eXngine::Applications::GLFWApplication::GetFrameBufferSize()
{
	Size sz(0, 0);
	glfwGetFramebufferSize(this->m_pWindow, &sz.W, &sz.H);

	return sz;
}

std::vector<const char*> eXngine::Applications::GLFWApplication::GetExtensions()
{
	// if (!glfwVulkanSupported()) return { };

	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	return std::vector<const char*>(extensions, extensions + extensionCount);
}

void eXngine::Applications::GLFWApplication::SetKeyboardHandler(GLFWKeyboardCallback handler)
{
	assert(m_pWindow != nullptr);
	assert(handler != nullptr);

	m_keyboard = &handler;

	glfwSetKeyCallback(m_pWindow, handler);
}

void eXngine::Applications::GLFWApplication::SetCharacterHandler(GLFWCharacterCallback handler)
{
	assert(m_pWindow != nullptr);
	assert(handler != nullptr);

	m_character = &handler;

	glfwSetCharCallback(m_pWindow, handler);
}

void eXngine::Applications::GLFWApplication::SetMousePosHandler(GLFWMousePosCallback handler)
{
	assert(m_pWindow != nullptr);
	assert(handler != nullptr);

	m_mousePos = &handler;

	glfwSetCursorPosCallback(m_pWindow, handler);
}

void eXngine::Applications::GLFWApplication::SetMouseClickHandler(GLFWMouseClickCallback handler)
{
	assert(m_pWindow != nullptr);
	assert(handler != nullptr);

	m_mouseClick = &handler;

	glfwSetMouseButtonCallback(m_pWindow, handler);
}

int eXngine::Applications::GLFWApplication::Loop()
{
	assert(m_pRenderer.has_value());

	auto & pRenderer = m_pRenderer.value();
	pRenderer->Initialize();

	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();

		pRenderer->OnRender();
	}

	pRenderer->OnExit();
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();

	return EXN_SUCCESS;
}
