#pragma once

#include <Arduino.h>

namespace logging {
  typedef void (*logCallback_t)(int, const char*, const char*);

  int logger(const char* format, va_list args);

  void addOnLogCallback(logCallback_t logCallback);

}

