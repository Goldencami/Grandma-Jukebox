#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(320, 240);

void drawSmile(int cx, int cy, int r) {
  for (int a = 20; a <= 160; a++) {
    float rad = a * 3.14159 / 180.0;

    int x = cx + r * cos(rad);
    int y = cy + r * sin(rad);

    // thickness = key part (makes it visible as a curve)
    tft.fillCircle(x, y, 2, TFT_BLACK);
  }
}

void BMOneutralFace() {
  int w = tft.width();
  int h = tft.height();

  // background
  tft.fillScreen(tft.color565(150, 240, 186));

  // eye settings
  int eyeR = 7;
  int eyeY = h * 0.35;

  tft.fillCircle(w * 0.4, eyeY, eyeR, TFT_BLACK);
  tft.fillCircle(w * 0.6, eyeY, eyeR, TFT_BLACK);

  // smile :)
  drawSmile(w / 2, h * 0.55, 35);
}

void setup() {
  tft.init();
  tft.setRotation(4);

  BMOneutralFace();
}

void loop() {
}