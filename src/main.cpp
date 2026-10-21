#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "BoardConfig.h"

TFT_eSPI tft;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

static bool inverted = false;
static uint8_t rotation = DEFAULT_ROTATION;
static bool bgr = false;

struct Btn { int x, y, w, h; const char* label; };
static const Btn btns[3] = {
  {10, 190, 95, 44, "INVERT"},
  {112, 190, 95, 44, "ROTER"},
  {214, 190, 95, 44, "RGB/BGR"},
};

static void drawScreen(int rawX = -1, int rawY = -1, int px = -1, int py = -1) {
  tft.fillScreen(TFT_BLACK);
  // Farvetest: skal ses som rød, grøn, blå
  int w = tft.width();
  tft.fillRect(0, 0, w / 3, 30, TFT_RED);
  tft.fillRect(w / 3, 0, w / 3, 30, TFT_GREEN);
  tft.fillRect(2 * w / 3, 0, w - 2 * (w / 3), 30, TFT_BLUE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(8, 38);  tft.printf("NOVA RIDE OS - HW probe (testversion)");
  tft.setCursor(8, 58);  tft.printf("Profil: %s  rot=%u inv=%d bgr=%d", PROFILE_NAME, rotation, inverted, bgr);
  tft.setCursor(8, 78);  tft.printf("Chip: %s rev %d, %u MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
  tft.setCursor(8, 98);  tft.printf("Flash: %u KB  PSRAM: %u", ESP.getFlashChipSize() / 1024, ESP.getPsramSize());
  tft.setCursor(8, 118); tft.printf("Fri heap: %u KB", ESP.getFreeHeap() / 1024);
  tft.setCursor(8, 138); tft.printf("Touch raw: %d,%d  px: %d,%d", rawX, rawY, px, py);
  tft.setCursor(8, 158); tft.print("Roede=roed? Graes=groen? Blaa=blaa?");
  for (auto& b : btns) {
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
  tft.init();
  applySettings();
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(rotation);
  Serial.printf("[HW] model=%s rev=%d flash=%uKB psram=%u mac=%s profile=%s\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getFlashChipSize() / 1024,
                ESP.getPsramSize(), WiFi.macAddress().c_str(), PROFILE_NAME);
  drawScreen();
}

void loop() {
  static uint32_t lastTouch = 0;
  if (ts.touched() && millis() - lastTouch > 250) {
    TS_Point p = ts.getPoint();
    int px = map(p.x, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, tft.width());
    int py = map(p.y, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, tft.height());
    px = constrain(px, 0, tft.width() - 1);
    py = constrain(py, 0, tft.height() - 1);
    lastTouch = millis();
    Serial.printf("[TOUCH] raw=%d,%d z=%d px=%d,%d\n", p.x, p.y, p.z, px, py);
    for (int i = 0; i < 3; i++) {
      const Btn& b = btns[i];
      if (px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h) {
        if (i == 0) inverted = !inverted;
        if (i == 1) { rotation = (rotation + 1) & 3; ts.setRotation(rotation); }
        if (i == 2) {
          bgr = !bgr;
          // Skriv MADCTL med/uden BGR-bit (0x08) for rotation 1 (0x60 på begge controllere)
          tft.writecommand(0x36);
          tft.writedata((bgr ? 0x08 : 0x00) | 0x60);
        }
        if (i != 2) applySettings();
      }
    }
    drawScreen(p.x, p.y, px, py);
  }
  delay(10);
}
