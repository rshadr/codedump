#ifndef __sp_debug_h__
#define __sp_debug_h__

#include <stdarg.h>


void spDebugLogFatal(char const *fmt, ...);
[[noreturn]] void spDebugCrash (int rc);


#define SP_ASSERT(cnd, ...) \
  do { \
    if (! (cnd) ) { \
      spDebugLogFatal("%s", #cnd); \
      spDebugLogFatal(__VA_ARGS__); \
      spDebugCrash(-1); \
    } \
  } while (0)

#define SP_VK_ASSERT(expr, ...) \
  SP_ASSERT( ((expr) == VK_SUCCESS), __VA_ARGS__)


#endif /* !defined(__sp_debug_h__) */

