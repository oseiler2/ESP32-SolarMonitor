#pragma once

#include <Arduino.h>

void startTimer(const char* timerName, uint32_t delayMs, TimerCallbackFunction_t fnToCall);
