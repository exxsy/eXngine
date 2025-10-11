#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>
#include <utils/fbx-loader.h>

namespace eXngine::Renderers
{
	class BaseRenderer
	{
	protected:
		char *m_szName = nullptr;
		Size m_szFrameBufferSize;

		std::unique_ptr<Utils::FbxLoader> m_pMeshLoader;
		bool m_bFrameBufferResized = false;
		//std::mutex m_resizeMutex;

		void* m_pDevice = nullptr;
	public:

		BaseRenderer(const char *, Size = Size(0, 0));
		void SetFrameBufferResize(bool);
		void SetFrameBufferSize(Size);
		virtual void Initialize() = 0;
		virtual void OnRender() = 0;
		virtual void OnExit() = 0;
	};
}