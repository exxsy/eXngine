#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <combaseapi.h>
#include <renderers/defines.h>
#include <renderers/abstract.h>
#include <texture/image.h>

#include <types/vector.h>
#include <types/color.h>

using namespace eXngine::Types;

namespace eXngine::Renderers
{
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

	struct ShaderModule
	{
	public:
		std::vector<char> code;
		ShaderTypes type;
	};

	template <typename MemoryType, typename IndexType = EXUINT32>
	struct RenderCommand
	{
	public:
		IndexType m_iIndicesStart;
		IndexType m_iVerticesStart;
		IndexType m_nIndicesCount;
		IndexType m_nVerticesCount;
		MemoryType m_pVertices;
		MemoryType m_pIndices;
		const char *m_szShaderName = nullptr;
	};

	template <typename T, typename S = EXUINT32>
	interface IRenderCommands
	{
	protected:
		std::vector<RenderCommand<T, S>> m_RenderCommands;

	public:
		virtual void PushRenderCommand(RenderCommand<T, S>) = 0;
		virtual void PopRenderCommand() = 0;
	};

	class EXNEXPORT BaseRenderer : virtual public AbstractRenderer
	{
	protected:
		FPSData m_sFpsData;
		std::map<std::string, ShaderModule *> m_Shaders;
		OnCleanupHandler m_fOnCleanup;

		// Size m_szFrameBufferSize;
		// EXCHAR *m_szName = EXN_NULL_HANDLE;
		// bool m_bFrameBufferResized = false;
		// std::mutex m_resizeMutex;
		// void* m_pDevice = EXN_NULL_HANDLE;

		EXN_PROPERTY(EXVOIDPTR, Device, pDevice, EXN_NULL_HANDLE);
		EXN_PROPERTY(EXCHAR *, Name, szName, EXN_NULL_HANDLE);
		EXN_PROPERTY(Size, FrameBufferSize, szFrameBufferSize, Size(0, 0));
		EXN_PROPERTY(EXBOOL, FrameBufferResized, bFrameBufferResized, false);

	public:
		BaseRenderer(const EXCHAR *, Size);
		BaseRenderer() = default;
		// void SetFrameBufferResized(bool);
		// void SetFrameBufferSize(Size);
		// void SetFrameBufferSize(EXINT, EXINT);
		void SetOnCleanupHandler(OnCleanupHandler);
		void UpdateFPS();

		FPSData GetFPS() const;
		Images::ImageManager *GetImageManager();

		virtual bool LoadShader(const char *, const std::vector<char> &, ShaderTypes) = 0;
		virtual void UseShader(const char *) = 0;
		virtual void DestroyShader(const char *) = 0;

		virtual void DrawLine(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;
		virtual void DrawTriangle(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;	
		virtual void DrawRectangle(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;
		virtual void DrawCircle(eXvec<EXFLOAT, 2>, EXFLOAT, eXcolor) = 0;

	};
}