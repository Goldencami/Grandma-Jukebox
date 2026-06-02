#include <SPI.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <FS.h>
#include <SD.h>

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 27
#define GREEN_BTN 32

// SD CARD VARIABLES
#define REASSIGN_PINS
#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 5

// TFT VARIABLE
TFT_eSPI tft = TFT_eSPI(320, 240);

// RTC VARIABLE
RTC_DS3231 rtc;
String formattedTime = "";
String previousTime = "";

// Convert month number to Spanish month name
const char* months[] = {"", "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};

// STATE MACHINE VARIABLES
enum State {
  IDLE,
  MUSIC,
  SELECTION,
  PLAY,
  AUDIO
};

State state = IDLE;
State currentScreen;

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

int idx = 0;
void displayMusicTypes() {
  tft.fillScreen(bmoGreen);

  tft.setTextColor(TFT_BLACK);

  tft.setCursor(20, 40);
  tft.print("Reproductor de musica");

  tft.setCursor(20, 90);
  tft.print("Canciones");

  tft.setCursor(20, 120);
  tft.print("Himnos Biblicos");

  state = SELECTION;
}

void selectMusic() {
  if(handleWhiteBtn()) {
    idx = (idx + 1) % 2;
  }

  // Arrow pointing at index 0 (songs)
  if(idx == 0) {
    eraseArrow(200, 120);
    drawArrow(135, 90);
  }
  else if(idx == 1) { // Arrow pointing at index 1 (hymns)
    eraseArrow(135, 90);
    drawArrow(200, 120);
  }
}

void drawArrow(int width, int y) {
  int x = 20 + width + 5;
  // tft.fillTriangle(30, 40, 50, 30, 50, 50, TFT_BLACK);

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

// RTC FUNCTIONS
void displayRTC() {
  // Get the current time from the RTC
  DateTime now = rtc.now();
    
  // Getting each time field in individual variables
  // And adding a leading zero when needed;
  String yearStr = String(now.year());
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day());
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month());
  monthStr = months[now.month()];

  // 12-hour clock
  int hour24 = now.hour();
  int hour12 = hour24 % 12;

  if (hour12 == 0) {
    hour12 = 12;
  }

  String hourStr = (hour12 < 10 ? "0" : "") + String(hour12);
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute());

  String clockPeriod = (hour24 < 12) ? "AM" : "PM";

  // Complete time string
  formattedTime = monthStr + " " + dayStr + ", " + yearStr + "  —  " + hourStr + ":" + minuteStr + " " + clockPeriod;

  if (formattedTime != previousTime) {
    // Erase previous time
    tft.fillRect(0, 200, 320, 40, bmoGreen);
    previousTime = formattedTime;
  }

  tft.setTextColor(TFT_BLACK);
  tft.setCursor(12, 220);
  tft.print(formattedTime);
}

// BUTTON FUNCTIONS
void handleYellowBtn() {
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
        state = MUSIC;
      }
    }
  }
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

void handleRedBtn() {
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
        idx = 0;
        state = IDLE;
      }
    }
  }
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

  Serial.println("HELP");

  pinMode(YELLOW_BTN, INPUT_PULLUP);
  pinMode(WHITE_BTN, INPUT_PULLUP);
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(GREEN_BTN, INPUT_PULLUP);

  // FORCE CS pins HIGH BEFORE SPI START
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  SPI.begin(SCK, MISO, MOSI);

  tft.init();
  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);
  bmoGreen = tft.color565(150, 240, 186);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // Setup SD Card module
  if (!SD.begin(5, SPI, 4000000)) {
    Serial.println("SD mount failed");
    while (1);
  }
  else {
    Serial.println("SD OK");
  }

  BMOidleFace();
  displayRTC();
}

void loop() {
  handleYellowBtn();
  handleRedBtn();

  unsigned long timeNow = millis();
  if(timeNow - lastRTCdelay > RTCdelay) {
    lastRTCdelay = timeNow;
    displayRTC();
  }

  if(state != currentScreen || state == SELECTION) {
    currentScreen = state;

    if(state == IDLE) {
      BMOidleFace();
    }
    else if(state == MUSIC) {
      displayMusicTypes();
    }
    else if(state == SELECTION) {
      selectMusic();

      if(handleGreenBtn()) {
        if(idx == 0) {
          File root = SD.open("/canciones/");
          if (root) {
            readDirectory(root, 0);
          }
        }
        else if(idx == 1) {
          File root = SD.open("/himnos/");
          if (root) {
            readDirectory(root, 0);
          }
        }
      }
    }
  }
}