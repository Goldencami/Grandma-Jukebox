#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Wire.h>
#include <RTClib.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include "esp_task_wdt.h"
#include "esp_log.h"
#include <Fonts/FreeSansBold12pt7b.h> 

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 38
#define GREEN_BTN 1

#define I2S_LRC  13
#define I2S_BCLK 14
#define I2S_DOUT 9

#define RTC_SDA 21
#define RTC_SCL 47

#define SD_SCK   39
#define SD_MISO  40
#define SD_MOSI  41
#define SD_CS    10

#define TFT_CS   15
#define TFT_DC    4
#define TFT_RST   8
#define TFT_MOSI 11
#define TFT_SCLK 12

// TIME VARIABLES
unsigned int debounceDelayYellow = 100;
unsigned long lastYellowDebounce = millis();
byte yellowBtnState = LOW;

unsigned int debounceDelayWhite = 100;
unsigned long lastWhiteDebounce = millis();
byte whiteBtnState = LOW;

unsigned int debounceDelayRed = 100;
unsigned long lastRedDebounce = millis();
byte redBtnState = LOW;

unsigned int debounceDelayGreen = 100;
unsigned long lastGreenDebounce = millis();
byte greenBtnState = LOW;

unsigned int RTCdelay = 500;
unsigned long lastRTCdelay = millis();

// MAX98357 I2S VARIABLES
String musicFiles[50];
int fileCount = 0;
int currentSong = 0;

// AUDIO OBJECTS (ESP8266Audio — runs on Core 0 via audioTask)
AudioGeneratorMP3 *mp3       = nullptr;
AudioFileSourceSD *audioSrc  = nullptr;
AudioOutputI2S    *audioOut  = nullptr;

// Shared state between loop() on Core 1 and audioTask on Core 0.
// Protected by audioMutex to avoid race conditions.
volatile bool audioShouldPlay = false;   // Core 1 sets true to start a song
volatile bool audioShouldStop = false;   // Core 1 sets true to stop mid-song
volatile bool audioIsPlaying  = false;   // Core 0 sets true while decoding
volatile bool audioPaused     = false;   // Core 1 toggles to pause/resume
volatile int  audioTrackIndex = 0;       // which track to play
SemaphoreHandle_t audioMutex;
TaskHandle_t audioTaskHandle = nullptr;

// RTC VARIABLES
RTC_DS3231 rtc;
String previousTime = "-1";
String dateArr[3]; // [month, day, year]
String hoursArr[3]; // [hour, minute, AM/PM]
String tempDateArr[3];
String tempHourArr[3];
bool isSetDateDone = false;
bool isSetHourDone = false;

int lastDateIdx = -1;
int lastTimeIdx = -1;

// Convert month number to Spanish month name
const char* months[] = {"", "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
int monthIdx = 0;

// SD CARD VARIABLES
SPIClass sdSPI(HSPI);   // plain object, not a pointer - no TFT in this sketch to order against

// TFT VARIABLES — using hardware SPI constructor so setSPISpeed() actually works.
// The simple Adafruit_ST7789(CS, DC, MOSI, SCLK, RST) constructor uses software
// SPI internally on ESP32, which is extremely slow and makes loop() unresponsive.
SPIClass tftSPI(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// STATE MACHINE VARIABLES
enum State {
  IDLE,
  SELECTION,
  PLAY,
  CONFIGS
};

State state = IDLE;

bool isIdleScreenDisplayed = false;
bool isMusicTypeDisplayed = false;
bool isConfigScreenDisplayed = false;

// BMO FUNCTIONS
uint16_t bmoGreen;

void drawSmile(int cx, int cy, int r) {
  for (int a = 20; a <= 160; a++) {
    float rad = a * DEG_TO_RAD;

    int x = cx + r * cos(rad);
    int y = cy + r * sin(rad);

    tft.fillCircle(x, y, 2, ST77XX_BLACK);
  }
}

void BMOidleFace() {
  int w = tft.width();
  int h = tft.height();

  // background
  tft.fillScreen(bmoGreen);
  previousTime = "-1";   // force RTC to redraw on next tick — fillScreen wiped it

  // eye settings
  int eyeR = 7;
  int eyeY = h * 0.35;

  tft.fillCircle(w * 0.3, eyeY, eyeR, ST77XX_BLACK);
  tft.fillCircle(w * 0.7, eyeY, eyeR, ST77XX_BLACK);

  // smile :)
  drawSmile(w / 2, h * 0.43, 40);
}

void drawArrow(int width, int y) {
  int x = 20 + width + 5;

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    ST77XX_BLACK
  );
}

void eraseArrow(int width, int y) {
  int x = 20 + width + 5; 

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    bmoGreen
  );
}

