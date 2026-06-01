#include <SPI.h>
#include <TFT_eSPI.h>
#include <RTClib.h>

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 5
#define GREEN_BTN 19

TFT_eSPI tft = TFT_eSPI(320, 240);

RTC_DS3231 rtc;
String formattedTime = "";
String previousTime = "";

// Convert month number to Spanish month name
const char* months[] = {"", "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};

enum State {
  IDLE,
  MUSIC,
  SELECTION
};

State state = IDLE;
State currentScreen;

// BMO COLOURS
uint16_t bmoGreen = tft.color565(150, 240, 186);

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
  tft.setCursor(15, 220);
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

      if(yellowBtnState == HIGH) {
        Serial.println("Yellow button pressed");
        state = MUSIC;
      }
      else {
        Serial.println("Yellow button released");
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

      if(whiteBtnState == HIGH) {
        Serial.println("White button pressed");
        return true;
      }
      else {
        Serial.println("White button released");
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

      if(redBtnState == HIGH) {
        Serial.println("Red button pressed");
        idx = 0;
        state = IDLE;
      }
      else {
        Serial.println("Red button released");
      }
    }
  }
}

void handleGreenBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastGreenDebounce > debounceDelayGreen) {
    byte newBtnState = digitalRead(GREEN_BTN);

    if(newBtnState != greenBtnState) {
      greenBtnState = newBtnState;
      lastGreenDebounce = timeNow;

      if(greenBtnState == HIGH) {
        Serial.println("Green button pressed");
      }
      else {
        Serial.println("Green button released");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(YELLOW_BTN, INPUT);
  pinMode(WHITE_BTN, INPUT);
  pinMode(RED_BTN, INPUT);
  pinMode(GREEN_BTN, INPUT);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
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
  
  tft.init();
  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);

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
    }
  }
}