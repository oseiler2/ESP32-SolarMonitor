#pragma once

#include <Arduino.h>

extern void goToDeepSleep();

typedef enum {
  RM_UNDEFINED = 0,
  RM_BAT_SAFE,
  RM_LOW,
  RM_FULL,
} RunMode;

typedef enum {
  RR_UNDEFINED = 0,
  RR_FULL_RESET,
  RR_WAKE_FROM_SLEEPTIMER,
  RR_WAKEUP_EXT1,
  RR_WAKE_FROM_BUTTON,
} ResetReason;

typedef enum {
  WR_UNDEFINED = 0,
  WAKE_FROM_SLEEPTIMER,
  WAKE_FROM_RADIO_INT,
  WAKE_FROM_BUTTON_GPIO,
  WAKE_FROM_BUTTON_EXT0
} WakeUpReason;

namespace Power {

  ResetReason afterReset();
  void deepSleep(uint32_t durationInSeconds);

  RunMode getRunMode();
  boolean setRunMode(RunMode mode);

  void prepareForLightSleep();
  WakeUpReason lightSleep(uint32_t durationInMSec);
  WakeUpReason getWakeUpReason();

  void powerDown();

  uint32_t getUpTime();

  void setMainClock(uint32_t MHz);

}
