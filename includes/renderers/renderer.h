#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>
#include <utils/fbx-loader.h>

namespace eXngine::Renderers
{
	class BaseRenderer;

	typedef void (*OnRenderHandler)(BaseRenderer*, VkCommandBuffer);
	typedef void (*OnCleanupHandler)();

	struct FPSData
	{
		float m_fLastTime = 0.0f;
		float m_fDeltaTime = 0.0f;
		int m_fFrameCount = 0;
		float m_fFrameTimer = 0.0f;
		float m_fFPS = 0.0f;
		float m_fAverageDeltaTime = 0.0f;
	};

	class BaseRenderer
	{
	protected:
		char *m_szName = nullptr;
		Size m_szFrameBufferSize;
		FPSData m_sFpsData;
		OnRenderHandler m_fOnRender;
		OnCleanupHandler m_fOnCleanup;

		std::unique_ptr<Utils::FbxLoader> m_pMeshLoader;
		bool m_bFrameBufferResized = false;
		//std::mutex m_resizeMutex;

		void* m_pDevice = nullptr;
	public:
		BaseRenderer(const char *, Size = Size(0, 0));
		void SetFrameBufferResize(bool);
		void SetFrameBufferSize(Size);
		void SetOnRenderHandler(OnRenderHandler);
		void SetOnCleanupHandler(OnCleanupHandler);
		virtual void Initialize() = 0;
		virtual void OnRender() = 0;
		virtual void OnExit() = 0;

		void UpdateFPS();
		FPSData GetFPS() const;
	};
}