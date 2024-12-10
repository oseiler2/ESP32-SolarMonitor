#include <modbus.h>
#include <configManager.h>
#include <model.h>

HardwareSerial rs485Serial(1);

// Local logging tag
static const char TAG[] = "Modbus";

ModbusMaster node;

void preTransmission() {
  rs485Serial.flush();
  //delay(10);
  digitalWrite(RS485_RE, 1);
  digitalWrite(RS485_DE, 1);
}

void postTransmission() {
  digitalWrite(RS485_RE, 0);
  digitalWrite(RS485_DE, 0);
}

void setIdle() {
  digitalWrite(RS485_RE, 1);
  digitalWrite(RS485_DE, 0);
}

void setupModbus() {
  rs485Serial.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(10);
  pinMode(RS485_RE, OUTPUT);
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_RE, 0);
  digitalWrite(RS485_DE, 0);
  node.begin(config.solarControllerNodeId, rs485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  setIdle();
}

uint16_t getResponseBuffer(uint8_t u8Index) {
  return node.getResponseBuffer(u8Index);
}

uint8_t setTransmitBuffer(uint8_t u8Index, uint16_t u16Value) {
  return node.setTransmitBuffer(u8Index, u16Value);
}

uint8_t readInputRegisters(uint16_t addr, uint8_t qty) {
  uint8_t result;
  node.clearResponseBuffer();
  result = node.readInputRegisters(addr, qty);
  setIdle();
  if (result == ModbusMaster::ku8MBSuccess) {
    ESP_LOGD(TAG, "Read %04x:%04x", addr, node.getResponseBuffer(0));
  } else {
    ESP_LOGD(TAG, "Error %02x reading from %04x", result, addr);
  }
  return result;
}

uint8_t readInputRegister(uint16_t addr) {
  return readInputRegisters(addr, 1);
}

uint8_t readHoldingRegisters(uint16_t addr, uint8_t qty) {
  uint8_t result;
  node.clearResponseBuffer();
  result = node.readHoldingRegisters(addr, qty);
  setIdle();
  if (result == ModbusMaster::ku8MBSuccess) {
    ESP_LOGD(TAG, "Read %04x:%04x", addr, node.getResponseBuffer(0));
  } else {
    ESP_LOGD(TAG, "Error %02x reading from %04x", result, addr);
  }
  return result;
}

uint8_t readHoldingRegister(uint16_t addr) {
  return readHoldingRegisters(addr, 1);
}

void clearTransmitBuffer() {
  node.clearTransmitBuffer();
}

uint8_t writeMultipleRegisters(uint16_t addr, uint16_t qty) {
  uint8_t result = node.writeMultipleRegisters(addr, qty);
  setIdle();
  if (result == ModbusMaster::ku8MBSuccess) {
    ESP_LOGD(TAG, "Wrote %u bytes to %04x", qty, addr);

  } else {
    ESP_LOGD(TAG, "Error %02x writing %u bytes to %04x", result, qty, addr);
  }
  return result;
}


#define MAX_RETRY 3

uint8_t readInputRegisterRetry(uint16_t addr, uint8_t* payload, uint8_t payloadIdx = 0) {
  uint8_t i = 0;
  uint8_t result;
  do { result = readInputRegister(addr); } while (i++ < MAX_RETRY && result != ModbusMaster::ku8MBSuccess);
  if (result != ModbusMaster::ku8MBSuccess) return result;
  payload[payloadIdx] = lowByte(getResponseBuffer(0));
  payload[payloadIdx + 1] = highByte(getResponseBuffer(0));
  return result;
}

uint8_t readInputRegisterRetry(uint16_t addr) {
  uint8_t i = 0;
  uint8_t result;
  do { result = readInputRegister(addr); } while (i++ < MAX_RETRY && result != ModbusMaster::ku8MBSuccess);
  return result;
}

uint8_t readInputRegistersRetry(uint16_t addr, uint8_t qty) {
  uint8_t i = 0;
  uint8_t result;
  do { result = readInputRegisters(addr, qty); } while (i++ < MAX_RETRY && result != ModbusMaster::ku8MBSuccess);
  return result;
}


void readRTC(uint8_t* payload) {
  ESP_LOGI(TAG, "Reading RTC");
  if (readHoldingRegisters(0x9013ul, 0x03) != ModbusMaster::ku8MBSuccess) { return; }
  /*
  [0]Sec
[1]Min
[2]H
[3]Day
[4]Mon
[5]Year
  */
  payload[0] = lowByte(getResponseBuffer(0));
  payload[1] = highByte(getResponseBuffer(0));
  payload[2] = lowByte(getResponseBuffer(1));
  payload[3] = highByte(getResponseBuffer(1));
  payload[4] = lowByte(getResponseBuffer(2));
  payload[5] = highByte(getResponseBuffer(2));
  ESP_LOGI(TAG, "RTC: %2u/%02u/20%02u %2u:%02u.%02u",
    payload[3], payload[4], payload[5], payload[2], payload[1], payload[0]);
}

void writeRTC(uint8_t* payload) {
  ESP_LOGI(TAG, "Setting RTC to  %02u/%02u/20%02u %2u:%02u.%02u",
    payload[3], payload[4], payload[5], payload[2], payload[1], payload[0]);
  for (uint8_t i = 0; i < 3; i++) {
    setTransmitBuffer(i, (uint16_t)payload[2 * i] | (uint16_t)payload[2 * i + 1] << 8);
  }
  uint8_t result = writeMultipleRegisters(0x9013ul, 3);
  ESP_LOGI(TAG, "%s", result == ModbusMaster::ku8MBSuccess ? "Success!" : "Failure!");
}

