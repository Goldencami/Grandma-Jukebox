#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(320, 240);

void drawSmile(int cx, int cy, int r) {
  for (int a = 20; a <= 160; a++) {
    float rad = a * DEG_TO_RAD;

    int x = cx + r * cos(rad);
    int y = cy + r * sin(rad);

    tft.fillCircle(x, y, 2, TFT_BLACK);
  }
}


void BMOneutralFace() {
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
  tft.init();
  tft.setRotation(4);

  BMOneutralFace();
}

void loop() {
}