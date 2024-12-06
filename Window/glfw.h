#pragma once

#include "application.h"
#include <GLFW/glfw3.h>

namespace eXngine 
{
	namespace Applications 
	{
		class GLFWApplication : public Application
		{
		private:
			bool Initialize() override;
			int Loop() override;

			GLFWwindow* m_window = nullptr;
			bool maximized = false;
		public:
			GLFWApplication() = default;
			GLFWApplication(const char* name, Point position, Size size, bool maximized = false);
			int Run() override;
		};
	}
}