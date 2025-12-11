
#include <functional>
#include <string>
#include <cstdarg>

#include <eXngine.h>
#include <utils/log.h>

EXNEXPORT void log(eXngine::LogLevels level, const char * message, ...)
{
	va_list args;
	va_start(args, message);
	static char buffer[10480];
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, message, args);
	va_end(args);

	static const char * prefixes[eXngine::LogLevels::eXlog_Count] = {
		"[INFO]: ",
		"[WARN]: ",
		"[CRIT]: ",
		"[DEBG]: ",
		"[FATL]: ",
		"[TRCE]: "
	};

	printf("%s%s\n", prefixes[level], buffer);
}