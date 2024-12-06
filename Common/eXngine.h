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