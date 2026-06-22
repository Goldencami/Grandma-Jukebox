# Grandma-Jukebox
Jukebox to play songs for grandma and pill reminder.

> WARNING: Do **NOT** use GPIO19 or GPIO20 on this board. These GPIOs are internally connected to the USB-OTG and causes the USB no being detected by the computer. IF you accidentally upload a sketch and used those pins, you can upload an empty sketch using USB-UART and the communication with USB-OTG will work again.

> WARNING: Do NOT use GPIO2 for buttons or any other digital I/O. It is an ESP32-S3 strapping pin sampled at boot — depending on its state at power-on, it can prevent the board from booting correctly.

## Components
- ESP32-S3 WROOM: Dual-core 32-bit microprocessor up to 240 MHz, 16 MB Flash, 16 MB PSRAM
- 3.2" 240x320 SPI TFT Display — ST7789 driver chip (confirmed; not ILI9341 despite some markings/assumptions — see Display Driver section below)
- DS3231 RTC
- MAX98357A I2S
- 3525 4ohm 3W-2.0port
- TF Micro SD Card Module
- 32GB Sandisk Ultra Micro SD card
- 4 x push buttons

## Display Driver — IMPORTANT
This panel's actual controller is ST7789, not ILI9341. Use the `Adafruit_ST7789` library (via "Adafruit ST7735 and ST7789 Library" in Library Manager), not `TFT_eSPI` and not `Adafruit_ILI9341`.

**Why not TFT_eSPI**: `TFT_eSPI` crashes with a `Guru Meditation Error: StoreProhibited` (`EXCVADDR: 0x00000010`) immediately on `tft.init()`, on this exact hardware. This was confirmed reproducible across two different ESP32-S3 boards, multiple ESP32 Arduino core versions (3.2.x and 2.0.17), both ILI9341 and ST7789 driver settings, both RGB/BGR color order settings, and using the library's own unmodified stock example sketch. Root cause was never conclusively identified — suspected ESP-IDF 5.x SPI/DMA internals incompatibility — but it is reliably reproducible, so this library should be avoided for this project.

**Why not Adafruit_ILI9341**: Works partially (boots, draws, colors correctable via a manual R/B channel swap) but the addressable drawing window doesn't match the physical glass — roughly 1/5 of the right edge of the screen is not usable. This confirms the controller is not actually ILI9341.

Confirmed working setup (`Adafruit_ST7789`):
```cpp
#include <Adafruit_ST7789.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

tft.init(240, 320);        // native resolution, required
tft.invertDisplay(false);  // REQUIRED — this panel needs inversion off;
                            // library defaults to inverted (ST77XX_INVON),
                            // which shows colors wrong (e.g. white as black)
tft.setRotation(1);        // landscape: width=320, height=240
```

If you ever swap in a genuinely different physical panel, re-verify the driver chip and re-test `invertDisplay(true)` vs `false` and color order before assuming this config still applies.

## Enable PSRAM in the IDE
This project uses PSRAM to play music. 

In Arduino IDE:
- Tools → PSRAM and set it to "OPI PSRAM"
- Tools → Flash Size: "16 MB (128Mb)".
- Tools → Partition Scheme: "16M Flash (2MB APP/12.5MB FATFS)".

> These are the PSRAM, Flash size and Partition Scheme values for the ESP32 that is being used.

## Pinouts with ESP32
<img width="1637" height="727" alt="Image" src="https://github.com/user-attachments/assets/1d34ac77-a264-4828-980a-de5787600532" />

> TFT and the SD card are sharing the same SPI buses (MOSI, SCK)
### SPI TFT Screen
| SPI TFT | ESP32 |
|---------|------------|
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `CS` | `GPIO15` |
| `RESET` | `GPIO8` |
| `DC` | `GPIO4` |
| `SDI (MOSI)` | `GPIO11` |
| `SCK` | `GPIO12` |
| `LED` | `3.3V` |

### SD CARD MODULE
| SD Card | ESP32 |
|---------|------------|
| `GND` | `GND` |
| `CLK` | `GPIO39` |
| `MISO` | `GPIO40` |
| `MOSI` | `GPIO41` |
| `CS` | `GPIO10` |
| `3.3V` | `3.3V` |

### DS3231 RTC
| DS3231 RTC | ESP32 |
|---------|------------|
| `GND` | `GND` |
| `VCC` | `VIN` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO47` |

### MAX98357 I2S
| MAX98357 I2S | ESP32 |
|---------|------------|
| `VIN` | `VIN` |
| `GND` | `GND` |
| `LRC` | `GPIO13` |
| `BCLK` | `GPIO14` |
| `DIN` | `GPIO9` |


### BUTTONS
| BUTTONS | ESP32 |
|---------|------------|
| `YELLOW` | `GPIO16` |
| `WHITE` | `GPIO17` |
| `RED` | `GPIO38` |
| `GREEN` | `GPIO1` |