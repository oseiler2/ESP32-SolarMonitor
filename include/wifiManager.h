#pragma once

#include <config.h>
#include <callbacks.h>
#include <ESPAsyncWebServer.h>
#include <configParameter.h>

extern void reboot(TimerHandle_t xTimer);
extern void saveConfig(TimerHandle_t xTimer);
extern void updateLoraSettings(TimerHandle_t xTimer);

namespace WifiManager {
  void setupWifiManager(const char* appName, std::vector<ConfigParameterBase<Config>*> configParameterVector, bool keepCaptivePortalActive, bool captivePortalActiveWhenNotConnected);
  void resetSettings();
  void startCaptivePortal();
  String getMac();
  void wifiManagerLoop(void* pvParameters);
  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);
  void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
}
