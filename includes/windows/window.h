#pragma once

#include <optional>
#include <eXngine.h>

namespace eXngine::Applications
{
	typedef void (*OnInitializeHandler)(void *);
	typedef void (*OnLoopHandler)(void *);
	typedef void (*OnCleanupHandler)(void *);

	class EXNEXPORT Application
	{
	protected:
		EXINT8 * m_szName = EXN_NULL_HANDLE;
		Point m_szPosition;
		Size m_szSize;
		EXBOOL m_bMaximized = false;
		void* m_pHandle = EXN_NULL_HANDLE;

		OnInitializeHandler m_fnOnInitialize = EXN_NULL_HANDLE;
		OnLoopHandler m_fnOnLoop = EXN_NULL_HANDLE;
		OnCleanupHandler m_fnOnCleanup = EXN_NULL_HANDLE;
	public:
		Application(const char *name, Point position, Size size, bool maximized);
		Application();
		~Application();
		const char *GetName();
		Size GetSize();
		Point GetPosition();
		bool IsMaximized();
		void SetSize(Size size);
		void SetPosition(Point position);
		void SetMaximized(bool maximized);
		void SetOnInitializeHandler(OnInitializeHandler handler);
		void SetOnLoopHandler(OnLoopHandler handler);
		void SetOnCleanupHandler(OnCleanupHandler handler);
		void *GetHandle();

		virtual bool Initialize() = 0;
		virtual int Run() = 0;
	};
}