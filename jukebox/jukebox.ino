#include <SPI.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <FS.h>
#include <SD.h>
#include <Audio.h>

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 38
#define GREEN_BTN 9

#define TFT_RST 8
#define TFT_CS 15

#define RTC_SDA 21
#define RTC_SCL 12

// SD CARD VARIABLES
#define REASSIGN_PINS
#define SCK 18
#define MISO 6
#define MOSI 11
#define CS 5

// MAX98357 I2S VARIABLES
#define I2S_BCLK 14
#define I2S_LRC  13
#define I2S_DOUT 10

Audio audio;
String musicFiles[50];
String musicFolder = "";
int fileCount = 0;
int currentSong = 0;

// TFT VARIABLE
TFT_eSPI tft = TFT_eSPI(320, 240);

// RTC VARIABLE
RTC_DS3231 rtc;
String formattedTime = "";
String previousTime = "";
String dateArr[3]; // [month, day, year]
String hoursArr[3]; // [hour, minute, AM/PM]
String tempDateArr[3];
String tempHourArr[3];
bool isSetDateDone = false;
bool isSetHourDone = false;

int lastSec = -1;
int lastMin = -1;
int lastHour = -1;
int lastDay = -1;
int lastMonth = -1;
int lastYear = -1;

char timeBuffer[20];
char dateBuffer[30];

// Convert month number to Spanish month name
const char* months[] = {"", "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
int monthIdx = 0;

// STATE MACHINE VARIABLES
enum State {
  IDLE,
  SELECTION,
  PLAY,
  AUDIO,
  CONFIGS
};

State state = IDLE;

bool isIdleScreenDisplayed = false;
bool isMusicTypeDisplayed = false;
bool isConfigScreenDisplayed = false;

bool started = false;
int lastMusicIdx = -1;
int lastDateIdx = -1;
int lastTimeIdx = -1;

// BMO COLOURS
uint16_t bmoGreen;

// RTC DELAY
unsigned int RTCdelay = 500;
unsigned long lastRTCdelay = millis();

// BUTTON STATES
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

// BMO FUNCTIONS
void drawSmile(int cx, int cy, int r) {
  for (int a = 20; a <= 160; a++) {
    float rad = a * DEG_TO_RAD;

    int x = cx + r * cos(rad);
    int y = cy + r * sin(rad);

    tft.fillCircle(x, y, 2, TFT_BLACK);
  }
}

void BMOidleFace() {
  int w = tft.width();
  int h = tft.height();

  // background
  tft.fillScreen(bmoGreen);

  // eye settings
  int eyeR = 7;
  int eyeY = h * 0.35;

  tft.fillCircle(w * 0.3, eyeY, eyeR, TFT_BLACK);
  tft.fillCircle(w * 0.7, eyeY, eyeR, TFT_BLACK);

  // smile :)
  drawSmile(w / 2, h * 0.43, 40);
}

void drawArrow(int width, int y) {
  int x = 20 + width + 5;

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    TFT_BLACK
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
    TFT_BLACK
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

int musicIdx = 0;
void displayMusicTypes() {
  tft.fillScreen(bmoGreen);

  tft.setTextColor(TFT_BLACK);

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
  }

  if (musicIdx == lastMusicIdx) return; // no redraw and avoids glitching screen

  // erase both first
  eraseArrow(135, 90);
  eraseArrow(200, 120);

  if (musicIdx == 0) drawArrow(135, 90);
  else drawArrow(200, 120);

  lastMusicIdx = musicIdx;
}

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
  int textW = tft.textWidth(month);
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

void playMusic(int index) {
  audio.stopSong();
  audio.connecttoFS(SD, "/himnos/Carlos Vives - La Gota Fria.wav");
}

void nextSong() {
  currentSong++;

  if(currentSong >= fileCount) {
    currentSong = 0;
  }

  playMusic(currentSong);
}

void previousSong() {
  currentSong--;

  if(currentSong < 0) {
    currentSong = fileCount - 1;
  }
  
  playMusic(currentSong);
}

// SD CARD FUNCTIONS
void readDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println("/");
      readDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  dir.close();
}

