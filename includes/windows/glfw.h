#pragma once

#include "window.h"
#include <GLFW/glfw3.h>
#include <vector>

namespace eXngine 
{
	namespace Applications 
	{
		typedef GLFWmousebuttonfun GLFWMouseClickCallback;
		typedef GLFWcursorposfun GLFWMousePosCallback;
		typedef GLFWcharfun GLFWCharacterCallback;
		typedef GLFWkeyfun GLFWKeyboardCallback;

		class GLFWApplication : public Application
		{
		private:
			int Loop() override;

			GLFWwindow* m_pWindow = nullptr;
			GLFWKeyboardCallback* m_keyboard = nullptr;
			GLFWCharacterCallback* m_character = nullptr;
			GLFWMousePosCallback* m_mousePos = nullptr;
			GLFWMouseClickCallback* m_mouseClick = nullptr;
	
			bool maximized = false;
		public:
			GLFWApplication() = default;
			GLFWApplication(const char* name, Point position, Size size, bool maximized = false);
			bool Initialize() override;
			int Run() override;
			GLFWwindow* GetWindow();
			Size GetFrameBufferSize();
			std::vector<const char*> GetExtensions();
			void SetKeyboardHandler(GLFWKeyboardCallback handler);
			void SetCharacterHandler(GLFWCharacterCallback handler);
			void SetMousePosHandler(GLFWMousePosCallback handler);
			void SetMouseClickHandler(GLFWMouseClickCallback handler);
		};
	}
}