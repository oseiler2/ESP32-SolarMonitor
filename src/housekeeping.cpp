#include <logging.h>
#include <housekeeping.h>
#include <mqtt.h>
#include <power.h>
#include <esp_heap_caps.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = "Housekeeping";

namespace housekeeping {
  Ticker cyclicTimer;

  void doHousekeeping() {
    uint32_t uptime = Power::getUpTime() / 1000;
    uint8_t sec = uptime % 60;
    uint8_t min = (uptime / 60) % 60;
    uint8_t h = (uptime / 3600) % 24;
    uint16_t d = (uptime / (60 * 60 * 24));

    ESP_LOGI(TAG, "Uptime: %ud %uh %um %us, Heap: Free:%u, Min:%u, Size:%u, Alloc:%u, StackHWM:%u",
      d, h, min, sec,
      ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(),
      ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(NULL));
    if (wifiManagerTask) {
      ESP_LOGI(TAG, "WifiLoop %u bytes left | Taskstate = %d | core = %u | priority = %u",
        uxTaskGetStackHighWaterMark(wifiManagerTask), eTaskGetState(wifiManagerTask), xTaskGetAffinity(wifiManagerTask), uxTaskPriorityGet(wifiManagerTask));
    }
    if (mqtt::mqttLoop) {
      ESP_LOGI(TAG, "MqttLoop %u bytes left | Taskstate = %d | core = %u | priority = %u",
        uxTaskGetStackHighWaterMark(mqtt::mqttTask), eTaskGetState(mqtt::mqttTask), xTaskGetAffinity(mqtt::mqttTask), uxTaskPriorityGet(mqtt::mqttTask));
    }
    if (ESP.getMinFreeHeap() <= 2048) {
      ESP_LOGW(TAG,
        "Memory full, counter cleared (heap low water mark = %u Bytes / "
        "free heap = %u bytes)",
        ESP.getMinFreeHeap(), ESP.getFreeHeap());
      Serial.flush();
      reboot(NULL);
    }

  }

  uint32_t getFreeRAM() {
#ifndef BOARD_HAS_PSRAM
    return ESP.getFreeHeap();
#else
    return ESP.getFreePsram();
#endif
  }


}