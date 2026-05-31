#include <TFT_eSPI.h>

#define YELLOW_BTN 25
#define WHITE_BTN 26
#define RED_BTN 27
#define GREEN_BTN 14

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

TFT_eSPI tft = TFT_eSPI(320, 240);
enum State {
  IDLE,
  MUSIC
};

State state = IDLE;
State currentScreen;

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

void setup() {
  Serial.begin(115200);

  pinMode(YELLOW_BTN, INPUT);
  pinMode(WHITE_BTN, INPUT);
  pinMode(RED_BTN, INPUT);
  pinMode(GREEN_BTN, INPUT);

  tft.init();
  tft.setRotation(4);

  BMOidleFace();
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

void handleWhiteBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastWhiteDebounce > debounceDelayWhite) {
    byte newBtnState = digitalRead(WHITE_BTN);

    if(newBtnState != whiteBtnState) {
      whiteBtnState = newBtnState;
      lastWhiteDebounce = timeNow;

      if(whiteBtnState == HIGH) {
        Serial.println("White button pressed");
      }
      else {
        Serial.println("White button released");
      }
    }
  }
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

void loop() {
  handleYellowBtn();
  handleWhiteBtn();
  handleRedBtn();
  handleGreenBtn();

  if(state != currentScreen) {
    currentScreen = state;

    if(state == IDLE) {
      BMOidleFace();
    }
    else if(state == MUSIC) {
      // draw music screen
    }
  }
}