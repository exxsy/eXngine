#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>


namespace eXngine
{
	namespace Renderers
	{
		class Renderer
		{
		protected:
			char *m_szName = nullptr;
			Size m_szFrameBufferSize;

			std::atomic<bool> m_bFrameBufferResized{false};
			std::mutex m_resizeMutex;
		private:
		public:
			Renderer(const char *, Size = Size(0, 0));
			void SetFrameBufferResize(bool);
			void SetFrameBufferSize(Size);
			virtual bool Initialize() = 0;
			virtual void OnRender() = 0;
			virtual void OnExit() = 0;
		};
	}
}