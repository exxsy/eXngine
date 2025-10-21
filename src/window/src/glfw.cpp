#include <windows/glfw.h>
#include <windows/window.h>
#include <cassert>

namespace eXngine::Applications
{
	GLFWApplication::GLFWApplication(const char *name, Point position, Size size, bool maximized) : 
		Application(name, position, size, maximized), m_pWindow(nullptr)
	{
		assert(Initialize());
	}

	bool GLFWApplication::Initialize()
	{
#ifdef GLFW_INCLUDE_VULKAN
		assert(glfwVulkanSupported() == GLFW_FALSE);
#endif

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, (int)IsMaximized());
		glfwWindowHint(GLFW_POSITION_X, (int)GetPosition().X);
		glfwWindowHint(GLFW_POSITION_Y, (int)GetPosition().Y);

		m_pWindow = glfwCreateWindow(GetSize().W, GetSize().H, GetName(), nullptr, nullptr);

		assert(m_pWindow != nullptr);

		glfwSetWindowUserPointer(m_pWindow, m_pRenderer.value());

		return true;
	}

	int GLFWApplication::Run()
	{
		return Loop();
	}

	GLFWwindow *GLFWApplication::GetWindow()
	{
		return m_pWindow;
	}

	Size GLFWApplication::GetFrameBufferSize()
	{
		Size sz(0, 0);
		glfwGetFramebufferSize(this->m_pWindow, &sz.W, &sz.H);

		return sz;
	}

	std::vector<const char *> GLFWApplication::GetExtensions()
	{
#ifdef GLFW_INCLUDE_VULKAN
		if (!glfwVulkanSupported())
			return {};
#endif

		uint32_t extensionCount = 0;
		const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

		return std::vector<const char *>(extensions, extensions + extensionCount);
	}

	void GLFWApplication::SetKeyboardHandler(GLFWKeyboardCallback handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnKeyboard = &handler;

		glfwSetKeyCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetScrollHandler(GLFWScrollCallback handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnScroll = &handler;

		glfwSetScrollCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetCharacterHandler(GLFWCharacterCallback handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnCharacter = &handler;

		glfwSetCharCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetMousePosHandler(GLFWMousePosCallback handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnMousePos = &handler;

		glfwSetCursorPosCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetMouseClickHandler(GLFWMouseClickCallback handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnMouseClick = &handler;

		glfwSetMouseButtonCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetFramebufferSizeHandler(GLFWframebuffersizefun handler)
	{
		assert(m_pWindow != nullptr);
		assert(handler != nullptr);

		m_fnFramebufferSize = &handler;

		glfwSetFramebufferSizeCallback(m_pWindow, handler);
	}

	int GLFWApplication::Loop()
	{
		assert(m_pRenderer.has_value());

		auto & pRenderer = m_pRenderer.value();

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
}