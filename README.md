# Grandma-Jukebox
Jukebox to play songs for grandma and pill reminder.

> WARNING: Do **NOT** use GPIO19 or GPIO20 on this board. These GPIOs are internally connected to the USB-OTG and causes the USB no being detected by the computer. IF you accidentally upload a sketch and used those pins, you can upload an empty sketch using USB-UART and the communication with USB-OTG will work again.

## Materials
- ESP32-S3-DevKitC-1
- 3.2" 240x320 SPI FT Display
- DS3231 RTC
- 2 x MAX98357A I2S
- 2 x 3525 4ohm 3W-2.0port
- TF Micro SD Card Module
- 32GB Sandisk Ultra Micro SD card
- 4 x push buttons

## Pinouts with ESP32
> TFT and the SD card are sharing the same SPI buses (MOSI, SCK)
### SPI TFT Screen
| SPI TFT | ESP32 |
|---------|------------|
| `VCC` | `VIN` |
| `GND` | `GND` |
| `CS` | `GPIO15` |
| `RESET` | `GPIO8` |
| `DC` | `GPIO4` |
| `SDI (MOSI)` | `GPIO11` |
| `SCK` | `GPIO18` |
| `LED` | `3.3V` |

### SD CARD MODULE
| SD Card | ESP32 |
|---------|------------|
| `GND` | `GND` |
| `CLK` | `GPIO18` |
| `MISO` | `GPIO6` |
| `MOSI` | `GPIO11` |
| `CS` | `GPIO5` |
| `3.3V` | `3.3V` |

### DS3231 RTC
| DS3231 RTC | ESP32 |
|---------|------------|
| `GND` | `GND` |
| `VCC` | `VIN` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO12` |

### MAX98357 I2S
| MAX98357 I2S | ESP32 |
|---------|------------|
| `VIN` | `VIN` |
| `GND` | `GND` |
| `LRC` | `GPIO13` |
| `BCLK` | `GPIO14` |
| `DIN` | `GPIO10` |


### BUTTONS
| BUTTONS | ESP32 |
|---------|------------|
| `YELLOW` | `GPIO16` |
| `WHITE` | `GPIO17` |
| `RED` | `GPIO38` |
| `GREEN` | `GPIO9` |