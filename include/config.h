#pragma once

#include <Arduino.h>
#include <logging.h>
#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP32C6

#define BUTTON_PIN             9

#define USE_SX1268

#define LORA_RESET           10
#define LORA_DIO0            19
#define LORA_DIO1            18    
#define LORA_BUSY            -1
#define LORA_CS               8

#define SPI_MOSI                5
#define SPI_MISO                4
#define SPI_SCK                 1

#define RS485_RX               11
#define RS485_TX                6
#define RS485_RE                0
#define RS485_DE                7

#define RELAIS                 20

#define UART1_TX               22
#define UART1_RX               23

#endif



static const char* CONFIG_FILENAME = "/config.json";

// ----------------------------  Config struct ------------------------------------- 
#define CONFIG_SIZE (1800ul)

#define SSID_LEN                  32
#define WIFI_PASSWORD_LEN         64

#define TIMEZONE_LEN              30

struct Config {

  uint8_t solarControllerNodeId = 1;

  boolean wifiEnabled;

  char timezone[TIMEZONE_LEN + 1];

  boolean hasRelais = false;
  uint8_t relaisDuration = 3;
  boolean hasLora = false;
};

