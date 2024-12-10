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
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
static const char* MQTT_CLIENT_CERT_FILENAME = "/mqtt_client_cert.pem";
static const char* MQTT_CLIENT_KEY_FILENAME = "/mqtt_client_key.pem";
static const char* TEMP_MQTT_ROOT_CA_FILENAME = "/temp_mqtt_root_ca.pem";
static const char* ROOT_CA_FILENAME = "/root_ca.pem";

#define MQTT_QUEUE_LENGTH      25

// ----------------------------  Config struct ------------------------------------- 
#define CONFIG_SIZE (1800ul)

#define LIVE_DATA_MIN_INTERVAL            (60ul)
#define STATS_DATA_MIN_INTERVAL           (60ul)
#define MQTT_USERNAME_LEN 20
#define MQTT_PASSWORD_LEN 20
#define MQTT_HOSTNAME_LEN 30
#define MQTT_TOPIC_LEN 30
#define SSID_LEN                  32
#define WIFI_PASSWORD_LEN         64

#define TIMEZONE_LEN              30

const uint8_t CONFIG_TYPE_NONE = 0x00u;
const uint8_t CONFIG_TYPE_MQTT = bit(0);
const uint8_t CONFIG_TYPE_LORA = bit(1);

struct Config {
  uint16_t deviceId;
  char mqttTopic[MQTT_TOPIC_LEN + 1];
  char mqttUsername[MQTT_USERNAME_LEN + 1];
  char mqttPassword[MQTT_PASSWORD_LEN + 1];
  char mqttHost[MQTT_HOSTNAME_LEN + 1];
  bool mqttUseTls;
  bool mqttInsecure;
  uint16_t mqttServerPort;
  uint8_t solarControllerNodeId = 1;

  boolean wifiEnabled;
  
  uint32_t liveDataInterval;
  uint32_t statsDataInterval;
  
  char timezone[TIMEZONE_LEN + 1];

  boolean hasRelais = false;
  uint8_t relaisDuration = 3;
  boolean hasLora = false;
};

