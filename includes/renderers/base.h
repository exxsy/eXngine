#pragma once

#include <eXngine.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <combaseapi.h>
#include <renderers/defines.h>
#include <renderers/abstract.h>

#include <texture/image.h>

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
		const char* m_szShaderName = nullptr;
	};

	template <typename T, typename S = EXUINT32>
	interface IRenderCommands {
	protected:
		std::vector<RenderCommand<T, S>> m_RenderCommands;

	public:
		virtual void PushRenderCommand(RenderCommand<T, S>) = 0;
		virtual void PopRenderCommand() = 0;
	};

	class EXNEXPORT BaseRenderer : virtual public AbstractRenderer
	{
	protected:
		EXCHAR *m_szName = nullptr;
		Size m_szFrameBufferSize;
		FPSData m_sFpsData;
		std::map<std::string, ShaderModule*> m_Shaders;
		OnCleanupHandler m_fOnCleanup;

		bool m_bFrameBufferResized = false;
		//std::mutex m_resizeMutex;

		void* m_pDevice = nullptr;
	public:
		BaseRenderer(const EXCHAR *, Size = Size(0, 0));
		void SetFrameBufferResized(bool);
		void SetFrameBufferSize(Size);
		void SetOnCleanupHandler(OnCleanupHandler);
		void UpdateFPS();

		FPSData GetFPS() const;
		Images::ImageManager* GetImageManager();

		virtual bool LoadShader(const char*, const std::vector<char>&, ShaderTypes) = 0;
		virtual void UseShader(const char*) = 0;
		virtual void DestroyShader(const char*) = 0;
	};
}