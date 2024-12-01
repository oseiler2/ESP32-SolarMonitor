#include <events.h>
#include <globals.h>


// Local logging tag
static const char TAG[] = "Events";

void startTimer(const char* timerName, uint32_t delayMs, TimerCallbackFunction_t fnToCall) {
  // TODO: these timers don't seem to survive light sleep and never fire in that case.
  TimerHandle_t timer = xTimerCreate(timerName, pdMS_TO_TICKS(delayMs), pdFALSE, (void*)0, fnToCall);
  if (timer == NULL) {
    ESP_LOGE(TAG, "xTimerCreate returned error");
  } else {
    if (xTimerStart(timer, 0) == pdPASS) {
    } else {
      ESP_LOGE(TAG, "timer could not be started");
    }
  }
}