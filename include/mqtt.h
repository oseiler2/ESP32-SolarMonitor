#pragma once

#include <globals.h>
#include <ArduinoJson.h>

// If you issue really large certs (e.g. long CN, extra options) this value may need to be
// increased, but 1600 is plenty for a typical CN and standard option openSSL issued cert.
#define MQTT_CERT_SIZE 8192

// Use larger of cert or config for MQTT buffer size.
#define MQTT_BUFFER_SIZE MQTT_CERT_SIZE > CONFIG_SIZE ? MQTT_CERT_SIZE : CONFIG_SIZE


namespace mqtt {

  void setupMqtt(const char* appName);

  void publishData(DynamicJsonDocument* _payload);
  void publishConfiguration();
  void publishStatusMsg(const char* statusMessage);

  void mqttLoop(void* pvParameters);

  extern TaskHandle_t mqttTask;
}
