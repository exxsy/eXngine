#pragma once

namespace eXngine
{
	struct Point
	{
	public:
		Point(int X, int Y) : X(X), Y(Y) {}
		int X, Y;
	};

	struct Size
	{
	public:
		Size(int W, int H) : W(W), H(H) {}
		int W, H;
	};

	struct Boundary
	{
	public:
		Boundary(Point pos, Size size) : position(pos), size(size) {}
		Point position;
		Size size;
	};

	enum PrimitiveTypes : int
	{
		Points,
		Lines,
		LineStrip,
		Triangles,
		TriangleStrip
	};
}

#define EXENGINE_MAKE_VERSION(patch, major, minor) (((patch) << 16) | ((major) << 8) | (minor))
#define EXENGINE_GET_PATCH_VERSION(version) (((version) >> 16) & 0xFF)
#define EXENGINE_GET_MAJOR_VERSION(version) (((version) >> 8) & 0xFF)
#define EXENGINE_GET_MINOR_VERSION(version) ((version) & 0xFF)
#define EX_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

#define EXINT8          __int8
#define EXINT16         __int16
#define EXINT32         __int32
#define EXINT64         __int64
#define EXINT           EXINT32
#define EXUINT8         unsigned char
#define EXUINT16        unsigned __int16
#define EXUINT32        unsigned __int32
#define EXUINT64        unsigned __int64			
#define EXUINT          EXUINT32
#define EXFLOAT         float
#define EXDOUBLE        double
#define EXLONGDOUBLE    long double
#define EXCHAR          char
#define EXBOOL          bool
#define EXUINTPTR       EXUINT32*
#define EXVEC2          glm::vec2
#define EXVEC3          glm::vec3
#define EXVEC4          glm::vec4
#define EXMAT4          glm::mat4
#define EXMAT3          glm::mat3
#define EXMAT2          glm::mat2

#define EXENGINE "eXngine"
#define EXENGINE_VERSION EXENGINE_MAKE_VERSION(1, 0, 0)
#define EXN_SUCCESS 0
#define EXN_FAILURE 1
#define EXN_TRUE true
#define EXN_FALSE false
#define EXN_NULL NULL
#define EXN_NULL_HANDLE nullptr
