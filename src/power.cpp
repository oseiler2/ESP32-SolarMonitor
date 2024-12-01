#include <power.h>
#include <config.h>
#include <nvs_config.h>
#include <sdkconfig.h>
#include <driver/gpio.h>
#include <rom/rtc.h>
#include <driver/rtc_io.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = "Power";

namespace Power {

  RTC_NOINIT_ATTR RunMode runmode;
  RTC_NOINIT_ATTR struct timeval RTC_sleep_start_time;
  RTC_NOINIT_ATTR uint32_t RTC_millis;

  void resetRtcVars() {
    runmode = RM_UNDEFINED;
    RTC_millis = 0;
  }

  RunMode getRunMode() {
    return runmode;
  }

  boolean setRunMode(RunMode pMode) {
    if (pMode == RM_UNDEFINED) return false;
    runmode = pMode;
    ESP_LOGD(TAG, "Run mode: %s", runmode == RM_UNDEFINED ? "RM_UNDEFINED" : (runmode == RM_BAT_SAFE ? "battery safe" : (runmode == RM_LOW ? "conserve power" : "full power")));
    return true;
  }

  void enableGpioPullDn(int pin) {
    if (gpio_pulldown_en((gpio_num_t)pin) != ESP_OK) ESP_LOGE(TAG, "error in gpio_pulldown_en: %i", pin);
  }

