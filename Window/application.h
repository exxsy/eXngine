#pragma once

#include <eXngine.h>

namespace eXngine 
{
	namespace Applications 
	{
		class Application 
		{
			char* m_name = nullptr;
			Point m_position;
			Size m_size;
			bool maximized;
		public:
			Application(const char* name, Point position, Size size, bool maximized);
			~Application();
			virtual bool Initialize() = 0;
			virtual int Run() = 0;
			virtual int Loop() = 0;
			const char* GetName();
			Size GetSize();
			Point GetPosition();
			bool IsMaximized();

			void SetSize(Size size);
			void SetPosition(Point position);
			void SetMaximized(bool maximized);
		};
	}
}