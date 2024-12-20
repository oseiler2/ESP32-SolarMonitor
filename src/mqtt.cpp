#include <globals.h>
#include <mqtt.h>
#include <config.h>

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <configManager.h>
#include <wifiManager.h>
#include <events.h>
//#include <ota.h>

#include <LittleFS.h>

// Local logging tag
static const char TAG[] = "MQTT";

namespace mqtt {

  struct MqttMessage {
    uint8_t cmd;
    DynamicJsonDocument* payload;
    char* statusMessage;
  };

  const uint8_t X_CMD_PUBLISH_DATA = bit(0);
  const uint8_t X_CMD_PUBLISH_CONFIGURATION = bit(1);
  const uint8_t X_CMD_PUBLISH_STATUS_MSG = bit(2);

  const char* appName = { 0 };

  TaskHandle_t mqttTask;
  QueueHandle_t mqttQueue;

  WiFiClient* wifiClient;
  PubSubClient* mqtt_client;

  uint32_t lastReconnectAttempt = 0;
  uint16_t connectionAttempts = 0;

  char* cloneStr(const char* original) {
    char* copy = (char*)malloc(strlen(original) + 1);
    strncpy(copy, original, strlen(original));
    copy[strlen(original)] = 0x00;
    return copy;
  }

  void publishData(DynamicJsonDocument* _payload) {
    if (!WiFi.isConnected() || !mqtt_client->connected()) {
      delete _payload;
      return;
    }
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_DATA;
    msg.payload = _payload;
    if (!mqttQueue || !xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100))) {
      delete msg.payload;
    }
  }

  boolean publishDataInternal(MqttMessage queueMsg) {
    char topic[256];
    char msg[256];
    sprintf(topic, "%s/%u/up/data", config.mqttTopic, config.deviceId);

    // Serialize JSON to file
    if (serializeJson(*queueMsg.payload, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      delete queueMsg.payload;
      return true; // pretend to have been successful to prevent queue from clogging up
    }
    if (strncmp(msg, "null", 4) == 0) {
      ESP_LOGD(TAG, "Nothing to publish");
      delete queueMsg.payload;
      return true; // pretend to have been successful to prevent queue from clogging up
    }
    ESP_LOGD(TAG, "Publishing data values: %s:%s", topic, msg);
    if (!mqtt_client->publish(topic, msg)) {
      ESP_LOGI(TAG, "publish data failed!");
      return false;
    }
    delete queueMsg.payload;
    return true;
  }

  void publishConfiguration() {
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_CONFIGURATION;
    msg.statusMessage = nullptr;
    if (mqttQueue) xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100));
  }

  void setMqttCerts(WiFiClientSecure* wifiClient, const char* mqttRootCertFilename, const char* mqttClientKeyFilename, const char* mqttClientCertFilename) {
    File root_ca_file = LittleFS.open(mqttRootCertFilename, FILE_READ);
    if (root_ca_file) {
      ESP_LOGD(TAG, "Loading MQTT root ca from FS (%s)", mqttRootCertFilename);
      wifiClient->loadCACert(root_ca_file, root_ca_file.size());
      root_ca_file.close();
    }
    File client_key_file = LittleFS.open(mqttClientKeyFilename, FILE_READ);
    if (client_key_file) {
      ESP_LOGD(TAG, "Loading MQTT client key from FS (%s)", mqttClientKeyFilename);
      wifiClient->loadPrivateKey(client_key_file, client_key_file.size());
      client_key_file.close();
    }
    File client_cert_file = LittleFS.open(mqttClientCertFilename, FILE_READ);
    if (client_cert_file) {
      ESP_LOGD(TAG, "Loading MQTT client cert from FS (%s)", mqttClientCertFilename);
      wifiClient->loadCertificate(client_cert_file, client_cert_file.size());
      client_cert_file.close();
    }
  }

  boolean testMqttConfig(WiFiClient* wifiClient, Config testConfig) {
    char buf[128];
    boolean mqttTestSuccess;
    PubSubClient* testMqttClient = new PubSubClient(*wifiClient);
    testMqttClient->setServer(testConfig.mqttHost, testConfig.mqttServerPort);
    sprintf(buf, "%s-%u-%s", appName, testConfig.deviceId, WifiManager::getMac().c_str());
    // disconnect current connection if not enough heap avalable to initiate another tls session.
    if (testConfig.mqttUseTls && ESP.getFreeHeap() < 75000) mqtt_client->disconnect();
    mqttTestSuccess = testMqttClient->connect(buf, testConfig.mqttUsername, testConfig.mqttPassword);
    if (mqttTestSuccess) {
      ESP_LOGD(TAG, "Test MQTT connected");
      sprintf(buf, "%s/%u/up/status", testConfig.mqttTopic, testConfig.deviceId);
      mqttTestSuccess = testMqttClient->publish(buf, "{\"test\":true}");
      if (!mqttTestSuccess) ESP_LOGI(TAG, "connecting using new mqtt settings failed!");
    }
    delete testMqttClient;
    return mqttTestSuccess;
  }

  boolean publishConfigurationInternal() {
    char buf[256];
    char msg[CONFIG_SIZE];
    DynamicJsonDocument doc(CONFIG_SIZE);
    doc["appVersion"] = APP_VERSION;
    sprintf(buf, "%s", WifiManager::getMac().c_str());
    doc["mac"] = buf;
    sprintf(buf, "%s", WiFi.localIP().toString().c_str());
    doc["ip"] = buf;

    for (ConfigParameterBase<Config>* configParameter : getConfigParameters()) {
      if (!(strncmp(configParameter->getId(), "deviceId", strlen(buf)) == 0)
        && !(strncmp(configParameter->getId(), "mqttPassword", strlen(buf)) == 0))
        configParameter->toJson(config, &doc);
    }

    if (serializeJson(doc, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      return true; // pretend to have been successful to prevent queue from clogging up
    }
    sprintf(buf, "%s/%u/up/config", config.mqttTopic, config.deviceId);
    ESP_LOGI(TAG, "Publishing configuration: %s:%s", buf, msg);
    if (!mqtt_client->publish(buf, msg)) {
      ESP_LOGI(TAG, "publish configuration failed!");
      return false;
    }
    return true;
  }

  void publishStatusMsg(const char* statusMessage) {
    if (strlen(statusMessage) > 200) {
      ESP_LOGW(TAG, "msg too long - discarding");
      return;
    }
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_STATUS_MSG;
    msg.statusMessage = cloneStr(statusMessage);
    if (!mqttQueue || !xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100))) {
      free(msg.statusMessage);
    }
  }

  boolean publishStatusMsgInternal(char* statusMessage, boolean keepOnFailure) {
    if (strlen(statusMessage) > 200) {
      free(statusMessage);
      return true;// pretend to have been successful to prevent queue from clogging up
    }
    char topic[256];
    sprintf(topic, "%s/%u/up/status", config.mqttTopic, config.deviceId);
    char msg[256];
    DynamicJsonDocument doc(CONFIG_SIZE);
    doc["msg"] = statusMessage;
    if (serializeJson(doc, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      free(statusMessage);
      return true;// pretend to have been successful to prevent queue from clogging up
    }
    if (!mqtt_client->publish(topic, msg)) {
      ESP_LOGI(TAG, "publish status msg failed!");
      if (!keepOnFailure) free(statusMessage);
      // don't free heap, since message will be re-tried
      return false;
    }
    free(statusMessage);
    return true;
  }

  // Helper to write a file to fs
  bool writeFile(const char* name, unsigned char* contents) {
    File f;
    if (!(f = LittleFS.open(name, FILE_WRITE))) {
      return false;
    }
    int len = strlen((char*)contents);
    if (f.write(contents, len) != len) {
      f.close();
      return false;
    }
    f.close();
    return true;
  }

  void callback(char* topic, byte* payload, unsigned int length) {
    char buf[256];
    char msg[length + 1];
    strncpy(msg, (char*)payload, length);
    msg[length] = 0x00;
    ESP_LOGI(TAG, "Message arrived [%s] %s", topic, msg);

    sprintf(buf, "%s/%u/down/", config.mqttTopic, config.deviceId);
    int16_t cmdIdx = -1;
    if (strncmp(topic, buf, strlen(buf)) == 0) {
      ESP_LOGI(TAG, "Device specific downlink message arrived [%s]", topic);
      cmdIdx = strlen(buf);
    }
    sprintf(buf, "%s/down/", config.mqttTopic);
    if (strncmp(topic, buf, strlen(buf)) == 0) {
      ESP_LOGI(TAG, "Device agnostic downlink message arrived [%s]", topic);
      cmdIdx = strlen(buf);
    }
    if (cmdIdx < 0) return;
    strncpy(buf, topic + cmdIdx, strlen(topic) - cmdIdx + 1);
    ESP_LOGI(TAG, "Received command [%s]", buf);

    if (strncmp(buf, "getConfig", strlen(buf)) == 0) {
      publishConfiguration();
    } else if (strncmp(buf, "setConfig", strlen(buf)) == 0) {
      DynamicJsonDocument doc(CONFIG_SIZE);
      DeserializationError error = deserializeJson(doc, msg);
      if (error) {
        ESP_LOGW(TAG, "Failed to parse message: %s", error.f_str());
        return;
      }

      bool rebootRequired = false;
      Config mqttConfig = config;
      bool mqttConfigUpdated = false;
      int8_t res;
      for (ConfigParameterBase<Config>* configParameter : getConfigParameters()) {
        if (configParameter->getFunction() & CONFIG_TYPE_MQTT) {
          res = configParameter->fromJson(mqttConfig, &doc, false);
          if (res == CONFIG_PARAM_UPDATED) {
            mqttConfigUpdated = true;
            ESP_LOGI(TAG, "MQTT Config %s updated to %s", configParameter->getId(), configParameter->toString(mqttConfig).c_str());
          }
        } else {
          res = configParameter->fromJson(config, &doc, false);
          if (res == CONFIG_PARAM_UPDATED) {
            rebootRequired |= configParameter->isRebootRequiredOnChange();
            ESP_LOGI(TAG, "Config %s updated to %s. Reboot needed? %s", configParameter->getId(), configParameter->toString(config).c_str(), configParameter->isRebootRequiredOnChange() ? "true" : "false");
          }
        }
      }
      bool mqttTestSuccess = true;

      if (mqttConfigUpdated) {
        WiFiClient* testWifiClient;
        if (mqttConfig.mqttUseTls) {
          testWifiClient = new WiFiClientSecure();
          if (mqttConfig.mqttInsecure) {
            ((WiFiClientSecure*)testWifiClient)->setInsecure();
          }
          setMqttCerts((WiFiClientSecure*)testWifiClient, MQTT_ROOT_CA_FILENAME, MQTT_CLIENT_KEY_FILENAME, MQTT_CLIENT_CERT_FILENAME);
        } else {
          testWifiClient = new WiFiClient();
        }
        mqttTestSuccess = testMqttConfig(testWifiClient, mqttConfig);
        delete testWifiClient;
        if (mqttTestSuccess) {
          config = mqttConfig;
          rebootRequired = true;
        }
      }
      if (saveConfiguration(config) && rebootRequired) {
        publishStatusMsgInternal(cloneStr("configuration updated - rebooting shortly"), false);
        startTimer("Reboot", 2000, reboot);
      }
    } else if (strncmp(buf, "installMqttRootCa", strlen(buf)) == 0) {
      ESP_LOGD(TAG, "installMqttRootCa");
      if (!writeFile(TEMP_MQTT_ROOT_CA_FILENAME, (unsigned char*)&msg[0])) {
        ESP_LOGW(TAG, "Error writing mqtt root ca");
        publishStatusMsgInternal(cloneStr("Error writing cert to FS"), false);
        return;
      }
      bool mqttTestSuccess = config.mqttInsecure || !config.mqttUseTls; // no need to test if not using tls, or not checking certs
      if (config.mqttUseTls && !config.mqttInsecure) {
        ESP_LOGD(TAG, "test connection using new ca");
        // test connection using cert
        WiFiClientSecure* testWifiClient = new WiFiClientSecure();
        setMqttCerts(testWifiClient, TEMP_MQTT_ROOT_CA_FILENAME, MQTT_CLIENT_KEY_FILENAME, MQTT_CLIENT_CERT_FILENAME);
        mqttTestSuccess = testMqttConfig(testWifiClient, config);
        delete testWifiClient;
      }
      ESP_LOGD(TAG, "mqttTestSuccess %u", mqttTestSuccess);
      if (mqttTestSuccess) {
        if (LittleFS.exists(MQTT_ROOT_CA_FILENAME) && !LittleFS.remove(MQTT_ROOT_CA_FILENAME)) {
          ESP_LOGE(TAG, "Failed to remove original CA file");
          publishStatusMsgInternal(cloneStr("Could not remove original CA - giving up"), false);
          return;  // leave old file in place and give up.
        }
        if (!LittleFS.rename(TEMP_MQTT_ROOT_CA_FILENAME, MQTT_ROOT_CA_FILENAME)) {
          publishStatusMsgInternal(cloneStr("Could not replace original CA with new CA - PANIC - giving up"), false);
          ESP_LOGE(TAG, "Failed to move temporary CA file");
          config.mqttInsecure = true;
          saveConfiguration(config);
          startTimer("Reboot", 2000, reboot);
          return;
        }
        ESP_LOGI(TAG, "installed and tested new CA, rebooting shortly");
        publishStatusMsgInternal(cloneStr("installed and tested new CA - rebooting shortly"), false);
        startTimer("Reboot", 2000, reboot);
      } else {
        ESP_LOGI(TAG, "publish connect msg failed!");
        publishStatusMsgInternal(cloneStr("Connecting using the new CA failed - reverting"), false);
        if (!LittleFS.remove(TEMP_MQTT_ROOT_CA_FILENAME)) ESP_LOGW(TAG, "Failed to remove temporary CA file");
      }
    } else if (strncmp(buf, "installRootCa", strlen(buf)) == 0) {
      ESP_LOGD(TAG, "installRootCa");
      if (!writeFile(ROOT_CA_FILENAME, (unsigned char*)&msg[0])) {
        ESP_LOGW(TAG, "Error writing root ca");
        publishStatusMsgInternal(cloneStr("Error writing cert to FS"), false);
      }
    } else if (strncmp(buf, "resetWifi", strlen(buf)) == 0) {
      WifiManager::resetSettings();
    } else if (strncmp(buf, "ota", strlen(buf)) == 0) {
      //      OTA::checkForUpdate();
    } else if (strncmp(buf, "forceota", strlen(buf)) == 0) {
      //      OTA::forceUpdate(msg);
    } else if (strncmp(buf, "reboot", strlen(buf)) == 0) {
      startTimer("Reboot", 1000, reboot);
    }
  }

  void reconnect() {
    if (!WiFi.isConnected() || mqtt_client->connected()) return;
    if (millis() - lastReconnectAttempt < 60000) return;
    if (strncmp(config.mqttHost, "127.0.0.1", MQTT_HOSTNAME_LEN) == 0 ||
      strncmp(config.mqttHost, "localhost", MQTT_HOSTNAME_LEN) == 0) return;
    char topic[256];
    char id[64];
    sprintf(id, "%s-%u-%s", appName, config.deviceId, WifiManager::getMac().c_str());
    lastReconnectAttempt = millis();
    ESP_LOGD(TAG, "Attempting MQTT connection...");
    connectionAttempts++;
    sprintf(topic, "%s/%u/up/status", config.mqttTopic, config.deviceId);
    if (mqtt_client->connect(id, config.mqttUsername, config.mqttPassword, topic, 1, false, "{\"msg\":\"disconnected\"}")) {
      ESP_LOGD(TAG, "MQTT connected");
      sprintf(topic, "%s/%u/down/#", config.mqttTopic, config.deviceId);
      mqtt_client->subscribe(topic);
      sprintf(topic, "%s/down/#", config.mqttTopic);
      mqtt_client->subscribe(topic);
      sprintf(topic, "%s/%u/up/status", config.mqttTopic, config.deviceId);
      char msg[256];
      DynamicJsonDocument doc(CONFIG_SIZE);
      doc["online"] = true;
      doc["connectionAttempts"] = connectionAttempts;
      if (serializeJson(doc, msg) == 0) {
        ESP_LOGW(TAG, "Failed to serialise payload");
        return;
      }
      if (mqtt_client->publish(topic, msg))
        connectionAttempts = 0;
      else
        ESP_LOGI(TAG, "publish connect msg failed!");
    } else {
      ESP_LOGW(TAG, "MQTT connection failed, rc=%i", mqtt_client->state());
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  void logCallback(int level, const char* tag, const char* message) {
    publishStatusMsg(message);
  }

  void setupMqtt(const char* _appName) {
    appName = _appName;
    mqttQueue = xQueueCreate(MQTT_QUEUE_LENGTH, sizeof(struct MqttMessage));
    if (mqttQueue == NULL) {
      ESP_LOGE(TAG, "Queue creation failed!");
    }
    if (config.mqttUseTls) {
      wifiClient = new WiFiClientSecure();
      if (config.mqttInsecure) {
        ((WiFiClientSecure*)wifiClient)->setInsecure();
      }
      setMqttCerts((WiFiClientSecure*)wifiClient, MQTT_ROOT_CA_FILENAME, MQTT_CLIENT_KEY_FILENAME, MQTT_CLIENT_CERT_FILENAME);
    } else {
      wifiClient = new WiFiClient();
    }

    mqtt_client = new PubSubClient(*wifiClient);
    mqtt_client->setServer(config.mqttHost, config.mqttServerPort);
    mqtt_client->setCallback(callback);
    if (!mqtt_client->setBufferSize(MQTT_BUFFER_SIZE)) ESP_LOGE(TAG, "mqtt_client->setBufferSize failed!");

    //    logging::addOnLogCallback(logCallback);
  }

  void mqttLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    lastReconnectAttempt = millis() - 50000;
    BaseType_t notified;
    MqttMessage msg;
    while (1) {
      notified = xQueuePeek(mqttQueue, &msg, pdMS_TO_TICKS(100));
      if (notified == pdPASS) {
        if (mqtt_client->connected()) {
          if (msg.cmd == X_CMD_PUBLISH_CONFIGURATION) {
            if (publishConfigurationInternal()) {
              xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(100));
            }
          } else if (msg.cmd == X_CMD_PUBLISH_DATA) {
            // don't keep measurements in the queue should they fail to be published
            publishDataInternal(msg);
            xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(100));
          } else if (msg.cmd == X_CMD_PUBLISH_STATUS_MSG) {
            // keep status messages in the queue should they fail to be published
            if (publishStatusMsgInternal(msg.statusMessage, true)) {
              xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(100));
            }
          }
        }
      }
      if (!mqtt_client->connected()) {
        reconnect();
      }
      mqtt_client->loop();
      vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(NULL);
  }
}
