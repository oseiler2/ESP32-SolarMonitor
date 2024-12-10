#include <globals.h>
#include <configManager.h>
#include <nvs_config.h>
#include <rtcOsc.h>
#include <power.h>

#include <SPI.h>
#include <wifiManager.h>
#include <timekeeper.h>
#include <radio.h>
#include <modbus.h>
#include <mqtt.h>
#include <model.h>

#include <housekeeping.h>

#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_task_wdt.h>
#include <esp_bt.h>
#include <esp_sntp.h>
#include <esp_chip_info.h>
#include <spi_flash_mmap.h>
#include <esp_flash.h>

// Local logging tag
static const char TAG[] = "Main";

Model* model;

TaskHandle_t wifiManagerTask;

#define PAYLOAD_SIZE 20
static uint8_t payload[PAYLOAD_SIZE];

const uint32_t debounceDelay = 50;
volatile DRAM_ATTR uint32_t lastBtnDebounceTime = 0;
volatile DRAM_ATTR uint8_t buttonState = 0;
uint8_t oldConfirmedButtonState = 0;
uint32_t lastConfirmedBtnPressedTime = 0;

void IRAM_ATTR buttonHandler() {
  buttonState = (digitalRead(BUTTON_PIN) ? 0 : 1);
  lastBtnDebounceTime = millis();
}

void reboot(TimerHandle_t xTimer) {
  esp_restart();
}

void goToDeepSleep() {
  //   if (radio1) radio1->goToSleep();
}

boolean saveConfigEvt = false;
boolean updateLoraSettingsEvt = false;

void saveConfig(TimerHandle_t xTimer) {
  if (xTimer != nullptr) saveConfigEvt = true;
}

void modelUpdatedEvt(uint16_t mask) {
  if ((mask & ~M_CONFIG_CHANGED) != M_NONE) {
    char buf[8];
    DynamicJsonDocument* doc = new DynamicJsonDocument(1024);
    if (mask & M_LIVE_DATA) {
      LiveData liveData = model->getLiveData();
      (*doc)["panel_mV"] = liveData.panel_mV;
      (*doc)["load_mV"] = liveData.load_mV;
      (*doc)["battery_mV"] = liveData.battery_mV;
      (*doc)["panel_mA"] = liveData.panel_mA;
      (*doc)["load_mA"] = liveData.load_mA;
      (*doc)["battery_mA"] = liveData.battery_mA;
      (*doc)["batterySoc"] = liveData.batterySoc;
      (*doc)["batteryCurrent_mA"] = liveData.batteryCurrent_mA;
    }
    if (mask & M_STATS_DATA) {
      StatsData statsData = model->getStatsData();
      (*doc)["battery_min_mV"] = statsData.battery_min_mV;
      (*doc)["battery_max_mV"] = statsData.battery_max_mV;
      (*doc)["panel_min_mV"] = statsData.panel_min_mV;
      (*doc)["panel_max_mV"] = statsData.panel_max_mV;
      (*doc)["consumed_Wh"] = statsData.consumed_Wh;
      (*doc)["generated_Wh"] = statsData.generated_Wh;
      (*doc)["heatsinkTempCenti"] = statsData.heatsinkTemp_centiDeg;
      (*doc)["statusBattery"] = statsData.statusBattery;
      (*doc)["statusCharger"] = statsData.statusCharger;
      (*doc)["statusDischarger"] = statsData.statusDischarger;
    }
    mqtt::publishData(doc);
  }
}

void updateLoraSettings(TimerHandle_t xTimer) {
  if (xTimer != nullptr) updateLoraSettingsEvt = true;
}

void logCoreInfo() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
    flash_size = 0;
  }
  ESP_LOGI(TAG,
    "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
    "%d, %dMB %s Flash",
    chip_info.cores,
    (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
    chip_info.revision, flash_size / (1024 * 1024),
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
    : "external");
  ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
    ESP.getHeapSize(), ESP.getFreeHeap());
#ifdef BOARD_HAS_PSRAM
  ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d",
    ESP.getPsramSize(), ESP.getFreePsram());
#endif
  ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
    ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
    ESP.getFlashChipSpeed());
}

#include <esp_heap_caps.h>
void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char* function_name) {
  ESP_LOGE(TAG, "%s was called but failed to allocate %d bytes with %04x%04x capabilities.", function_name, requested_size, (uint16_t)(caps >> 16), (uint16_t)(caps & 0x0000ffff));
}

RTC_NOINIT_ATTR uint32_t liveDataSent;
RTC_NOINIT_ATTR uint32_t statsDataSent;

