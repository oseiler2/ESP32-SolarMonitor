#include <configManager.h>

#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Local logging tag
static const char TAG[] = "ConfigManager";

Config config;

// Allocate a temporary JsonDocument
// Don't forget to change the capacity to match your requirements.
// Use arduinojson.org/v6/assistant to compute the capacity.
/*
{

}
*/

#define DEFAULT_DEVICE_ID                    0
#define DEFAULT_MQTT_TOPIC      "solarmonitor"
#define DEFAULT_MQTT_HOST          "127.0.0.1"
#define DEFAULT_MQTT_PORT                 1883
#define DEFAULT_MQTT_USERNAME   "solarmonitor"
#define DEFAULT_MQTT_PASSWORD   "solarmonitor"
#define DEFAULT_MQTT_USE_TLS             false
#define DEFAULT_MQTT_INSECURE            false
std::vector<ConfigParameterBase<Config>*> configParameterVector;

void setupConfigManager() {
  if (!LittleFS.begin(true)) {
    ESP_LOGW(TAG, "LittleFS failed! Already tried formatting.");
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!LittleFS.begin()) {
      ESP_LOGW(TAG, "LittleFS failed second time!");
    }
  }

  //  configParameterVector.clear();
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("deviceId", "Device ID", &Config::deviceId, DEFAULT_DEVICE_ID, CONFIG_TYPE_MQTT, false, false));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttTopic", "MQTT topic", (char Config::*) & Config::mqttTopic, DEFAULT_MQTT_TOPIC, MQTT_TOPIC_LEN, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttUsername", "MQTT username", (char Config::*) & Config::mqttUsername, DEFAULT_MQTT_USERNAME, MQTT_USERNAME_LEN, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttPassword", "MQTT password", (char Config::*) & Config::mqttPassword, DEFAULT_MQTT_PASSWORD, MQTT_PASSWORD_LEN, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttHost", "MQTT host", (char Config::*) & Config::mqttHost, DEFAULT_MQTT_HOST, MQTT_HOSTNAME_LEN, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("mqttServerPort", "MQTT port", &Config::mqttServerPort, DEFAULT_MQTT_PORT, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttUseTls", "MQTT use TLS", &Config::mqttUseTls, DEFAULT_MQTT_USE_TLS, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttInsecure", "MQTT ignore certificate errors", &Config::mqttInsecure, DEFAULT_MQTT_INSECURE, CONFIG_TYPE_MQTT));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("timezone", "Timezone", (char Config::*) & Config::timezone, "NZST-12NZDT,M9.5.0,M4.1.0/3", TIMEZONE_LEN, CONFIG_TYPE_NONE));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("controllerNodeId", "Solar Controller Node Id", &Config::solarControllerNodeId, 1, CONFIG_TYPE_NONE, false, true));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("hasRelais", "Board has relais populated", &Config::hasRelais, false, CONFIG_TYPE_NONE));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("relaisDuration", "Duration(s) for the relais to operate", &Config::relaisDuration, 3, CONFIG_TYPE_NONE, false, false));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("hasLora", "Board has Lora module populated", &Config::hasLora, false, CONFIG_TYPE_LORA, true));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("wifi", "WiFi enable", &Config::wifiEnabled, true, CONFIG_TYPE_NONE, true));
  configParameterVector.push_back(new Uint32ConfigParameter<Config>("liveDataInt", "LiveData interval", &Config::liveDataInterval, 300ul, LIVE_DATA_MIN_INTERVAL, 0xfffffffful, CONFIG_TYPE_NONE, true));
  configParameterVector.push_back(new Uint32ConfigParameter<Config>("statsDataInt", "StatsData interval", &Config::statsDataInterval, 3600ul, STATS_DATA_MIN_INTERVAL, 0xfffffffful, CONFIG_TYPE_NONE, true));
}

std::vector<ConfigParameterBase<Config>*> getConfigParameters() {
  return configParameterVector;
}

void getDefaultConfiguration(Config& _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->setToDefault(_config);
  }
}

void logConfiguration(const Config _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    ESP_LOGD(TAG, "%s: %s", configParameter->getId(), configParameter->toString(_config).c_str());
  }
}

boolean validateConfiguration() {
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Could not open config file");
    return false;
  }
  StaticJsonDocument<0> doc, filter;
  boolean result = deserializeJson(doc, file, DeserializationOption::Filter(filter)) == DeserializationError::Ok;
  file.close();
  ESP_LOGV(TAG, "result %u", result);
  return result;
}

boolean loadConfiguration(Config& _config) {
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Could not open config file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(CONFIG_SIZE);
  if (doc.capacity() == 0) {
    ESP_LOGE(TAG, "DynamicJsonDocument allocation failed!");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    ESP_LOGE(TAG, "Failed to parse config file: %s", error.f_str());
    file.close();
    return false;
  }

  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->fromJson(_config, &doc, true);
  }

  file.close();
  return true;
}

boolean saveConfiguration(const Config _config) {
  ESP_LOGD(TAG, "###################### saveConfiguration");
  logConfiguration(_config);
  // Delete existing file, otherwise the configuration is appended to the file
  if (LittleFS.exists(CONFIG_FILENAME)) {
    LittleFS.remove(CONFIG_FILENAME);
  }

  // Open file for writing
  File file = LittleFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (!file) {
    ESP_LOGW(TAG, "Could not create config file for writing");
    return false;
  }

  DynamicJsonDocument doc(CONFIG_SIZE);
  if (doc.capacity() == 0) {
    ESP_LOGE(TAG, "DynamicJsonDocument allocation failed!");
    return false;
  }
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->toJson(_config, &doc);
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGW(TAG, "Failed to write to file");
    file.close();
    return false;
  }

  // Close the file
  file.close();
  ESP_LOGD(TAG, "Stored configuration successfully");
  return true;
}

// Prints the content of a file to the Serial
void printConfigurationFile() {
  // Open file for reading
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}
