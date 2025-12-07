#pragma once

#include <vector>
#include <cassert>

#include <windows/window.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace eXngine::Windows
{
	typedef GLFWmousebuttonfun GLFWMouseClickCallback;
	typedef GLFWscrollfun GLFWScrollCallback;
	typedef GLFWcursorposfun GLFWMousePosCallback;
	typedef GLFWcharfun GLFWCharacterCallback;
	typedef GLFWkeyfun GLFWKeyboardCallback;

	class EXNEXPORT GLFWWindow : public Window
	{
	private:
		std::vector<GLFWmonitor *> m_vMonitors;
		GLFWmonitor *m_pPrimaryMonitor = EXN_NULL_HANDLE;
		GLFWwindow *m_pWindow = EXN_NULL_HANDLE;
		GLFWKeyboardCallback *m_fnKeyboard = EXN_NULL_HANDLE;
		GLFWCharacterCallback *m_fnCharacter = EXN_NULL_HANDLE;
		GLFWMousePosCallback *m_fnMousePos = EXN_NULL_HANDLE;
		GLFWMouseClickCallback *m_fnMouseClick = EXN_NULL_HANDLE;
		GLFWScrollCallback *m_fnScroll = EXN_NULL_HANDLE;
		GLFWframebuffersizefun *m_fnFramebufferSize = EXN_NULL_HANDLE;

		EXVOIDPTR m_pHandle = EXN_NULL_HANDLE;

	public:
		GLFWWindow(const EXCHAR *name, Point position, Size size, bool maximized = false);

		virtual bool Initialize() override;
		virtual void ProcessInput(eXkey, bool) override;
		virtual EXVOIDPTR GetHandle() override;
		virtual int Run() override;

		GLFWwindow *GetWindow();
		Size GetFrameBufferSize();
		std::vector<const char *> GetExtensions();
		void SetKeyboardHandler(GLFWKeyboardCallback handler);
		void SetCharacterHandler(GLFWCharacterCallback handler);
		void SetMousePosHandler(GLFWMousePosCallback handler);
		void SetMouseClickHandler(GLFWMouseClickCallback handler);
		void SetScrollHandler(GLFWScrollCallback handler);
		void SetFramebufferSizeHandler(GLFWframebuffersizefun handler);
	};
}