  void enableGpioHold(int pin) {
    if (gpio_hold_en((gpio_num_t)pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_en: %i", pin);
  }

  void disableGpioHold(int pin) {
    if (gpio_hold_dis((gpio_num_t)pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_dis: %i", pin);
  }

  void enableRtcHold(int pin) {
    if (rtc_gpio_hold_en((gpio_num_t)pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_en: %i", pin);
  }

  void disableRtcHold(int pin) {
    if (rtc_gpio_hold_dis((gpio_num_t)pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_dis: %i", pin);
  }

  void setGpioSleepPullMode(int pin, gpio_pull_mode_t pullMode) {
    if (gpio_sleep_set_pull_mode((gpio_num_t)pin, pullMode) != ESP_OK) ESP_LOGE(TAG, "error in gpio_sleep_set_pull_mode: %i, %i", pin, pullMode);
  }

  ResetReason afterReset() {
    ResetReason reason = RR_UNDEFINED;

    struct timeval sleep_stop_time;
    uint64_t sleep_time_ms;

    switch (rtc_get_reset_reason(0)) {
      case DEEPSLEEP_RESET:           /**<5, Deep Sleep reset digital core (hp system)*/
        // keep state
        // calculate time spent in deep sleep
        gettimeofday(&sleep_stop_time, NULL);
        sleep_time_ms =
          (sleep_stop_time.tv_sec - RTC_sleep_start_time.tv_sec) * 1000 +
          (sleep_stop_time.tv_usec - RTC_sleep_start_time.tv_usec) / 1000;
        RTC_millis = RTC_millis + sleep_time_ms;
        ESP_LOGD(TAG, "Slept for %u ms, RTC_millis %u, upTime %u", sleep_time_ms, RTC_millis, getUpTime());
        break;
      case NO_MEAN:
      case POWERON_RESET:             /**<1, Vbat power on reset*/
      case RTC_SW_SYS_RESET:          /**<3, Software reset digital core (hp system)*/
      case SDIO_RESET:                /**<6, Reset by SLC module, reset digital core (hp system)*/
      case TG0WDT_SYS_RESET:          /**<7, Timer Group0 Watch dog reset digital core (hp system)*/
      case TG1WDT_SYS_RESET:          /**<8, Timer Group1 Watch dog reset digital core (hp system)*/
      case RTCWDT_SYS_RESET:          /**<9, RTC Watch dog Reset digital core (hp system)*/
      case TG0WDT_CPU_RESET:          /**<11, Time Group0 reset CPU*/
      case RTC_SW_CPU_RESET:          /**<12, Software reset CPU*/
      case RTCWDT_CPU_RESET:          /**<13, RTC Watch dog Reset CPU*/
      case RTCWDT_BROWN_OUT_RESET:    /**<15, Reset when the vdd voltage is not stable*/
      case RTCWDT_RTC_RESET:          /**<16, RTC Watch dog reset digital core and rtc module*/
      case TG1WDT_CPU_RESET:          /**<17, Time Group1 reset CPU*/
      case SUPER_WDT_RESET:           /**<18, super watchdog reset digital core and rtc module*/
      case EFUSE_RESET:               /**<20, efuse reset digital core (hp system)*/
      case USB_UART_CHIP_RESET:       /**<21, usb uart reset digital core (hp system)*/
      case USB_JTAG_CHIP_RESET:       /**<22, usb jtag reset digital core (hp system)*/
      case JTAG_RESET:                /**<24, jtag reset CPU*/
      default:
        resetRtcVars();
        reason = RR_FULL_RESET;
        break;
    }

    switch (esp_sleep_get_wakeup_cause()) {
      case ESP_SLEEP_WAKEUP_EXT0:
        reason = RR_WAKE_FROM_BUTTON;
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        reason = RR_WAKE_FROM_SLEEPTIMER;
        break;
      case ESP_SLEEP_WAKEUP_EXT1:
        reason = RR_WAKEUP_EXT1;
        break;
      case ESP_SLEEP_WAKEUP_UNDEFINED:
      case ESP_SLEEP_WAKEUP_ALL:

      case ESP_SLEEP_WAKEUP_TOUCHPAD:
      case ESP_SLEEP_WAKEUP_ULP:
      case ESP_SLEEP_WAKEUP_GPIO:
      case ESP_SLEEP_WAKEUP_UART:
      case ESP_SLEEP_WAKEUP_WIFI:
      case ESP_SLEEP_WAKEUP_COCPU:
      case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
      case ESP_SLEEP_WAKEUP_BT:
      default:
        break;
    }

    switch (runmode) {
      case RM_UNDEFINED: ESP_LOGI(TAG, "Run mode: RM_UNDEFINED"); break;
      case RM_BAT_SAFE:  ESP_LOGI(TAG, "Run mode: RM_BAT_SAFE"); break;
      case RM_LOW: ESP_LOGI(TAG, "Run mode: RM_LOW"); break;
      case RM_FULL: ESP_LOGI(TAG, "Run mode: RM_FULL");  break;
      default: ESP_LOGE(TAG, "Run mode: UNKNOWN (%u)", runmode); break;
    }

    switch (reason) {
      case RR_UNDEFINED: ESP_LOGI(TAG, "Reset reason: RR_UNDEFINED"); break;
      case RR_FULL_RESET: ESP_LOGI(TAG, "Reset reason: FULL_RESET"); break;
      case RR_WAKE_FROM_SLEEPTIMER: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_SLEEPTIMER"); break;
      case RR_WAKE_FROM_BUTTON: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_BUTTON"); break;
      case RR_WAKEUP_EXT1:  ESP_LOGI(TAG, "Reset reason: RR_WAKEUP_EXT1"); break;
      default: ESP_LOGI(TAG, "Reset reason: UNKNOWN (%u)", reason); break;
    }

    disableGpioHold(LORA_CS);

    return reason;
  }

  void prepareForLightSleep() {}

  WakeUpReason lightSleep(uint32_t durationInMSec) {
    prepareForLightSleep();

    int result = esp_sleep_enable_timer_wakeup((uint64_t)durationInMSec * 1000UL);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_timer_wakeup: %i", result);

    // wake up when BUTTON_PIN is changing from VDD to GND
    result = gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in gpio_wakeup_enable: %i", result);

    // BUTTON wake up
    result = esp_sleep_enable_gpio_wakeup();
    if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_gpio_wakeup: %i", result);

    // Radio int wake up
//    result = esp_sleep_enable_ext1_wakeup_io(1 << LORA_0_DIO1 , ESP_EXT1_WAKEUP_ANY_HIGH);
  //  if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_ext1_wakeup_io: %i", result);

    ESP_LOGD(TAG, "Light sleep now %u", durationInMSec);
    Serial.flush();

    setGpioSleepPullMode(LORA_CS, GPIO_PULLUP_ONLY);
    setGpioSleepPullMode(LORA_RESET, GPIO_PULLUP_ONLY);

    uint32_t now = millis();
    result = esp_light_sleep_start();
    if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_light_sleep_start: %i", result);


    WakeUpReason wr = getWakeUpReason();
    ESP_LOGD(TAG, "Wakey wakey, slept %u ms, %s", millis() - now,
      wr == WAKE_FROM_BUTTON_EXT0 ? "button" :
      wr == WAKE_FROM_RADIO_INT ? "radio int" :
      wr == WAKE_FROM_BUTTON_GPIO ? "GPIO" :
      wr == WAKE_FROM_SLEEPTIMER ? "timer" :
      "UNKNOWN"
    );
    return wr;
  }

  void prepareForDeepSleep(bool powerDown) {
    ESP_LOGI(TAG, "prepareForDeepSleep(%s)", powerDown ? "true" : "false");

    esp_wifi_stop();
    esp_bt_controller_disable();

    goToDeepSleep();
    NVS::close();

    digitalWrite(LORA_CS, 1);
    enableGpioHold(LORA_CS);


    setGpioSleepPullMode(LORA_RESET, GPIO_PULLUP_ONLY);

    setGpioSleepPullMode(SPI_MOSI, GPIO_PULLDOWN_ONLY);
    setGpioSleepPullMode(SPI_MISO, GPIO_PULLDOWN_ONLY);
    setGpioSleepPullMode(SPI_SCK, GPIO_PULLDOWN_ONLY);
  }

  void lightsOut();

  void deepSleep(uint32_t durationInSeconds) {
    prepareForDeepSleep(false);

    int result;
    if (durationInSeconds > 0) {
      result = esp_sleep_enable_timer_wakeup(durationInSeconds * 1000000UL);
      if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_timer_wakeup: %i", result);
    }

    lightsOut();
  }

  void powerDown() {
    prepareForDeepSleep(true);

    // @TODO: turn off all periphals possible!
    // check if RTC hold is required to pin down outputs

    lightsOut();
  }

  void lightsOut() {
#if SOC_PM_SUPPORT_RTC_PERIPH_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RTC_PERIPH");
#endif
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_XTAL");
#if SOC_PM_SUPPORT_RC_FAST_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RC_FAST");
#endif
#if SOC_PM_SUPPORT_CPU_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_CPU");
#endif
#if SOC_PM_SUPPORT_MODEM_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_MODEM, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_MODEM");
#endif
#if SOC_PM_SUPPORT_TOP_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_TOP, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_TOP");
#endif
#if SOC_PM_SUPPORT_VDDSDIO_PD
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_AUTO) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_VDDSDIO");
#endif

    ESP_LOGD(TAG, "Switching off - goodbye! Time: %u RTC_millis: %u", millis(), RTC_millis);
    Serial.flush();
    gettimeofday(&RTC_sleep_start_time, NULL);
    RTC_millis = RTC_millis + millis();

    esp_deep_sleep_start();
  }

  WakeUpReason getWakeUpReason() {
    WakeUpReason reason = WR_UNDEFINED;
    switch (esp_sleep_get_wakeup_cause()) {
      case ESP_SLEEP_WAKEUP_EXT0:
        reason = WAKE_FROM_BUTTON_EXT0;
        //        ESP_LOGI(TAG, "Wake up reason: WAKE_FROM_BUTTON_EXT0"); 
        break;
      case ESP_SLEEP_WAKEUP_EXT1:
        reason = WAKE_FROM_RADIO_INT;
        //        ESP_LOGI(TAG, "Wake up reason: WAKE_FROM_RADIO_INT");
        break;
      case ESP_SLEEP_WAKEUP_GPIO:
        reason = WAKE_FROM_BUTTON_GPIO;
        //        ESP_LOGI(TAG, "Wake up reason: WAKE_FROM_BUTTON_GPIO");
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        reason = WAKE_FROM_SLEEPTIMER;
        //        ESP_LOGI(TAG, "Wake up reason: WAKE_FROM_SLEEPTIMER");
        break;
      default:
        ESP_LOGW(TAG, "Wake up reason: RR_UNDEFINED (%u)", esp_sleep_get_wakeup_cause()); break;
    }
    return reason;
  }

  uint32_t getUpTime() {
    return RTC_millis + millis();
  }


  void setMainClock(uint32_t MHz) {
    ESP_LOGD(TAG, "About change CLK from %uMHz to %uMHz....", getCpuFrequencyMhz(), MHz);
    Serial.flush();
    setCpuFrequencyMhz(MHz);
    ESP_LOGD(TAG, "Done setting CLK to %uMHz, surrent Clock: %uMHz", MHz, getCpuFrequencyMhz());
  }

}
