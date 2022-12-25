#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>

#include "utils.h"

// intercept appThrowf to at least see what the fuck happened

void hook_app_throw(const wchar_t *fmt, ...) {
  static wchar_t msg[4096];

  va_list va;
  va_start(va, fmt);
  vswprintf(msg, sizeof(msg) / sizeof(*msg), fmt, va);
  va_end(va);

  // it's supposed to throw here, but since the exception handling is broken
  // throwing will just call std::terminate(), so this is a bit more graceful
  // throw msg;
  fatal_error("appThrowf called:\n%ls", msg);
}
