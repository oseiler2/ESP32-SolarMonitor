#include <logging.h>
#include <esp_rom_sys.h>
#include <vector>
#include <esp32-hal-uart.h>

#include <hal/uart_ll.h>

namespace logging {

  DRAM_ATTR uint8_t recursionGuard = 0;

  DRAM_ATTR std::vector<logCallback_t> callbacks;

  void addOnLogCallback(logCallback_t logCallback) {
    callbacks.push_back(logCallback);
  }

  IRAM_ATTR int logger(const char* format, va_list args) {
    const uint16_t LOG_BUFFER_LEN = 255;
    if (recursionGuard != 0) return 0;
    recursionGuard = recursionGuard + 1;
    static char buffer[LOG_BUFFER_LEN] = { 0 };
    char* finalBuffer = buffer;
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (len >= LOG_BUFFER_LEN) {
      finalBuffer = (char*)malloc(len + 1);
      if (finalBuffer == NULL) {
        recursionGuard = recursionGuard - 1;
        return 0;
      }
    }
    len = vsnprintf(finalBuffer, len + 1, format, args);

    // Serial log
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
//    esp_rom_printf("%s\n", finalBuffer);
#if (ARDUINO_USB_CDC_ON_BOOT == 1 && ARDUINO_USB_MODE == 0) || CONFIG_IDF_TARGET_ESP32C3 \
  || ((CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C6) && ARDUINO_USB_CDC_ON_BOOT == 1)
    ets_printf("%s", finalBuffer);
#else
    for (int i = 0; i < len; i++) {
      ets_write_char_uart(finalBuffer[i]);
    }
#endif
#else
    ets_printf("%s\n", finalBuffer);
#endif

    // any additional log callbacks
    for (logCallback_t logCallback : callbacks) {
      logCallback(0, "", finalBuffer);
    }

    if (len >= LOG_BUFFER_LEN) {
      free(finalBuffer);
    }
    // flushes TX - make sure that the log message is completely sent.
    if (uartGetDebug() != -1) {
      while (!uart_ll_is_tx_idle(UART_LL_GET_HW(uartGetDebug())));
    }
    recursionGuard = recursionGuard - 1;
    return len;
  }

}