#include <SPI.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <FS.h>
#include <SD.h>

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 38
#define GREEN_BTN 37

#define TFT_RST 3

#define RTC_SDA 21
#define RTC_SCL 12

// SD CARD VARIABLES
#define REASSIGN_PINS
#define SCK 18
#define MISO 19
#define MOSI 11
#define CS 5

// MAX98357 I2S VARIABLES
#define I2S_BCLK 14
#define I2S_LRC  13
#define I2S_DOUT 10

// Audio audio;
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
unsigned int debounceDelayYellow = 50;
unsigned long lastYellowDebounce = millis();
byte yellowBtnState = LOW;

unsigned int debounceDelayWhite = 50;
unsigned long lastWhiteDebounce = millis();
byte whiteBtnState = LOW;

unsigned int debounceDelayRed = 50;
unsigned long lastRedDebounce = millis();
byte redBtnState = LOW;

unsigned int debounceDelayGreen = 50;
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

  formattedTime = dateArr[0] + " " + dateArr[1] + ", " + dateArr[2] + " — " + hoursArr[0] + ":" + hoursArr[1] + " " + hoursArr[2];

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

  // audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // audio.setVolume(18);

  // FORCE CS pins HIGH BEFORE SPI START
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  SPI.begin(SCK, MISO, MOSI);

  tft.init();
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);

  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);
  bmoGreen = tft.color565(150, 240, 186);

  Wire.begin(RTC_SDA, RTC_SCL);

  if (! rtc.begin()) {
    // Serial.println("Couldn't find RTC");
    while (1);
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // // Setup SD Card module
  // if (!SD.begin(CS, SPI, 4000000)) {
  //   // Serial.println("SD mount failed");
  //   while (1);
  // }
  // // else {
  // //   Serial.println("SD OK");
  // // }

  BMOidleFace();
  displayRTC();
  Serial.println("NEW CODE IS RUNNING");
}

void loop() {
  // put your main code here, to run repeatedly:

}
