#pragma once

#include <optional>
#include <eXngine.h>

namespace eXngine::Applications
{
	typedef void (*OnInitializeHandler)(void *);
	typedef void (*OnLoopHandler)(void *);
	typedef void (*OnCleanupHandler)(void *);
	typedef void (*OnResizeHandler)(int, int);
	typedef void (*OnMouseMoveHandler)(int, int);
	typedef void (*OnKeyboardPressHandler)(int, int, int, int);
	typedef void (*OnMousePressHandler)(int, bool);

	class EXNEXPORT Application
	{
	protected:
		Point m_szPosition;
		Size m_szSize;
		EXBOOL m_bIsMaximized = false;
		EXCHAR *m_szName = EXN_NULL_HANDLE;
		EXVOIDPTR m_pHandle = EXN_NULL_HANDLE;
		EXINT8 m_bKeys[256] = {0};
		EXINT8 m_bMouseButtons[5] = {0};

		OnInitializeHandler m_fnOnInitialize = EXN_NULL_HANDLE;
		OnLoopHandler m_fnOnLoop = EXN_NULL_HANDLE;
		OnCleanupHandler m_fnOnCleanup = EXN_NULL_HANDLE;
		OnResizeHandler m_fnOnResize = EXN_NULL_HANDLE;
		OnMouseMoveHandler m_fnOnMouseMove = EXN_NULL_HANDLE;
		OnKeyboardPressHandler m_fnOnKeyboardPress = EXN_NULL_HANDLE;
		OnMousePressHandler m_fnOnMousePress = EXN_NULL_HANDLE;

	public:
		Application(const EXCHAR *name, Point position, Size size, bool maximized);
		Application();
		~Application();
		const EXCHAR *GetName();
		Size GetSize();
		Point GetPosition();
		bool IsMaximized();
		void SetSize(Size size);
		void SetPosition(Point position);
		void SetIsMaximized(bool maximized);
		void SetOnInitializeHandler(OnInitializeHandler handler);
		void SetOnLoopHandler(OnLoopHandler handler);
		void SetOnCleanupHandler(OnCleanupHandler handler);

		virtual void ProcessInput(eXkey, bool) = 0;
		virtual void *GetHandle() = 0;
		virtual bool Initialize() = 0;
		virtual int Run() = 0;
	};
}