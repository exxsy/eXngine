#include <windows/glfw.h>

namespace eXngine::Windows
{
	GLFWWindow::GLFWWindow(const EXCHAR *name, eXvec2 position, eXvec2 size, bool maximized) : Window(name, position, size, maximized), m_pWindow(nullptr)
	{
	}

	bool GLFWWindow::Initialize()
	{
		EX_FATAL(glfwInit() != GLFW_FALSE, "Failed to initialize GLFW.");
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, (int)this->GetIsMaximized());
		glfwWindowHint(GLFW_POSITION_X, (int)m_position.x);
		glfwWindowHint(GLFW_POSITION_Y, (int)m_position.y);

		int count;
		GLFWmonitor **monitors = glfwGetMonitors(&count);
		m_pPrimaryMonitor = glfwGetPrimaryMonitor();
		EX_FATAL(count != 0, "No GLFW monitors found.");

		m_vMonitors.resize(count);
		m_vMonitors = std::vector<GLFWmonitor *>(monitors, monitors + count);

		if (m_pPrimaryMonitor == EXN_NULL_HANDLE)
			m_pPrimaryMonitor = *m_vMonitors.begin();

		char name[256] = {0};

#ifdef UNICODE
		size_t out_size;
		wcstombs_s(&out_size, name, (const wchar_t *)this->m_szName, sizeof(name));
#else
		strncpy_s(name, this->m_szName, sizeof(name) - 1);
#endif

		m_pWindow = glfwCreateWindow(m_size.x, m_size.y, name,
									 GetIsMaximized() ? m_pPrimaryMonitor : EXN_NULL_HANDLE, EXN_NULL_HANDLE);

		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Failed to create GLFW window.");
		EX_INFO("GLFW window '%s' initialized successfully.", GetName());
		EX_INFO("Native Handle %p", GetHandle());

		return true;
	}

	EXINT GLFWWindow::Run()
	{
		if (m_fnOnInitialize != EXN_NULL_HANDLE)
			m_fnOnInitialize(this->m_pHandle);

		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();

			if (m_fnOnLoop != EXN_NULL_HANDLE)
				m_fnOnLoop(this->m_pHandle);
		}

		if (m_fnOnCleanup != EXN_NULL_HANDLE)
			m_fnOnCleanup(this->m_pHandle);

		glfwDestroyWindow(m_pWindow);
		glfwTerminate();

		EX_INFO("GLFW window '%s' terminated successfully.", GetName());

		return EXN_SUCCESS;
	}

	GLFWwindow *GLFWWindow::GetWindow()
	{
		return m_pWindow;
	}

	std::vector<const char *> GLFWWindow::GetExtensions()
	{
#ifdef GLFW_INCLUDE_VULKAN
		if (!glfwVulkanSupported())
			return {};
#endif

		uint32_t extensionCount = 0;
		const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

		return std::vector<const char *>(extensions, extensions + extensionCount);
	}

	void GLFWWindow::SetKeyboardHandler(GLFWKeyboardCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid keyboard handler.");

		m_fnKeyboard = &handler;

		glfwSetKeyCallback(m_pWindow, handler);
	}

	void GLFWWindow::SetScrollHandler(GLFWScrollCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid scroll handler.");

		m_fnScroll = &handler;

		glfwSetScrollCallback(m_pWindow, handler);
	}

	void GLFWWindow::SetCharacterHandler(GLFWCharacterCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid character handler.");

		m_fnCharacter = &handler;

		glfwSetCharCallback(m_pWindow, handler);
	}

	void GLFWWindow::SetMousePosHandler(GLFWMousePosCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid mouse position handler.");

		m_fnMousePos = &handler;

		glfwSetCursorPosCallback(m_pWindow, handler);
	}

	void GLFWWindow::SetMouseClickHandler(GLFWMouseClickCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid mouse click handler.");

		m_fnMouseClick = &handler;

		glfwSetMouseButtonCallback(m_pWindow, handler);
	}

	void GLFWWindow::SetFramebufferSizeHandler(GLFWframebuffersizefun handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid framebuffer size handler.");

		m_fnFramebufferSize = &handler;

		glfwSetFramebufferSizeCallback(m_pWindow, handler);
	}

	void *GLFWWindow::GetHandle()
	{
#ifdef GLFW_EXPOSE_NATIVE_COCOA
		m_pHandle = reinterpret_cast<EXVOIDPTR>(glfwGetCocoaWindow(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_WAYLAND
		m_pHandle = reinterpret_cast<EXVOIDPTR>(glfwGetWaylandWindow(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_X11
		m_pHandle = reinterpret_cast<EXVOIDPTR>(glfwGetX11Window(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_WIN32
		m_pHandle = reinterpret_cast<EXVOIDPTR>(glfwGetWin32Window(m_pWindow));
#else
		EX_FATAL(false, "Native window handle retrieval not supported on this platform.");
#endif

		EX_FATAL(m_pHandle != EXN_NULL_HANDLE, "Failed to get native window handle.");

		return m_pHandle;
	}

	void GLFWWindow::ProcessInput(eXkey key, bool pressed)
	{
		m_bKeys[key] = pressed;
	}
}
