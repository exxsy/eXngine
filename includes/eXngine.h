#pragma once

#define EXENGINE_MAKE_VERSION(patch, major, minor) (((patch) << 16) | ((major) << 8) | (minor))
#define EXENGINE_GET_PATCH_VERSION(version) (((version) >> 16) & 0xFF)
#define EXENGINE_GET_MAJOR_VERSION(version) (((version) >> 8) & 0xFF)
#define EXENGINE_GET_MINOR_VERSION(version) ((version) & 0xFF)
#define EX_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

#define EXINT8 __int8
#define EXINT16 __int16
#define EXINT32 __int32
#define EXINT64 __int64
#define EXINT EXINT32
#define EXUINT8 unsigned __int8
#define EXUINT16 unsigned __int16
#define EXUINT32 unsigned __int32
#define EXUINT64 unsigned __int64
#define EXUINT EXUINT32
#define EXFLOAT float
#define EXDOUBLE double
#define EXLONGDOUBLE long double
#define EXLONGLONG long long
#define EXSIZE EXLONGLONG
#define EXBOOL bool
#define EXUINTPTR EXUINT32 *
#define EXVOIDPTR void *
#define EXDWORD unsigned long
#define EXMATH glm

#ifdef UNICODE
#define EXCHAR wchar_t
#else
#define EXCHAR char
#endif

#define EXENGINE "eXngine"
#define EXENGINE_VERSION EXENGINE_MAKE_VERSION(1, 0, 0)
#define EXN_SUCCESS 0
#define EXN_FAILURE 1
#define EXN_TRUE true
#define EXN_FALSE false
#define EXN_NULL NULL
#define EXN_NULL_HANDLE nullptr

#ifdef EXNEXPORT
#undef EXNEXPORT
#endif

#define EXNEXPORT __declspec(dllexport)
#define EXNIMPORT __declspec(dllimport)

#define EXN_SINGLETON(type, name, ...)           \
public:                                          \
	inline static type *Get##name()                     \
	{                                            \
		static type *s_##name = EXN_NULL_HANDLE; \
		if (s_##name == EXN_NULL_HANDLE)         \
		{                                        \
			s_##name = new type(__VA_ARGS__);    \
		}                                        \
		return s_##name;                         \
	};

#define EXN_PROPERTY(type, name, prop, def)     \
	type m_##prop = def;                        \
                                                \
public:                                         \
	type Get##name() const { return m_##prop; } \
	void Set##name(type value) { m_##prop = value; }

#define EXN_PROPERTY_ARRAY(type, name, prop, sz)       \
	type m_##prop[sz] = {0};                           \
                                                       \
public:                                                \
	const type *Get##name() const { return m_##prop; } \
	void Set##name(type *value) { memcpy_s((void *)m_##prop, sizeof(type) * sz, value, sizeof(type) * sz); }

#ifdef _WIN32
#include <windows.h>
#define WINDOWS_LEAN_AND_MEAN
#endif

#include <eXtypes.h>
#include <eXdebug.h>
#include <eXmemory.h>