void drawDownArrow(int width, int y) {
  int x = 20 + width + 5;

  tft.fillTriangle(
    x,      y - 10,  // top-left of base
    x + 10, y - 10,  // top-right of base
    x + 5,  y,       // bottom tip
    ST77XX_BLACK
  );
}

void eraseDownArrow(int width, int y) {
  int x = 20 + width + 5;

  tft.fillTriangle(
    x,      y - 10,  // top-left of base
    x + 10, y - 10,  // top-right of base
    x + 5,  y,       // bottom tip
    bmoGreen
  );
}

void drawSong(String path) {
  int secondIndex = path.indexOf("/", path.indexOf("/") + 1);

  String songMP3 = path.substring(secondIndex + 1);
  String title = songMP3.substring(0, songMP3.length()-4);

  // convention title is: artistName - songName
  int dashIdx = title.indexOf("-");
  String artist = title.substring(0, dashIdx - 2);
  String song = title.substring(dashIdx + 2, title.length());

  tft.fillRect(0, 0, 320, 130, bmoGreen);
  tft.setTextColor(ST77XX_BLACK, bmoGreen);

  int16_t x1, y1;
  uint16_t w, h;

  // Center song title
  tft.getTextBounds(song, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 70);
  tft.print(song);

  // Center artist name
  tft.getTextBounds(artist, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 120);
  tft.print(artist);
}

void displayConfigs() {
  tft.fillScreen(bmoGreen);
  tft.setCursor(20, 40);
  tft.print("Configuraciones");

  DateTime now = rtc.now();
  getDate(tempDateArr, now);
  getHour(tempHourArr, now);
}

// MUSIC FUNCTIONS
int musicIdx = 0;
void displayMusicTypes() {
  tft.fillScreen(bmoGreen);
  previousTime = "-1";   // force RTC to redraw on next tick — fillScreen wiped it

  tft.setTextColor(ST77XX_BLACK);

  tft.setCursor(20, 40);
  tft.print("Reproductor de musica");

  tft.setCursor(20, 90);
  tft.print("Canciones");

  tft.setCursor(20, 120);
  tft.print("Himnos Biblicos");
}

void selectMusic() {
  if (handleWhiteBtn()) {
    musicIdx = (musicIdx + 1) % 2;
    // erase both first
    eraseArrow(135, 90);
    eraseArrow(200, 120);
  }

  if (musicIdx == 0) {
    drawArrow(135, 90);
  }
  else  {
    drawArrow(200, 120);
  }
}

void playMusic(int index) {
  String path = musicFiles[index];

  // Signal Core 0 audio task to play this track.
  // drawSong() runs here on Core 1 — completely separate from audio decoding.
  xSemaphoreTake(audioMutex, portMAX_DELAY);
  audioTrackIndex  = index;
  audioShouldStop  = true; // stop any currently playing track first
  audioShouldPlay  = true; // then start the new one
  xSemaphoreGive(audioMutex);

  previousTime = "-1";   // force RTC to redraw after drawSong updates the screen
  drawSong(path);      // update screen immediately, doesn't wait for audio
}

void stopMusic() {
  xSemaphoreTake(audioMutex, portMAX_DELAY);
  audioShouldStop = true;
  audioShouldPlay = false;
  xSemaphoreGive(audioMutex);
}

void pauseResumeMusic() {
  xSemaphoreTake(audioMutex, portMAX_DELAY);
  audioPaused = !audioPaused;
  xSemaphoreGive(audioMutex);
  // Serial.printf("Audio %s\n", audioPaused ? "paused" : "resumed");
}

// Called by audioTask on Core 0 when a track finishes naturally
void onTrackFinished() {
  currentSong++;
  if (currentSong >= fileCount) {
    currentSong = 0;
    state = IDLE;
  } else {
    playMusic(currentSong);
  }
}

void nextSong() {
  currentSong++;
  if (currentSong >= fileCount) {
    stopMusic();
    currentSong = 0;
    state = IDLE;
  } else {
    playMusic(currentSong);
  }
}

