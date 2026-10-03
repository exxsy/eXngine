#pragma once

#include <eXmemory.h>
#include <utils/log.h>

#ifdef _WIN32
#define EX_DEBUG_BREAK() __debugbreak()
#define EX_ASSERT(expr)       \
    {                         \
        assert(expr);         \
    }
#else
#include <csignal>
#define EX_DEBUG_BREAK() std::raise(SIGTRAP)
#endif

#define EX_LOG(level, expr, msg, ...) \
    if (!(expr))                      \
        log(level, msg, ##__VA_ARGS__);

// expr is evaluated exactly once: it often creates a Vulkan object (EX_FATAL(vkCreate...(...) == VK_SUCCESS)).
#define EX_FATAL(expr, ...)                                                                                                                                       \
    {                                                                                                                                                             \
        const bool exn_expr_result = static_cast<bool>(expr);                                                                                                     \
        EX_LOG(eXngine::LogLevels::eXlog_Fatal, exn_expr_result, "Expression: %s, File: %s, Line: %d, Message: %s", #expr, __FILE__, __LINE__, ##__VA_ARGS__); \
        EX_ASSERT(exn_expr_result);                                                                                                                               \
    }

#define EX_ERROR(expr, ...)                                                                                                                                       \
    {                                                                                                                                                             \
        const bool exn_expr_result = static_cast<bool>(expr);                                                                                                     \
        EX_LOG(eXngine::LogLevels::eXlog_Error, exn_expr_result, "Expression: %s, File: %s, Line: %d, Message: %s", #expr, __FILE__, __LINE__, ##__VA_ARGS__); \
        EX_ASSERT(exn_expr_result);                                                                                                                               \
    }

#define EX_INFO(msg, ...)    EX_LOG(eXngine::LogLevels::eXlog_Info, false, msg, ##__VA_ARGS__)
#define EX_WARNING(msg, ...) EX_LOG(eXngine::LogLevels::eXlog_Warning, false, msg, ##__VA_ARGS__)
#define EX_DEBUG(expr, ...)  EX_LOG(eXngine::LogLevels::eXlog_Debug, expr, "Expression: %s, %s", #expr, ##__VA_ARGS__)
#define EX_TRACE(expr, ...)  EX_LOG(eXngine::LogLevels::eXlog_Trace, expr, "Expression: %s, %s", #expr, ##__VA_ARGS__)