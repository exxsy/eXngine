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
}