void previousSong() {
  currentSong--;
  if (currentSong < 0) {
    currentSong = 0;
  }
  playMusic(currentSong);
}

// Called automatically by the Audio library when a track finishes
void audio_eof_mp3(const char *info) {
  onTrackFinished();
}

// RTC FUNCTIONS
int dateIdx = 0;
void setDate(String *outputArray) {
  if (handleGreenBtn()) {
    if (dateIdx == 2) {
      int year = outputArray[2].toInt();
      int month = monthIdx;
      int day = outputArray[1].toInt();

      // safety clamps
      if (year < 2000) year = 2000;
      if (year > 2099) year = 2099;

      int maxDays = getMaxDays(month, year);
      if (day < 1) day = 1;
      if (day > maxDays) day = maxDays;

      DateTime now = rtc.now();
      rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), 0));

      dateArr[0] = outputArray[0];
      dateArr[1] = outputArray[1];
      dateArr[2] = outputArray[2];

      isSetDateDone = true;
      dateIdx = 0;

      tft.fillRect(20, 80, 250, 120, bmoGreen);
      return;
    }

    dateIdx++;
  }

  selectDate();

  if (handleWhiteBtn()) {
    tft.fillRect(20, 80, 250, 120, bmoGreen);

    if (dateIdx == 0) {
      monthIdx++;
      if (monthIdx > 12) monthIdx = 1;
      outputArray[0] = months[monthIdx];
    }
    else if (dateIdx == 1) {
      int year = outputArray[2].toInt();
      int tempDay = outputArray[1].toInt();

      tempDay++;
      int maxDays = getMaxDays(monthIdx, year);
      if (tempDay > maxDays) tempDay = 1;

      outputArray[1] = (tempDay < 10) ? "0" + String(tempDay) : String(tempDay);
    }
    else if (dateIdx == 2) {
      int tempYear = outputArray[2].toInt();
      tempYear++;
      if (tempYear > 2099) tempYear = 2000;
      outputArray[2] = String(tempYear);
    }
  }

  else if (handleYellowBtn()) {
    tft.fillRect(20, 80, 250, 120, bmoGreen);

    if (dateIdx == 0) {
      monthIdx--;
      if (monthIdx < 1) monthIdx = 12;
      outputArray[0] = months[monthIdx];
    }
    else if (dateIdx == 1) {
      int year = outputArray[2].toInt();
      int tempDay = outputArray[1].toInt();

      tempDay--;
      int maxDays = getMaxDays(monthIdx, year);
      if (tempDay < 1) tempDay = maxDays;

      outputArray[1] = (tempDay < 10) ? "0" + String(tempDay) : String(tempDay);
    }
    else if (dateIdx == 2) {
      int tempYear = outputArray[2].toInt();
      tempYear--;
      if (tempYear < 2000) tempYear = 2099;
      outputArray[2] = String(tempYear);
    }
  }

  String month = outputArray[0];

  int regionX = 40;
  int regionW = 100;
  int16_t x1, y1;
  uint16_t w, h;

  tft.getTextBounds(month, 0, 0, &x1, &y1, &w, &h);
  int textW = w;
  int x = regionX + (regionW - textW) / 2;

  tft.setCursor(x, 120);
  tft.print(month);

  tft.setCursor(170, 120);
  tft.print(outputArray[1]);

  tft.setCursor(210, 120);
  tft.print(outputArray[2]);
}

int getMaxDays(int month, int year) {
  if(month == 2) {
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }

  if(month == 4 || month == 6 || month == 9 || month == 11)
    return 30;

  return 31;
}

void selectDate() {
  if(dateIdx == 0) {
    eraseDownArrow(208, 90);
    drawDownArrow(60, 90);
  }
  else if(dateIdx == 1) {
    eraseDownArrow(60, 90);
    drawDownArrow(153, 90);
  }
  else if(dateIdx == 2) {
    eraseDownArrow(153, 90);
    drawDownArrow(208, 90);
  }
}

