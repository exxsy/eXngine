#pragma once

#include <windows/window.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace eXngine::Applications
{
	typedef GLFWmousebuttonfun GLFWMouseClickCallback;
	typedef GLFWscrollfun GLFWScrollCallback;
	typedef GLFWcursorposfun GLFWMousePosCallback;
	typedef GLFWcharfun GLFWCharacterCallback;
	typedef GLFWkeyfun GLFWKeyboardCallback;

	class GLFWApplication : public Application
	{
	private:
		int Loop() override;

		GLFWwindow *m_pWindow = nullptr;
		GLFWKeyboardCallback *m_fnKeyboard = nullptr;
		GLFWCharacterCallback *m_fnCharacter = nullptr;
		GLFWMousePosCallback *m_fnMousePos = nullptr;
		GLFWMouseClickCallback *m_fnMouseClick = nullptr;
		GLFWScrollCallback *m_fnScroll = nullptr;

		bool maximized = false;

	public:
		GLFWApplication() = default;
		GLFWApplication(const char *name, Point position, Size size, bool maximized = false);
		bool Initialize() override;
		int Run() override;
		GLFWwindow *GetWindow();
		Size GetFrameBufferSize();
		std::vector<const char *> GetExtensions();
		void SetKeyboardHandler(GLFWKeyboardCallback handler);
		void SetCharacterHandler(GLFWCharacterCallback handler);
		void SetMousePosHandler(GLFWMousePosCallback handler);
		void SetMouseClickHandler(GLFWMouseClickCallback handler);
		void SetScrollHandler(GLFWScrollCallback handler);
	};
}