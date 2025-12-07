#pragma once

#include <optional>
#include <eXngine.h>

namespace eXngine::Windows
{
	typedef void (*OnInitializeHandler)(void *);
	typedef void (*OnLoopHandler)(void *);
	typedef void (*OnCleanupHandler)(void *);
	typedef void (*OnResizeHandler)(EXINT, EXINT);
	typedef void (*OnMouseMoveHandler)(EXINT, EXINT);
	typedef void (*OnKeyboardPressHandler)(EXINT, EXINT, EXINT, EXINT);
	typedef void (*OnMousePressHandler)(EXINT, EXBOOL);

	class EXNEXPORT Window
	{
	protected:
		EXINT8 m_bKeys[256] = {0};
		EXINT8 m_bMouseButtons[5] = {0};

		OnInitializeHandler m_fnOnInitialize = EXN_NULL_HANDLE;
		OnLoopHandler m_fnOnLoop = EXN_NULL_HANDLE;
		OnCleanupHandler m_fnOnCleanup = EXN_NULL_HANDLE;
		OnResizeHandler m_fnOnResize = EXN_NULL_HANDLE;
		OnMouseMoveHandler m_fnOnMouseMove = EXN_NULL_HANDLE;
		OnKeyboardPressHandler m_fnOnKeyboardPress = EXN_NULL_HANDLE;
		OnMousePressHandler m_fnOnMousePress = EXN_NULL_HANDLE;

	protected:
		EXN_PROPERTY(Point, Position, position, Point(0, 0));
		EXN_PROPERTY(Size, Size, size, Size(0, 0));
		EXN_PROPERTY(EXBOOL, IsMaximized, bIsMaximized, EXN_FALSE);
		// EXN_PROPERTY_ARRAY(EXCHAR, Name, szName, 256);
		EXN_PROPERTY(EXCHAR *, Name, szName, EXN_NULL_HANDLE);

	public:
		Window(const EXCHAR *name, Point position, Size size, EXBOOL maximized);
		Window(const EXCHAR *name, Point position, Size size);
		~Window();
		void SetOnInitializeHandler(OnInitializeHandler handler);
		void SetOnLoopHandler(OnLoopHandler handler);
		void SetOnCleanupHandler(OnCleanupHandler handler);

		virtual void ProcessInput(eXkey, EXBOOL) = 0;
		virtual EXBOOL Initialize() = 0;
		virtual EXINT Run() = 0;
		virtual EXVOIDPTR GetHandle() = 0;
	};
}