int timeIdx = 0;
void setHour(String *outputArray) {
  if (handleGreenBtn()) {
    if (timeIdx == 2) {
      int hour12 = outputArray[0].toInt();
      int minute = outputArray[1].toInt();
      String ampm = outputArray[2];

      if (minute < 0 || minute > 59) minute = 0;

      int hour24 = hour12;

      if (ampm == "PM" && hour12 != 12) hour24 += 12;
      if (ampm == "AM" && hour12 == 12) hour24 = 0;

      DateTime now = rtc.now();
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour24, minute, 0));

      hoursArr[0] = outputArray[0];
      hoursArr[1] = outputArray[1];
      hoursArr[2] = outputArray[2];

      isSetHourDone = true;
      timeIdx = 0;

      tft.fillRect(20, 80, 250, 120, bmoGreen);
      return;
    }

    timeIdx++;
  }

  selectHour();

  if (handleWhiteBtn()) {
    tft.fillRect(20, 80, 250, 120, bmoGreen);

    if (timeIdx == 0) {
      int tempHour = outputArray[0].toInt();
      tempHour = (tempHour % 12) + 1;
      outputArray[0] = (tempHour < 10) ? "0" + String(tempHour) : String(tempHour);
    }
    else if (timeIdx == 1) {
      int tempMin = outputArray[1].toInt();
      tempMin = (tempMin + 1) % 60;
      outputArray[1] = (tempMin < 10) ? "0" + String(tempMin) : String(tempMin);
    }
    else if (timeIdx == 2) {
      outputArray[2] = (outputArray[2] == "AM") ? "PM" : "AM";
    }
  }

  else if (handleYellowBtn()) {
    tft.fillRect(20, 80, 250, 120, bmoGreen);

    if (timeIdx == 0) {
      int tempHour = outputArray[0].toInt();
      tempHour--;
      if (tempHour <= 0) tempHour = 12;

      outputArray[0] = (tempHour < 10) ? "0" + String(tempHour) : String(tempHour);
    }
    else if (timeIdx == 1) {
      int tempMin = outputArray[1].toInt();
      tempMin--;
      if (tempMin < 0) tempMin = 59;

      outputArray[1] = (tempMin < 10) ? "0" + String(tempMin) : String(tempMin);
    }
    else if (timeIdx == 2) {
      outputArray[2] = (outputArray[2] == "AM") ? "PM" : "AM";
    }
  }

  tft.setCursor(110, 120);
  tft.print(outputArray[0] + " : " + outputArray[1] + " " + outputArray[2]);
}

void selectHour() {
  if(timeIdx == 0) {
    eraseDownArrow(175, 90);
    drawDownArrow(93, 90);
  }
  else if(timeIdx == 1) {
    eraseDownArrow(93, 90);
    drawDownArrow(139, 90);
  }
  else if(timeIdx == 2) {
    eraseDownArrow(139, 90);
    drawDownArrow(175, 90);
  }
}

void getDate(String *outputArray, DateTime now) {
  String yearStr = String(now.year());
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day());
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month());
  monthIdx = now.month();
  monthStr = months[monthIdx];

  // [month, day, year]
  outputArray[0] = monthStr;
  outputArray[1] = dayStr;
  outputArray[2] = yearStr;
}

void getHour(String *outputArray, DateTime now) {
  // 12-hour clock
  int hour24 = now.hour();
  int hour12 = hour24 % 12;

  if (hour12 == 0) {
    hour12 = 12;
  }

  // [hour, minute, AM/PM]
  outputArray[0] = (hour12 < 10 ? "0" : "") + String(hour12);
  outputArray[1] = (now.minute() < 10 ? "0" : "") + String(now.minute());
  outputArray[2] = (hour24 < 12) ? "AM" : "PM";
}

void displayRTC() {
  DateTime now = rtc.now();
  getDate(dateArr, now);
  getHour(hoursArr, now);
  String formattedTime = dateArr[0] + " " + dateArr[1] + ", " + dateArr[2] + " - " + hoursArr[0] + ":" + hoursArr[1] + " " + hoursArr[2];

  if (formattedTime != previousTime) {
    tft.fillRect(0, 200, 320, 40, bmoGreen);
    tft.setTextColor(ST77XX_BLACK, bmoGreen);

    // measure to center text
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(formattedTime, 0, 0, &x1, &y1, &w, &h);
    int centeredX = (320 - w) / 2;

    tft.setCursor(centeredX, 220);
    tft.print(formattedTime); 

    previousTime = formattedTime;
  }
}

