#include <SPI.h>
#include <TFT_eSPI.h>
// #include <Fonts/FreeSans12pt7b.h>

#define YELLOW_BTN 25
#define WHITE_BTN 26
#define RED_BTN 27
#define GREEN_BTN 14

TFT_eSPI tft = TFT_eSPI(320, 240);

enum State {
  IDLE,
  MUSIC,
  SELECTION
};

State state = IDLE;
State currentScreen;

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
  uint16_t bmoGreen = tft.color565(150, 240, 186);
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
  uint16_t bmoGreen = tft.color565(150, 240, 186);
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
  uint16_t bmoGreen = tft.color565(150, 240, 186);
  int x = 20 + width + 5; 

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    bmoGreen
  );
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

  tft.init();
  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);

  BMOidleFace();
}

void loop() {
  handleYellowBtn();
  handleRedBtn();

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