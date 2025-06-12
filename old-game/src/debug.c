#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "debug.h"


void
spDebugLogFatal (char const *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}


[[noreturn]]
void
spDebugCrash (int rc)
{
  exit(rc);
}