// BUTTON FUNCTIONS
bool handleYellowBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastYellowDebounce > debounceDelayYellow) {
    byte newBtnState = digitalRead(YELLOW_BTN);

    if(newBtnState != yellowBtnState) {
      yellowBtnState = newBtnState;
      lastYellowDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(yellowBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleWhiteBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastWhiteDebounce > debounceDelayWhite) {
    byte newBtnState = digitalRead(WHITE_BTN);

    if(newBtnState != whiteBtnState) {
      whiteBtnState = newBtnState;
      lastWhiteDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(whiteBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleRedBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastRedDebounce > debounceDelayRed) {
    byte newBtnState = digitalRead(RED_BTN);

    if(newBtnState != redBtnState) {
      redBtnState = newBtnState;
      lastRedDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(redBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleGreenBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastGreenDebounce > debounceDelayGreen) {
    byte newBtnState = digitalRead(GREEN_BTN);

    if(newBtnState != greenBtnState) {
      greenBtnState = newBtnState;
      lastGreenDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(greenBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

// ============================================================
// Core 0 — Audio task: decodes MP3 and feeds I2S
// Never touches TFT — completely isolated from Core 1 UI work
// ============================================================
void audioTask(void* param) {
  audioOut = new AudioOutputI2S();
  audioOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audioOut->SetGain(0.5);

  for (;;) {
    // --- Check if we should start a new track ---
    xSemaphoreTake(audioMutex, portMAX_DELAY);
    bool shouldPlay = audioShouldPlay;
    bool shouldStop = audioShouldStop;
    int  trackIdx   = audioTrackIndex;
    if (shouldPlay)  audioShouldPlay = false;
    if (shouldStop)  audioShouldStop = false;
    xSemaphoreGive(audioMutex);

    // Stop currently playing track if requested
    if (shouldStop && mp3 && mp3->isRunning()) {
      mp3->stop();
      delete mp3;      mp3      = nullptr;
      delete audioSrc; audioSrc = nullptr;
      xSemaphoreTake(audioMutex, portMAX_DELAY);
      audioIsPlaying = false;
      xSemaphoreGive(audioMutex);
    }

    // Start new track
    if (shouldPlay && trackIdx < fileCount) {
      String path = musicFiles[trackIdx];
      audioSrc = new AudioFileSourceSD(path.c_str());
      mp3      = new AudioGeneratorMP3();
      mp3->begin(audioSrc, audioOut);
      xSemaphoreTake(audioMutex, portMAX_DELAY);
      audioIsPlaying = true;
      audioPaused    = false;   // always start new track unpaused
      xSemaphoreGive(audioMutex);
      // Serial.printf("Now playing: %s\n", path.c_str());
    }

    // Run one decode loop iteration if playing
    if (mp3 && mp3->isRunning()) {
      xSemaphoreTake(audioMutex, portMAX_DELAY);
      bool paused = audioPaused;
      xSemaphoreGive(audioMutex);

      // When paused: keep decoding but at gain=0 so DMA buffers stay filled
      // with silence. This prevents the "drain glitch" where stale loud frames
      // play out because decode stopped but DMA kept pushing existing data.
      // When resuming: gain is restored before the next decode call.
      audioOut->SetGain(paused ? 0.0 : 0.5);

      if (!mp3->loop()) {
        // Track finished naturally
        mp3->stop();
        delete mp3;      mp3      = nullptr;
        delete audioSrc; audioSrc = nullptr;
        xSemaphoreTake(audioMutex, portMAX_DELAY);
        audioIsPlaying = false;
        xSemaphoreGive(audioMutex);
        onTrackFinished();
      }
    } else {
      // Nothing playing — yield so Core 1 isn't starved
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void loadMusicFiles(String folder) {
  fileCount = 0;

  File dir = SD.open(folder);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("Folder not found: %s\n", folder.c_str());
    return;
  }

  File entry = dir.openNextFile();
  while (entry && fileCount < 50) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
        musicFiles[fileCount++] = folder + "/" + name;
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  // Serial.printf("Loaded %d tracks from %s\n", fileCount, folder.c_str());
}

void setup() {
  Serial.begin(115200);

  pinMode(YELLOW_BTN, INPUT_PULLUP);
  pinMode(WHITE_BTN, INPUT_PULLUP);
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(GREEN_BTN, INPUT_PULLUP);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI, 20000000)) {
    Serial.println("SD Card Mount Failed!");
    return;
  }
  Serial.println("SD Card mounted successfully");

  bmoGreen = tft.color565(150, 240, 186);

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // begin hardware SPI bus
  tft.init(240, 320);
  tft.setSPISpeed(40000000);  // 40MHz — hardware SPI so this actually takes effect
  tft.invertDisplay(false);  // counteract library's default ST77XX_INVON
  tft.setRotation(1);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);

  Wire.begin(RTC_SDA, RTC_SCL);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  else {
    // Only set the time if the RTC lost power (no battery / first boot).
    // Otherwise leave its own running clock alone.
    if(rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  BMOidleFace();
  displayRTC();

  // ---- Reconfigure watchdog BEFORE launching audio task ----
  // mp3->loop() runs continuously on Core 0 and will starve IDLE0.
  // Reconfigure TWDT to not monitor Core 0's IDLE task, and disable panic.
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms      = 30000,    // 30s timeout — never fires during decode
    .idle_core_mask  = (1 << 1), // watch Core 1 IDLE only, not Core 0
    .trigger_panic   = false     // warn only, never abort
  };
  esp_task_wdt_reconfigure(&wdt_config);

  // ---- Create mutex and launch audio task on Core 0 ----
  audioMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 8192, NULL, 2, &audioTaskHandle, 0);
}

void loop() {
  static unsigned long redHoldStart = 0;

  if (digitalRead(RED_BTN) == LOW) {
    if (redHoldStart == 0) redHoldStart = millis();

    if (millis() - redHoldStart > 3000) {
      isSetDateDone = false;
      isSetHourDone = false;
      state = CONFIGS;
    }
  } else {
    redHoldStart = 0;
  }

  unsigned long timeNow = millis();
  if((timeNow - lastRTCdelay > RTCdelay) && state != CONFIGS) {
    lastRTCdelay = timeNow;
    displayRTC();
  }

  if(state == IDLE) {
    if(!isIdleScreenDisplayed) {
      BMOidleFace(); // also resets previousTime so RTC redraws immediately
      isIdleScreenDisplayed = true;
    }

    if(handleYellowBtn()) {
      state = SELECTION;
    }
  }
  else if(state == SELECTION) {
    isIdleScreenDisplayed = false;

    if(!isMusicTypeDisplayed) {
      displayMusicTypes();
      isMusicTypeDisplayed = true;
    }

    selectMusic();

    if(handleRedBtn()) {
      musicIdx = 0;
      isMusicTypeDisplayed = false;
      state = IDLE;
    }

    // GREEN confirms selection — load the chosen folder and start playing
    if(handleGreenBtn()) {
      String folder = (musicIdx == 0) ? "/canciones" : "/himnos";
      loadMusicFiles(folder);

      if(fileCount > 0) {
        currentSong = 0;
        state = PLAY;
        playMusic(currentSong);   // drawSong() runs here on Core 1. Actual MP3 decode runs on Core 0
      } else {
        Serial.println("No MP3 files found in folder");
      }
    }
  }
  else if(state == PLAY) {
    isIdleScreenDisplayed = false;
    
    // Button handling during playback
    if(handleGreenBtn()) pauseResumeMusic(); // PLAY/PAUSE toggle
    if(handleWhiteBtn()) nextSong();
    if(handleYellowBtn()) previousSong();

    if(handleRedBtn()) {
      stopMusic();
      musicIdx = 0;
      isMusicTypeDisplayed = false;
      state = IDLE;
    }
  }
  else if(state == CONFIGS) {
    isIdleScreenDisplayed = false;

    if(!isConfigScreenDisplayed) {
      displayConfigs();
      isConfigScreenDisplayed = true;
    }

    if(handleRedBtn()) {
      dateIdx = 0;
      timeIdx = 0;
      lastDateIdx = -1;
      lastTimeIdx = -1;

      isSetDateDone = false;
      isSetHourDone = false;
      isConfigScreenDisplayed = false;

      state = IDLE;
    }


    if(!isSetDateDone && !isSetHourDone) {
      setDate(tempDateArr);
    }
    else if(isSetDateDone && !isSetHourDone) {
      setHour(tempHourArr);
    }

    if(isSetDateDone && isSetHourDone) {
      isSetDateDone = false;
      isSetHourDone = false;

      dateIdx = 0;
      timeIdx = 0;
      lastDateIdx = -1;
      lastTimeIdx = -1;

      isConfigScreenDisplayed = false;
      state = IDLE;
    }
  }
}