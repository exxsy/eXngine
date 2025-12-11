#pragma once

namespace eXngine
{
	struct Point
	{
	public:
		inline Point(int X, int Y) : X(X), Y(Y) {}
		inline Point() : X(0), Y(0) {}
		inline Point(const Point &point) : X(point.X), Y(point.Y) {}
		int X, Y;
	};

	struct Size
	{
	public:
		inline Size(int W, int H) : W(W), H(H) {}
		inline Size() : W(0), H(0) {}
		inline Size(const Size &size) : W(size.W), H(size.H) {}
		int W, H;
	};

	struct Boundary
	{
	public:
		Boundary(Point pos, Size size) : position(pos), size(size) {}
		Point position;
		Size size;
	};

	enum LogLevels : EXUINT32
	{
		eXlog_Info,
		eXlog_Warning,
		eXlog_Error,
		eXlog_Debug,
		eXlog_Fatal,
		eXlog_Trace,

		eXlog_Count
	};

	enum PrimitiveTypes : EXUINT32
	{
		eXprimitive_Points,
		eXprimitive_Lines,
		eXprimitive_LineStrip,
		eXprimitive_Triangles,
		eXprimitive_TriangleStrip,
		eXprimitive_TriangleFan,
		eXprimitive_LinesAdjacency,
		eXprimitive_TrianglesAdjacency,
		eXprimitive_LineStripAdjacency,
		eXprimitive_TriangleStripAdjacency,
		eXprimitive_Patches,
		eXprimitive_Count
	};

	enum ShaderTypes : EXUINT32
	{
		eXshader_Vertex,
		eXshader_Fragment,
		eXshader_Geometry,
		eXshader_Compute,
		eXshader_TessellationControl,
		eXshader_TessellationEvaluation,

		eXshader_Count
	};

	enum eXkey : unsigned int
	{
		eXkey_None = 0,

		// Letters (Unicode-compatible ASCII range)
		eXkey_A = 'A',
		eXkey_B = 'B',
		eXkey_C = 'C',
		eXkey_D = 'D',
		eXkey_E = 'E',
		eXkey_F = 'F',
		eXkey_G = 'G',
		eXkey_H = 'H',
		eXkey_I = 'I',
		eXkey_J = 'J',
		eXkey_K = 'K',
		eXkey_L = 'L',
		eXkey_M = 'M',
		eXkey_N = 'N',
		eXkey_O = 'O',
		eXkey_P = 'P',
		eXkey_Q = 'Q',
		eXkey_R = 'R',
		eXkey_S = 'S',
		eXkey_T = 'T',
		eXkey_U = 'U',
		eXkey_V = 'V',
		eXkey_W = 'W',
		eXkey_X = 'X',
		eXkey_Y = 'Y',
		eXkey_Z = 'Z',

		// Digits
		eXkey_0 = '0',
		eXkey_1 = '1',
		eXkey_2 = '2',
		eXkey_3 = '3',
		eXkey_4 = '4',
		eXkey_5 = '5',
		eXkey_6 = '6',
		eXkey_7 = '7',
		eXkey_8 = '8',
		eXkey_9 = '9',

		// Control and whitespace (map to VK_* where appropriate)
		eXkey_Escape = VK_ESCAPE,
		eXkey_Tab = VK_TAB,
		eXkey_Backspace = VK_BACK,
		eXkey_Return = VK_RETURN,
		eXkey_Space = VK_SPACE,

		// Modifiers
		eXkey_Shift = VK_SHIFT,
		eXkey_Control = VK_CONTROL,
		eXkey_Alt = VK_MENU,
		eXkey_CapsLock = VK_CAPITAL,
		eXkey_NumLock = VK_NUMLOCK,
		eXkey_ScrollLock = VK_SCROLL,

		// Navigation
		eXkey_Insert = VK_INSERT,
		eXkey_Delete = VK_DELETE,
		eXkey_Home = VK_HOME,
		eXkey_End = VK_END,
		eXkey_PageUp = VK_PRIOR,
		eXkey_PageDown = VK_NEXT,
		eXkey_Left = VK_LEFT,
		eXkey_Right = VK_RIGHT,
		eXkey_Up = VK_UP,
		eXkey_Down = VK_DOWN,