void setup() {
  // WDT
  esp_task_wdt_config_t wdt_config;
  wdt_config.timeout_ms = 60000;
  wdt_config.trigger_panic = true;
  wdt_config.idle_core_mask = 1;

  esp_err_t error = heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  esp_err_t err = esp_task_wdt_reconfigure(&wdt_config);

  // Serial
  Serial.begin(115200);

  if (err == ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "esp_task_wdt_reconfigure TWDT not initialized yet");
  } else  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_task_wdt_reconfigure failed");
  }

  // HW init
  pinMode(RELAIS, OUTPUT);
  digitalWrite(RELAIS, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Determine ResetReason
  ResetReason resetReason = Power::afterReset();
  boolean reinitFromSleep = (resetReason == RR_WAKE_FROM_SLEEPTIMER || resetReason == RR_WAKE_FROM_BUTTON || resetReason == RR_WAKEUP_EXT1);

  // disable BT
  esp_bt_controller_disable();

  // RtcOsc
  RtcOsc::setupRtc();

  // Logger
  esp_log_set_vprintf(logging::logger);

#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setDebugOutput(true);
#endif

  ESP_LOGI(TAG, "ESP Solar monitor v%s. Built from %s @ %s", APP_VERSION, SRC_REVISION, BUILD_TIMESTAMP);

  model = new Model(modelUpdatedEvt);

  logCoreInfo();

  // Configuration
  setupConfigManager();
  printConfigurationFile();
  if (!validateConfiguration()) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
    printConfigurationFile();
  }
  if (!loadConfiguration(config)) {
    ESP_LOGE(TAG, "ERROR!!!");
    assert(0);
  }

  //  logConfiguration(config);

  // Init time
  Timekeeper::init();

  // determine wakeup cause

  RunMode rm = Power::getRunMode();
  if (rm == RM_UNDEFINED) rm = RM_FULL;
  Power::setRunMode(rm);
  if (rm != RM_FULL || !config.wifiEnabled) {
    esp_wifi_stop();
    Power::setMainClock(10);
  } else {
    Power::setMainClock(80);
  }

  // Housekeeping
  housekeeping::cyclicTimer.attach_ms(60 * 1000, housekeeping::doHousekeeping);

  // Wifi
  if (rm == RM_FULL && config.wifiEnabled) {
    //esp_sntp_servermode_dhcp(1); // needs to be set before Wifi connects and gets DHCP IP
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WifiManager::eventHandler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WifiManager::eventHandler, NULL, NULL));
    WifiManager::setupWifiManager("SolarMon", getConfigParameters(), false, true);
    wifiManagerTask = WifiManager::start(
      "wifiManagerLoop",  // name of task
      8192,               // stack size of task}
      1,                  // priority of the task
      0);                 // CPU core

    mqtt::setupMqtt("SolarMon");

    char msg[128];
    sprintf(msg, "Reset reason: %u", resetReason);
    mqtt::publishStatusMsg(msg);

    xTaskCreate(
      mqtt::mqttLoop,     // task function
      "mqttLoop",         // name of task
      8192,               // stack size of task
      (void*)1,           // parameter of the task
      2,                  // priority of the task
      &mqtt::mqttTask);   // task handle
  }

  esp_log_level_set("*", ESP_LOG_VERBOSE);

  /*
  coredump::init();
  if (coredump::checkForCoredump()) {
    coredump::logCoredumpSummary();
    ESP_LOGE(TAG, "Coredump found!!!");
  }*/

  attachInterrupt(BUTTON_PIN, buttonHandler, CHANGE);

  setupModbus();

  if (!reinitFromSleep) {
    liveDataSent = millis() - (((config.liveDataInterval * 1000ul) / 10) * 9);
    statsDataSent = millis() - (((config.statsDataInterval * 1000ul) / 10) * 9);
  }
  ESP_LOGI(TAG, "Setup done.");
}

uint32_t lastSleep;
WakeUpReason wr;

boolean relais = false;

void loop() {

  if (saveConfigEvt) {
    saveConfigEvt = false;
    ESP_LOGI(TAG, "saveConfigEvt");
    saveConfiguration(config);
    model->configurationChanged();
  }

  if (updateLoraSettingsEvt) {
    updateLoraSettingsEvt = false;
    ESP_LOGI(TAG, "updateLoraSettingsEvt");
  }

  if (buttonState != oldConfirmedButtonState && (millis() - lastBtnDebounceTime) > debounceDelay) {
    oldConfirmedButtonState = buttonState;
    if (oldConfirmedButtonState == 1) {
      lastConfirmedBtnPressedTime = millis();
    } else if (oldConfirmedButtonState == 0) {
      uint16_t btnPressTime = millis() - lastConfirmedBtnPressedTime;
      ESP_LOGD(TAG, "lastConfirmedBtnPressedTime - millis() %u", btnPressTime);
      if (btnPressTime < 2000) {
        //        readHoldingRegisters(0x9013ul, 0x03);
        //        readLiveData((uint8_t*)&payload);
        //        readStatsData((uint8_t*)&payload);
        readRTC((uint8_t*)&payload);

      } else if (btnPressTime < 5000) {
        if (Timekeeper::hasValidTime()) {

          time_t now;
          struct tm localTime;
          time(&now);
          localtime_r(&now, &localTime);
          ESP_LOGI(TAG, "Local time: %02i:%02i.%02i %02i.%02i.%04i", localTime.tm_hour, localTime.tm_min, localTime.tm_sec, localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);

          /*
          [0]Sec
        [1]Min
        [2]H
        [3]Day
        [4]Mon
        [5]Year
          */
          payload[0] = localTime.tm_sec;
          payload[1] = localTime.tm_min;
          payload[2] = localTime.tm_hour;
          payload[3] = localTime.tm_mday;
          payload[4] = localTime.tm_mon + 1;
          payload[5] = localTime.tm_year - 100;
          writeRTC((uint8_t*)&payload);
        }
      } else if (btnPressTime > 5000) {
        WifiManager::startCaptivePortal();
      }
    }
  }

  if (config.liveDataInterval >= LIVE_DATA_MIN_INTERVAL && ((Power::getUpTime() - liveDataSent) > (config.liveDataInterval * 1000ul))) {
    ESP_LOGI(TAG, "Sending LiveData.....");
    model->updateModel(readLiveData());
    liveDataSent = Power::getUpTime();
  }

  if (config.statsDataInterval >= STATS_DATA_MIN_INTERVAL && ((Power::getUpTime() - statsDataSent) > (config.statsDataInterval * 1000ul))) {
    ESP_LOGI(TAG, "Sending StatsData.....");
    model->updateModel(readStatsData());
    statsDataSent = Power::getUpTime();
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}
