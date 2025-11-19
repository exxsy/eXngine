#include <windows/glfw.h>

namespace eXngine::Applications
{
	GLFWApplication::GLFWApplication(const EXCHAR *name, Point position, Size size, bool maximized) : Application(name, position, size, maximized), m_pWindow(nullptr)
	{
	}

	bool GLFWApplication::Initialize()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, (int)IsMaximized());
		glfwWindowHint(GLFW_POSITION_X, (int)m_szPosition.X);
		glfwWindowHint(GLFW_POSITION_Y, (int)m_szPosition.Y);

		char name[256] = {0};

#ifdef UNICODE
		size_t out_size;
		wcstombs_s(&out_size, name, (const wchar_t *)this->m_szName, sizeof(name));
#else
		strncpy_s(name, this->m_szName, sizeof(name) - 1);
#endif

		m_pWindow = glfwCreateWindow(m_szSize.W, m_szSize.H, name, nullptr, nullptr);

		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Failed to create GLFW window.");
		EX_INFO("GLFW window '%s' initialized successfully.", GetName());
		
		return true;
	}

	int GLFWApplication::Run()
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
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid keyboard handler.");

		m_fnKeyboard = &handler;

		glfwSetKeyCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetScrollHandler(GLFWScrollCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid scroll handler.");

		m_fnScroll = &handler;

		glfwSetScrollCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetCharacterHandler(GLFWCharacterCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid character handler.");

		m_fnCharacter = &handler;

		glfwSetCharCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetMousePosHandler(GLFWMousePosCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid mouse position handler.");

		m_fnMousePos = &handler;

		glfwSetCursorPosCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetMouseClickHandler(GLFWMouseClickCallback handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid mouse click handler.");

		m_fnMouseClick = &handler;

		glfwSetMouseButtonCallback(m_pWindow, handler);
	}

	void GLFWApplication::SetFramebufferSizeHandler(GLFWframebuffersizefun handler)
	{
		EX_FATAL(m_pWindow != EXN_NULL_HANDLE, "Window not initialized.");
		EX_ERROR(handler != EXN_NULL_HANDLE, "Invalid framebuffer size handler.");

		m_fnFramebufferSize = &handler;

		glfwSetFramebufferSizeCallback(m_pWindow, handler);
	}

	void *eXngine::Applications::GLFWApplication::GetHandle()
	{
#ifdef GLFW_EXPOSE_NATIVE_COCOA
		m_pHandle = reinterpret_cast<EXUINTPTR>(glfwGetCocoaWindow(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_WAYLAND
		m_pHandle = reinterpret_cast<EXUINTPTR>(glfwGetWaylandWindow(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_X11
		m_pHandle = reinterpret_cast<EXUINTPTR>(glfwGetX11Window(m_pWindow));
#elif GLFW_EXPOSE_NATIVE_WIN32
		m_pHandle = reinterpret_cast<void *>(glfwGetWin32Window(m_pWindow));
#else
		EX_FATAL(false, "Native window handle retrieval not supported on this platform.");
#endif

		EX_FATAL(m_pHandle != EXN_NULL_HANDLE, "Failed to get native window handle.");
		EX_INFO("Native Handle 0x%x", m_pHandle);

		return m_pHandle;
	}
}