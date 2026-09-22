# Grandma-Jukebox
Jukebox to play songs for grandma.

The micro SD card (32GB) contains two folders named `canciones` and `himnos` in which their corresponding mp3 files are located.

> WARNING: Do **NOT** use GPIO19 or GPIO20 on this board. These GPIOs are internally connected to the USB-OTG and causes the USB no being detected by the computer. IF you accidentally upload a sketch and used those pins, you can upload an empty sketch using USB-UART and the communication with USB-OTG will work again.

> WARNING: Do NOT use GPIO2 for buttons or any other digital I/O. It is an ESP32-S3 strapping pin sampled at boot — depending on its state at power-on, it can prevent the board from booting correctly.

In development, use the USB-UART to upload code. The final version will use the charging cable to connect to USB-OTG.

https://github.com/user-attachments/assets/e78aba78-86dd-4827-824b-220895683b4c

## Commands
### Idle Screen (BMO Face)
- **Yellow Button**:  Go to the Music Type Selection screen.
- **Red Button**: Hold for 3 seconds to go to the Configuration screen.

### Music Type Selection Screen
*(selecting between "Canciones" and "Himnos Biblicos")*
- **White Button**: Toggles between the two playlist options.
- **Red Button**: Goes back to Idle Screen.
- **Green Button**: CONFIRMS — loads and plays the selected playlist, then goes to the Now Playing screen.

### Now Playing Screen
- **Green Button**: Plays/pauses the current song.
- **White Button**: Plays next song.
- **Yellow Button**: Plays previous song.
- **Red Button**: Stops the music and goes back to Idle Screen.

### Configuration Screen
*(setting date, then time — one field at a time: month → day → year, then hour → minute → AM/PM)*
- **White Button**: Increases the value of the currently selected field.
- **Yellow Button**: Decreases the value of the currently selected field.
- **Green Button**: Confirms the current field and moves to the next one. On the last field of date/time, it also saves the value and advances to the next step (date → time → back to Idle).
- **Red Button**: Cancels and goes back to Idle Screen.

## Components
- ESP32-S3 WROOM: Dual-core 32-bit microprocessor up to 240 MHz, 16 MB Flash, 16 MB PSRAM
- 3.2" 240x320 SPI TFT Display — ST7789 driver chip (confirmed; not ILI9341 despite some markings/assumptions — see Display Driver section below)
- DS3231 RTC
- MAX98357A I2S
- 3525 4ohm 3W-2.0port
- TF Micro SD Card Module with an onboard 3.3V voltage regulator circuit
- 32GB Sandisk Ultra Micro SD card
- 4 x push buttons

## Display Driver — IMPORTANT
This panel's actual controller is ST7789, not ILI9341. Use the `Adafruit_ST7789` library (via "Adafruit ST7735 and ST7789 Library" in Library Manager), not `TFT_eSPI` and not `Adafruit_ILI9341`.

**Why not TFT_eSPI**: `TFT_eSPI` crashes with a `Guru Meditation Error: StoreProhibited` (`EXCVADDR: 0x00000010`) immediately on `tft.init()`, on this exact hardware. This was confirmed reproducible across two different ESP32-S3 boards, multiple ESP32 Arduino core versions (3.2.x and 2.0.17), both ILI9341 and ST7789 driver settings, both RGB/BGR color order settings, and using the library's own unmodified stock example sketch. Root cause was never conclusively identified — suspected ESP-IDF 5.x SPI/DMA internals incompatibility — but it is reliably reproducible, so this library should be avoided for this project.

**Why not Adafruit_ILI9341**: Works partially (boots, draws, colors correctable via a manual R/B channel swap) but the addressable drawing window doesn't match the physical glass — roughly 1/5 of the right edge of the screen is not usable. This confirms the controller is not actually ILI9341.

**Use hardware SPI, not the simple constructor**: The plain `Adafruit_ST7789(CS, DC, MOSI, SCLK, RST)` constructor falls back to software SPI on ESP32, which is slow enough to make `loop()` unresponsive. Use the hardware-SPI constructor (an explicit `SPIClass` bound to a free SPI bus) instead, and call `setSPISpeed()` so the higher clock actually takes effect.

Confirmed working setup (`Adafruit_ST7789`):
```cpp
#include <Adafruit_ST7789.h>

SPIClass tftSPI(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); // hardware SPI bus, no MISO needed
tft.init(240, 320);         // native resolution, required
tft.setSPISpeed(20000000);  // only takes effect with hardware SPI
tft.invertDisplay(false);   // REQUIRED — this panel needs inversion off;
                             // library defaults to inverted (ST77XX_INVON),
                             // which shows colors wrong (e.g. white as black)
tft.setRotation(1);         // landscape: width=320, height=240
```

If you ever swap in a genuinely different physical panel, re-verify the driver chip and re-test `invertDisplay(true)` vs `false` and color order before assuming this config still applies.

## Enable PSRAM in the IDE
This project enables PSRAM.

In Arduino IDE:
- Tools → PSRAM and set it to "OPI PSRAM"
- Tools → Flash Size: "16 MB (128Mb)".
- Tools → Partition Scheme: "16M Flash (2MB APP/12.5MB FATFS)".

> These are the PSRAM, Flash size and Partition Scheme values for the ESP32 that is being used.
>
> TODO: confirm and document *why* PSRAM is required (e.g. instability/crash without it) — the sketch doesn't call `ps_malloc`/`ps_calloc` directly, so if this was needed for a specific crash or buffer, note it here for future reference.

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
| `VCC` | `VIN` |
| `MISO` | `GPIO40` |
| `MOSI` | `GPIO41` |
| `SLK` | `GPIO39` |
| `CS` | `GPIO10` |

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


<img width="3300" height="2550" alt="Image" src="https://github.com/user-attachments/assets/f13d9ee9-ba31-480d-9429-1f7ee259034a" />

## Credits
The arms and legs are from the [BMO model](https://www.printables.com/model/1582055-bmo-from-adventure-time-local-ai-agent-project) by brenpoly, licensed under [CC BY-NC 4.0](http://creativecommons.org/licenses/by-nc/4.0/). They were scaled to 1.5x to fit BMO's body. No other parts of that model were used.
