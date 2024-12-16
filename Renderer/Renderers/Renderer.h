#pragma once

namespace eXngine
{
	namespace Renderers
	{
		class Renderer
		{
		protected:
			char* m_szName = nullptr;
			bool m_bFrameBufferResized = false;
		private:
		public:
			Renderer(const char *);
			void SetFrameBufferResize(bool);
			virtual bool Initialize() = 0;
			virtual void OnRender() = 0;
			virtual void OnExit() = 0;
		};
	}
}