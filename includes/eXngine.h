#pragma once

struct Point 
{
public:
	Point(int X, int Y) : X(X), Y(Y) {}
	int X, Y;
};

struct Size
{
public:
	Size(int W, int H): W(W), H(H) {}
	int W, H;
};

struct Boundary
{
public:
	Boundary(Point pos, Size size): position(pos), size(size) {}
	Point position;
	Size size;
};

typedef int EXINT;

#define EXENGINE_MAKE_VERSION(patch, major, minor) (((patch) << 16) | ((major) << 8) | (minor))
#define EXENGINE_GET_PATCH_VERSION(version) (((version) >> 16) & 0xFF)
#define EXENGINE_GET_MAJOR_VERSION(version) (((version) >> 8) & 0xFF)
#define EXENGINE_GET_MINOR_VERSION(version) ((version) & 0xFF)
#define EX_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

#define EXENGINE "eXngine"
#define EXENGINE_VERSION EXENGINE_MAKE_VERSION(1, 0, 0)
#define EXN_SUCCESS 0
#define EXN_FAILURE 1
