#pragma once

#include "../../includes/eXngine.h"
#include "../Renderers/AbstractRenderer.h"
#include <optional>

using namespace eXngine::Renderers;

namespace eXngine 
{
	namespace Applications 
	{
		class Application
		{
		protected:
			char* m_name = nullptr;
			Point m_position;
			Size m_size;
			bool maximized;

			std::optional<Renderer*> m_pRenderer = nullptr;
		public:
			void* m_pInstance = nullptr;
			Application(const char* name, Point position, Size size, bool maximized);
			Application();
			~Application();
			virtual bool Initialize() = 0;
			virtual int Run() = 0;
			virtual int Loop() = 0;
			const char* GetName();
			Size GetSize();
			Point GetPosition();
			bool IsMaximized();
			void SetRenderer(Renderer*);
			void SetSize(Size size);
			void SetPosition(Point position);
			void SetMaximized(bool maximized);
		};
	}
}