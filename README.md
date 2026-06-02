# Grandma-Jukebox
Jukebox to play songs for grandma and pill reminder.

## Pinouts with ESP32
> TFT and the SD card are sharing the same SPI buses (MOSI, SCK)
### SPI TFT Screen
| SPI TFT | ESP32 |
|---------|------------|
| `VCC` | `VIN` |
| `GND` | `GND` |
| `CS` | `GPIO15` |
| `RESET` | `GPIO26` |
| `DC` | `GPIO25` |
| `SDI (MOSI)` | `GPIO23` |
| `SCK` | `GPIO18` |
| `LED` | `3.3V` |

### SD CARD MODULE
| SD Card | ESP32 |
| `GND` | `GND` |
| `CLK` | `GPIO18` |
| `MISO` | `GPIO19` |
| `MOSI` | `GPIO23` |
| `CS` | `GPIO5` |
| `3.3V` | `3.3V` |

### DS3231 RTC
| DS3231 RTC | ESP32 |
| `GND` | `GND` |
| `VCC` | `VIN` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO22` |


### BUTTONS
| BUTTONS | ESP32 |
| `YELLOW` | `GPIO16` |
| `WHITE` | `GPIO17` |
| `RED` | `GPIO27` |
| `GREEN` | `GPIO32` |