#include <radio.h>
#include <SPI.h>

// Local logging tag
static const char TAG[] = "Radio";

SX1276* radio;
LoRaWANNode* loraNode;

void checkRadioCmd(int state, const char* op) {
  if (state == RADIOLIB_ERR_NONE) return;
  if (state == RADIOLIB_ERR_SPI_CMD_TIMEOUT) ESP_LOGE(TAG, " %s failed: SPI timeout", op);
  else if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) ESP_LOGE(TAG, " %s failed: Chip not found", op);
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) ESP_LOGE(TAG, " %s failed: Rx timeout", op);
  else if (state == RADIOLIB_ERR_CRC_MISMATCH) ESP_LOGE(TAG, " %s failed: CRC mismatch", op);
  else if (state == RADIOLIB_ERR_WRONG_MODEM) ESP_LOGE(TAG, " %s failed: Wrong modem ?!?", op);
  else if (state != RADIOLIB_ERR_NONE) ESP_LOGE(TAG, " %s failed: code %i", op, state);
}

int16_t checkNodeCmd(int16_t state, const char* op) {
  if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_LORAWAN_NEW_SESSION) return state;
  else if (state == RADIOLIB_ERR_NO_JOIN_ACCEPT) ESP_LOGE(TAG, "%s: no join accept", op);
  else if (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED) ESP_LOGE(TAG, "%s: LoRa header damaged", op);
  else if (state != RADIOLIB_ERR_NONE) ESP_LOGE(TAG, "%s: code %i", op, state);
  return state;
}

const uint64_t APP_EUI = 0x0000000000000000;
const uint64_t DEV_EUI = 0x70B3D57ED006B0AF;
uint8_t APP_KEY[] = { 0xDE, 0x3E, 0x19, 0x2D, 0x74, 0xA4, 0x46, 0xF8, 0x4D, 0x5A, 0xB9, 0xA6, 0x44, 0x4B, 0xD9, 0x4C };
uint8_t NWKS_KEY[] = { 0xEA, 0x47, 0x07, 0xF0, 0x4A, 0x58, 0x2C, 0x62, 0x0F, 0x84, 0x2E, 0x13, 0x21, 0x62, 0x22, 0x89 };

void initRadio() {
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, -1);

  radio = new SX1276(new Module((uint32_t)LORA_CS, LORA_DIO0, LORA_RESET, LORA_DIO1, SPI));

  // regional choices: EU868, US915, AU915, AS923, IN865, KR920, CN780, CN500
  const LoRaWANBand_t region = AU915;

  // sub-band: 0? 2?
  loraNode = new LoRaWANNode(radio, &region, 2);

  checkRadioCmd(radio->begin(), "begin");

  // Setup the OTAA session information
  loraNode->beginOTAA(APP_EUI, DEV_EUI, NWKS_KEY, APP_KEY);

  if (checkNodeCmd(loraNode->activateOTAA(), "activateOTAA") == RADIOLIB_LORAWAN_NEW_SESSION) {
    ESP_LOGI(TAG, "Join successful!");

    // Print the DevAddr
    ESP_LOGI(TAG, "DevAddr: %x", (unsigned long)loraNode->getDevAddr());

    // Disable the ADR algorithm (on by default which is preferable)
    loraNode->setADR(true);

    // Set a fixed datarate
  //  loraNode->setDatarate(4);

    // Manages uplink intervals to the TTN Fair Use Policy
    loraNode->setDutyCycle(false);

    // Enable the dwell time limits - 400ms is the limit for the US
  //  loraNode->setDwellTime(true, 400);
  }
  ESP_LOGI(TAG, "Ready!");

}
