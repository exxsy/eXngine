#pragma once

#include <optional>
#include <eXngine.h>
#include <renderers/abstract.h>

using namespace eXngine::Renderers;

namespace eXngine::Applications
{
	class EXNEXPORT Application
	{
	protected:
		char *m_name = nullptr;
		Point m_position;
		Size m_size;
		bool maximized;

		std::optional<AbstractRenderer *> m_pRenderer = nullptr;
	public:
		void *m_pInstance = nullptr;
		Application(const char *name, Point position, Size size, bool maximized);
		Application();
		~Application();
		const char *GetName();
		Size GetSize();
		Point GetPosition();
		bool IsMaximized();
		void SetRenderer(AbstractRenderer *);
		void SetSize(Size size);
		void SetPosition(Point position);
		void SetMaximized(bool maximized);

		virtual bool Initialize() = 0;
		virtual int Run() = 0;
		virtual int Loop() = 0;
	};
}