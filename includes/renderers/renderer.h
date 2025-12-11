#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <combaseapi.h>

#include <eXngine.h>

#include <texture/image.h>
#include <types/vector.h>
#include <types/color.h>

#include <renderers/defines.h>
#include <renderers/camera/camera.h>

using namespace eXngine::Types;

namespace eXngine::Renderers
{
	class eXrenderer;

	typedef void (*OnCleanupHandler)();
	typedef void (*OnRenderHandler)(eXrenderer *);

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

	class EXNEXPORT eXrenderer
	{
	protected:
		EXN_PROPERTY(EXVOIDPTR, Device, pDevice, EXN_NULL_HANDLE);
		EXN_PROPERTY(EXCHAR *, Name, szName, EXN_NULL_HANDLE);
		EXN_PROPERTY(EXBOOL, FrameBufferResized, bFrameBufferResized, false);
		EXN_PROPERTY(Size, FrameBufferSize, szFrameBufferSize, Size(0, 0));
		EXN_PROPERTY(eXcamera *, MainCamera, pMainCamera, EXN_NULL_HANDLE);
		EXN_PROPERTY(OnRenderHandler, OnRenderHandler, fOnRender, EXN_NULL_HANDLE);
		EXN_PROPERTY(OnCleanupHandler, OnCleanupHandler, fOnCleanup, EXN_NULL_HANDLE);

	protected:
		FPSData m_sFpsData;
		std::map<std::string, ShaderModule *> m_Shaders;
		std::map<const char *, eXcamera *> m_Cameras;
	public:
		eXrenderer(const EXCHAR *, Size);
		eXrenderer() = default;
		void UpdateFPS();
		void AddCamera(const char *, eXcamera &, EXBOOL);

		FPSData GetFPS() const;
		Images::ImageManager *GetImageManager();

		virtual bool LoadShader(const char *, const std::vector<char> &, ShaderTypes) = 0;
		virtual void UseShader(const char *) = 0;
		virtual void DestroyShader(const char *) = 0;

		virtual void DrawLine(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;
		virtual void DrawTriangle(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;
		virtual void DrawRectangle(eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXvec<EXFLOAT, 2>, eXcolor) = 0;
		virtual void DrawCircle(eXvec<EXFLOAT, 2>, EXFLOAT, eXcolor) = 0;

		virtual void Initialize() = 0;
		virtual void OnRender() = 0;
		virtual void OnExit() = 0;
	};
}