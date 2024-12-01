# ESP32 Solar monitor

For EPEver and compatible charge controllers using RS485 Modbus

## Documentation

### Schematic and PCB

[Schematic](docs/ESP32SolarMonitor.pdf) and [Gerber files](docs/ESP32SolarMonitor.gbr.zip).

The relevant pin mappings are set in the [config.h](include/config.h) file.

### Modbus protocol

[EPEver documentation](docs/ControllerProtocolV2.3.pdf)

## Configuration

The firmware is configured using the `config.json` file on the ESP32 file system. This can be changed via the web UI, or pre-flashed using the PlatformIO 'Upload Filesystem Image' target

Any properties in the config.json file that are ommitted will be set to their default values.

```
{
  "controllerNodeId": 1,
  "timezone": "NZST-12NZDT,M9.5.0,M4.1.0/3",
  "hasRelais": false,
  "relaisDuration": 3,
  "hasLora": false
}
```

| Setting                                                          | JSON property    | Default                     | Notes                                                                                         |
| ---------------------------------------------------------------- | ---------------- | --------------------------- | --------------------------------------------------------------------------------------------- |
| Node/device id of the controller, configurable in the controller | controllerNodeId | 1                           | Defaults to 1. Can be changed using the EPEver software                                       |
| Timezone                                                         | timezone         | NZST-12NZDT,M9.5.0,M4.1.0/3 | Posix TZ format                                                                               |
| Relais populated                                                 | hasRelais        | false                       | Is the on board relais populated? This can be used to briefly cut power to a connected device |
| Relais period (s)                                                | relaisDuration   | 3                           | How long to trigger the relais for                                                            |
| Lora modem populated                                             | hasLora          | false                       | Is the LoRa modem populated and should be used?                                               |
| Wifi enabled                                                     | wifiEnabled      | true                        |                                                                                               |

### Wifi

If no wifi credentials have been configured yet it will automatically launch an AP using the SSID `SolarMon-<ESP32mac>`. The device's Web UI can be accessed at [192.168.100.1](http://192.168.100.1/).

A password for the AP and the Web UI can be configured in the file `extra.ini` which needs to be created by copying [extra.template.ini](extra.template.ini) and applying the desired changes.