LiveData readLiveData() {
  ESP_LOGI(TAG, "Reading live data");
  static uint8_t payload[2];
  LiveData stats;
  boolean success = true;
  success &= readInputRegisterRetry(LIVE_DATA | PANEL_VOLTS) == ModbusMaster::ku8MBSuccess;
  stats.panel_mV = getResponseBuffer(0) * 10;
  success &= readInputRegisterRetry(LIVE_DATA | PANEL_AMPS) == ModbusMaster::ku8MBSuccess;
  stats.panel_mA = getResponseBuffer(0) * 10;
  success &= readInputRegisterRetry(LIVE_DATA | BATT_VOLTS) == ModbusMaster::ku8MBSuccess;
  stats.battery_mV = getResponseBuffer(0) * 10;
  success &= readInputRegisterRetry(LIVE_DATA | BATT_AMPS) == ModbusMaster::ku8MBSuccess;
  stats.battery_mA = getResponseBuffer(0) * 10;
  success &= readInputRegisterRetry(LIVE_DATA | LOAD_VOLTS) == ModbusMaster::ku8MBSuccess;
  stats.load_mV = getResponseBuffer(0) * 10;
  success &= readInputRegisterRetry(LIVE_DATA | LOAD_AMPS) == ModbusMaster::ku8MBSuccess;
  stats.load_mA = getResponseBuffer(0) * 10;
  // ignore high byte - overwrite [13]
  success &= readInputRegisterRetry(LIVE_DATA | BATTERY_SOC) == ModbusMaster::ku8MBSuccess;
  stats.batterySoc = getResponseBuffer(0);

  uint8_t i = 0; uint8_t result;
  do { result = readInputRegisters(BATTERY_CURRENT_L, 2); } while (i++ < MAX_RETRY && result != ModbusMaster::ku8MBSuccess);
  success &= result == ModbusMaster::ku8MBSuccess;

  uint16_t data = getResponseBuffer(1) & 0x8000u;  // keep MSB
  data |= getResponseBuffer(0) & 0x7fffu; // add remaining lower bits
  payload[0] = lowByte(data);
  payload[1] = highByte(data);
  stats.batteryCurrent_mA = ((uint16_t)payload[0] + ((uint16_t)payload[1] << 8)) * 10;
  ESP_LOGI(TAG, "Panel: %2.2fV %2.2fA, Bat: %2.2fV %2.2fA, Load: %2.2fV %2.2fA, SoC %u%%, Bat current %2.2fA",
    stats.panel_mV / 1000.0, stats.panel_mA / 1000.0, stats.battery_mV / 1000.0, stats.battery_mA / 1000.0, stats.load_mV / 1000.0, stats.load_mA / 1000.0,
    stats.batterySoc, stats.batteryCurrent_mA / 1000.0);
  if (success) return stats;
  else
    return (LiveData) {
    .panel_mV = 0,
      .load_mV = 0,
      .battery_mV = 0,
      .panel_mA = 0,
      .load_mA = 0,
      .battery_mA = 0,
      .batterySoc = 0,
      .batteryCurrent_mA = 0
  };
}

StatsData readStatsData() {
  ESP_LOGI(TAG, "Reading stats data");
  static uint8_t payload[13];
  StatsData stats;
  boolean success = true;
  success &= readInputRegistersRetry(STATISTICS, GEN_ENERGY_DAY_L + 1) == ModbusMaster::ku8MBSuccess;
  stats.panel_max_mV = getResponseBuffer(PV_MAX) * 10;
  stats.panel_min_mV = getResponseBuffer(PV_MIN) * 10;
  stats.battery_max_mV = getResponseBuffer(BATT_MAX) * 10;
  stats.battery_min_mV = getResponseBuffer(BATT_MIN) * 10;
  stats.consumed_Wh = getResponseBuffer(CONS_ENERGY_DAY_L) * 10;
  stats.generated_Wh = getResponseBuffer(GEN_ENERGY_DAY_L) * 10;

  success &= readInputRegisterRetry(LIVE_DATA | HEATSINK_TEMP) == ModbusMaster::ku8MBSuccess;
  stats.heatsinkTemp_centiDeg = getResponseBuffer(0);
  success &= readInputRegistersRetry(STATUS_FLAGS, 3) == ModbusMaster::ku8MBSuccess;
  stats.statusBattery = getResponseBuffer(STATUS_BATTERY);
  stats.statusCharger = getResponseBuffer(STATUS_CHARGER);
  stats.statusDischarger = getResponseBuffer(STATUS_DISCHARGER);

  ESP_LOGI(TAG, "Panel max %2.2fV min %2.2fV, Bat max %2.2fV min %2.2fV, Today consumed %2.2fkWh generated %2.2fkWh, heatsink %2.2f°C",
    stats.panel_max_mV / 1000.0, stats.panel_min_mV / 1000.0,
    stats.battery_max_mV / 1000.0, stats.battery_min_mV / 1000.0,
    stats.consumed_Wh / 1000.0, stats.generated_Wh / 1000.0,
    stats.heatsinkTemp_centiDeg / 100.0);
  return stats;
} 