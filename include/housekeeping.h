#pragma once

#include <Arduino.h>
#include <config.h>
#include <Ticker.h>

extern void reboot(TimerHandle_t xTimer);

namespace housekeeping {
  extern Ticker cyclicTimer;

  void doHousekeeping(void);
}

extern TaskHandle_t wifiManagerTask;
