
#include <functional>
#include <string>

#include <eXngine.h>
#include <utils/utils.h>

EXNEXPORT EXINT32 hash(const std::string & str)
{
	const std::hash<std::string> hasher;

	return static_cast<EXUINT32>(hasher(str));
}