void loadMusicList(File dir) {
  fileCount = 0;

  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    if (!entry.isDirectory()) {
      String name = entry.name();

      if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
        musicFiles[fileCount++] = musicFolder + name;
      }
    }

    entry.close();
  }
}

// RTC FUNCTIONS
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

  formattedTime = dateArr[0] + " " + dateArr[1] + ", " + dateArr[2] + " - " + hoursArr[0] + ":" + hoursArr[1] + " " + hoursArr[2];

  if (formattedTime != previousTime) {
    tft.fillRect(0, 200, 320, 40, bmoGreen); // full clear band
    tft.setTextColor(TFT_BLACK, bmoGreen);
    tft.setCursor(12, 220);
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

void setup() {
  Serial.begin(115200);

  pinMode(YELLOW_BTN, INPUT_PULLUP);
  pinMode(WHITE_BTN, INPUT_PULLUP);
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(GREEN_BTN, INPUT_PULLUP);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18);

  // FORCE CS pins HIGH BEFORE SPI START
  pinMode(TFT_CS, OUTPUT);
  pinMode(CS, OUTPUT); // SD
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(CS, HIGH);

  Wire.begin(RTC_SDA, RTC_SCL);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  tft.init();
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);
  bmoGreen = tft.color565(150, 240, 186);

  // Setup SD Card module
  if (!SD.begin(CS, tft.getSPIinstance(), 4000000)) {
    Serial.println("SD mount failed");
  }

  displayRTC();
  BMOidleFace();
  Serial.println("NEW CODE IS RUNNING");
}

void loop() {
  // MUST run on every iteration: feeds the I2S DMA buffer and reads
  // the next chunk of audio data from SD. Throttling this call (even
  // to a few times per second) starves audio output almost completely.
  audio.loop();

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
      BMOidleFace();
      isIdleScreenDisplayed = true;
    }

    if(handleYellowBtn()) {
      state = SELECTION;
    }
  }
  else {
    isIdleScreenDisplayed = false;

    if(state == SELECTION) {
      if(!isMusicTypeDisplayed) {
        displayMusicTypes();
        isMusicTypeDisplayed = true;
      }

      selectMusic();

      if(handleRedBtn()) {
        musicIdx = 0;
        isMusicTypeDisplayed = false;
        state = IDLE;
        started = false;
      }

      if(handleGreenBtn()) {
        if(musicIdx == 0) {
          musicFolder = "/canciones/";
        }
        else if(musicIdx == 1) {
          musicFolder = "/himnos/";
        }

        Serial.println("green pressed!");
        state = AUDIO;
      }
    }
    else if (state == AUDIO && !started) {
      Serial.println("Opening root");
      File root = SD.open(musicFolder);

      if (root) {
        Serial.println("ROOT OPENED OK");
        loadMusicList(root);
        root.close();
      }

      musicFolder = "";
      currentSong = 0;
      playMusic(currentSong);

      started = true;
      state = PLAY;
    }
    else if(state == PLAY) {
      if(handleGreenBtn()) {
        audio.pauseResume();   // PLAY/PAUSE
      }

      // if(handleWhiteBtn()) {
      //   nextSong();
      // }

      // if(handleYellowBtn()) {
      //   previousSong();
      // }

      if(handleRedBtn()) {
        musicIdx = 0;
        isMusicTypeDisplayed = false;
        state = IDLE;
        started = false;
      }
    }
    else if(state == CONFIGS) {
      if(!isConfigScreenDisplayed) {
        tft.fillScreen(bmoGreen);
        tft.setCursor(20, 40);
        tft.print("Configuraciones");

        DateTime now = rtc.now();
        getDate(tempDateArr, now);
        getHour(tempHourArr, now);

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
        started = false;
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
}