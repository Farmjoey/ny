#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include "BoardConfig.h"

// NOVA RIDE OS - milepæl 1: hardware-probe.
// Touch (XPT2046) læses med egen lille bit-bang-driver, så der ikke kræves eksternt bibliotek.

TFT_eSPI tft;

static bool inverted = false;
static uint8_t rotation = DEFAULT_ROTATION;
static uint8_t touchMode = 1;  // bit0=byt X/Y, bit1=spejl X, bit2=spejl Y

struct Btn { int x, y, w, h; const char* label; };
static const Btn btns[3] = {
  {10, 190, 95, 44, "INVERT"},
  {112, 190, 95, 44, "ROTER"},
  {214, 190, 95, 44, "TOUCH"},
};

static uint16_t xptRead(uint8_t cmd) {
  digitalWrite(TOUCH_CLK, LOW);
  digitalWrite(TOUCH_CS, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(TOUCH_MOSI, (cmd >> i) & 1);
    digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    digitalWrite(TOUCH_CLK, LOW);  delayMicroseconds(1);
  }
  digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);   // busy-bit
  digitalWrite(TOUCH_CLK, LOW);  delayMicroseconds(1);
  uint16_t v = 0;
  for (int i = 11; i >= 0; i--) {
    digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    digitalWrite(TOUCH_CLK, LOW);  delayMicroseconds(1);
    v |= (uint16_t)digitalRead(TOUCH_MISO) << i;
  }
  for (int i = 0; i < 3; i++) {
    digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    digitalWrite(TOUCH_CLK, LOW);  delayMicroseconds(1);
  }
  digitalWrite(TOUCH_CS, HIGH);
  return v;
}

static bool touchRead(int &rx, int &ry) {
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  xptRead(0x90);  // kasseret dummy-læsning
  long sx = 0, sy = 0;
  const int N = 4;
  for (int i = 0; i < N; i++) { sx += xptRead(0xD0); sy += xptRead(0x90); }
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  rx = sx / N; ry = sy / N;
  if (rx < 50 || ry < 50 || rx > 4090 || ry > 4090) return false;
  return true;
}

static void mapTouch(int rx, int ry, int &px, int &py) {
  int W = tft.width(), H = tft.height();
  int a, b;
  if (touchMode & 1) {
    a = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, W);
    b = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, H);
  } else {
    a = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, W);
    b = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, H);
  }
  a = constrain(a, 0, W - 1);
  b = constrain(b, 0, H - 1);
  if (touchMode & 2) a = W - 1 - a;
  if (touchMode & 4) b = H - 1 - b;
  px = a; py = b;
}

static void drawScreen(int rawX = -1, int rawY = -1, int px = -1, int py = -1) {
  tft.fillScreen(TFT_BLACK);
  int w = tft.width();
  tft.fillRect(0, 0, w / 3, 30, TFT_RED);
  tft.fillRect(w / 3, 0, w / 3, 30, TFT_GREEN);
  tft.fillRect(2 * (w / 3), 0, w - 2 * (w / 3), 30, TFT_BLUE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(8, 38);  tft.print("NOVA RIDE OS - HW probe (testversion)");
  tft.setCursor(8, 58);  tft.printf("Profil: %s  rot=%u inv=%d tmode=%u", PROFILE_NAME, rotation, inverted, touchMode);
  tft.setCursor(8, 78);  tft.printf("Chip: %s rev %d, %u MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
  tft.setCursor(8, 98);  tft.printf("Flash: %u KB  PSRAM: %u", ESP.getFlashChipSize() / 1024, ESP.getPsramSize());
  tft.setCursor(8, 118); tft.printf("Fri heap: %u KB", ESP.getFreeHeap() / 1024);
  tft.setCursor(8, 138); tft.printf("Touch raw: %d,%d  px: %d,%d", rawX, rawY, px, py);
  tft.setCursor(8, 158); tft.print("Rod/Gron/Blaa? Hold finger 1,5 s = skift touch-mode");
  for (auto &b : btns) {
    tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2, 2);
    tft.setTextDatum(TL_DATUM);
  }
  if (px >= 0) {
    tft.drawLine(px - 8, py, px + 8, py, TFT_WHITE);
    tft.drawLine(px, py - 8, px, py + 8, TFT_WHITE);
  }
}

static void applySettings() {
  tft.setRotation(rotation);
  tft.invertDisplay(inverted);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(TOUCH_CLK, OUTPUT);
  pinMode(TOUCH_MOSI, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_MISO, INPUT);
  pinMode(TOUCH_IRQ, INPUT);
  tft.init();
  applySettings();
  Serial.printf("[HW] model=%s rev=%d flash=%uKB psram=%u mac=%s profile=%s\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getFlashChipSize() / 1024,
                ESP.getPsramSize(), WiFi.macAddress().c_str(), PROFILE_NAME);
  drawScreen();
}

void loop() {
  static bool wasDown = false, cycled = false;
  static uint32_t pressStart = 0, lastDraw = 0;
  int rx, ry, px, py;
  bool down = touchRead(rx, ry);
  if (down) {
    mapTouch(rx, ry, px, py);
    if (!wasDown) {
      pressStart = millis(); cycled = false;
      Serial.printf("[TOUCH] raw=%d,%d px=%d,%d mode=%u\n", rx, ry, px, py, touchMode);
      for (int i = 0; i < 3; i++) {
        const Btn &b = btns[i];
        if (px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h) {
          if (i == 0) { inverted = !inverted; applySettings(); }
          if (i == 1) { rotation = (rotation + 1) & 3; applySettings(); }
          if (i == 2) { touchMode = (touchMode + 1) & 7; }
        }
      }
      drawScreen(rx, ry, px, py);
      lastDraw = millis();
    } else {
      if (!cycled && millis() - pressStart > 1500) {
        touchMode = (touchMode + 1) & 7; cycled = true;
        drawScreen(rx, ry, px, py); lastDraw = millis();
      } else if (millis() - lastDraw > 250) {
        drawScreen(rx, ry, px, py); lastDraw = millis();
      }
    }
  }
  wasDown = down;
  delay(10);
}
