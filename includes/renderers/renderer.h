#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>
#include <vector>

//#include <utils/fbx-loader.h>
#include <texture/image.h>

namespace eXngine::Renderers
{
	class BaseRenderer;

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
		OnCleanupHandler m_fOnCleanup;

		bool m_bFrameBufferResized = false;
		//std::mutex m_resizeMutex;

		void* m_pDevice = nullptr;
	public:
		BaseRenderer(const char *, Size = Size(0, 0));
		void SetFrameBufferResize(bool);
		void SetFrameBufferSize(Size);
		void SetOnCleanupHandler(OnCleanupHandler);
		void UpdateFPS();

		FPSData GetFPS() const;
		Images::ImageManager* GetImageManager();

		virtual void Initialize() = 0;
		virtual void OnRender() = 0;
		virtual void OnExit() = 0;

		// Base drawing functions
		//virtual void BeginFrame() = 0;
		//virtual void EndFrame() = 0;

		//// Basic shape drawing
		//virtual void DrawLine(const EXVEC3& start, const EXVEC3& end, const EXVEC3& color = EXVEC3(1.0f)) = 0;
		//virtual void DrawTriangle(const EXVEC3& v1, const EXVEC3& v2, const EXVEC3& v3, const EXVEC3& color = EXVEC3(1.0f)) = 0;
		//virtual void DrawQuad(const EXVEC3& position, const EXVEC2& size,  const EXVEC3& color = EXVEC3(1.0f)) = 0;

		//// Batch rendering functions
		//virtual void BeginBatch(eXngine::PrimitiveTypes type) = 0;
		//virtual void AddVertex(const Utils::Vertex& vertex) = 0;
		//virtual void EndBatch() = 0;

		//// Texture functions
		//virtual EXUINT32 CreateTexture(const void* data, EXUINT32 width, EXUINT32 height) = 0;
		//virtual void DeleteTexture(EXUINT32 textureId) = 0;
		//virtual void BindTexture(EXUINT32 textureId) = 0;

		//// State management
		//virtual void SetViewport(int x, int y, int width, int height) = 0;
		//virtual void EnableDepthTest(bool enable) = 0;
		//virtual void EnableBlending(bool enable) = 0;
		//
		//// Transform functions
		//virtual void PushMatrix() = 0;
		//virtual void PopMatrix() = 0;
		//virtual void Translate(const EXVEC3& offset) = 0;
		//virtual void Rotate(float angle, const EXVEC3& axis) = 0;
		//virtual void Scale(const EXVEC3& scale) = 0;
	};
}