		// Function keys
		eXkey_F1 = VK_F1,
		eXkey_F2 = VK_F2,
		eXkey_F3 = VK_F3,
		eXkey_F4 = VK_F4,
		eXkey_F5 = VK_F5,
		eXkey_F6 = VK_F6,
		eXkey_F7 = VK_F7,
		eXkey_F8 = VK_F8,
		eXkey_F9 = VK_F9,
		eXkey_F10 = VK_F10,
		eXkey_F11 = VK_F11,
		eXkey_F12 = VK_F12,
		eXkey_F13 = VK_F13,
		eXkey_F14 = VK_F14,
		eXkey_F15 = VK_F15,
		eXkey_F16 = VK_F16,
		eXkey_F17 = VK_F17,
		eXkey_F18 = VK_F18,
		eXkey_F19 = VK_F19,
		eXkey_F20 = VK_F20,
		eXkey_F21 = VK_F21,
		eXkey_F22 = VK_F22,
		eXkey_F23 = VK_F23,
		eXkey_F24 = VK_F24,

		// OEM / punctuation keys (platform dependent mapping via VK_OEM_*)
		eXkey_OEM_1 = VK_OEM_1, // ';:' for US
		eXkey_OEM_2 = VK_OEM_2, // '/?' for US
		eXkey_OEM_3 = VK_OEM_3, // '`~' for US
		eXkey_OEM_4 = VK_OEM_4, // '[{' for US
		eXkey_OEM_5 = VK_OEM_5, // '\\|' for US
		eXkey_OEM_6 = VK_OEM_6, // ']}' for US
		eXkey_OEM_7 = VK_OEM_7, // '\'"' for US
		eXkey_OEM_8 = VK_OEM_8,

		// Numpad
		eXkey_NumPad0 = VK_NUMPAD0,
		eXkey_NumPad1 = VK_NUMPAD1,
		eXkey_NumPad2 = VK_NUMPAD2,
		eXkey_NumPad3 = VK_NUMPAD3,
		eXkey_NumPad4 = VK_NUMPAD4,
		eXkey_NumPad5 = VK_NUMPAD5,
		eXkey_NumPad6 = VK_NUMPAD6,
		eXkey_NumPad7 = VK_NUMPAD7,
		eXkey_NumPad8 = VK_NUMPAD8,
		eXkey_NumPad9 = VK_NUMPAD9,
		eXkey_NumMultiply = VK_MULTIPLY,
		eXkey_NumAdd = VK_ADD,
		eXkey_NumSeparator = VK_SEPARATOR,
		eXkey_NumSubtract = VK_SUBTRACT,
		eXkey_NumDecimal = VK_DECIMAL,
		eXkey_NumDivide = VK_DIVIDE,

		// System / special
		eXkey_PrintScreen = VK_SNAPSHOT,
		eXkey_Pause = VK_PAUSE,

		// Common multimedia keys (when available)
		eXkey_VolumeMute = VK_VOLUME_MUTE,
		eXkey_VolumeDown = VK_VOLUME_DOWN,
		eXkey_VolumeUp = VK_VOLUME_UP,
		eXkey_MediaNextTrack = VK_MEDIA_NEXT_TRACK,
		eXkey_MediaPrevTrack = VK_MEDIA_PREV_TRACK,
		eXkey_MediaStop = VK_MEDIA_STOP,
		eXkey_MediaPlayPause = VK_MEDIA_PLAY_PAUSE,

		// Mouse buttons (placed at end to keep them distinct from VK range)
		eXkey_MouseLeft = 0x1000,
		eXkey_MouseRight = 0x1001,
		eXkey_MouseMiddle = 0x1002,
		eXkey_MouseX1 = 0x1003,
		eXkey_MouseX2 = 0x1004,
	};
}