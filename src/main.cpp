#include <Arduino.h>
#include <Arduino_GFX_Library.h>

static const int8_t PIN_LCD_DC   = 45;
static const int8_t PIN_LCD_CS   = 21;
static const int8_t PIN_LCD_SCK  = 38;
static const int8_t PIN_LCD_MOSI = 39;
static const int8_t PIN_LCD_RST  = 47;
static const int8_t PIN_LCD_BL   = 46;

static const bool LCD_INVERT = true;

static const int16_t SCREEN_W = 172;
static const int16_t SCREEN_H = 320;

static const int16_t EYE_R    = 40;
static const int16_t EYE_Y    = 150;
static const int16_t EYE_CX_L = 48;
static const int16_t EYE_CX_R = 124;
static const int16_t PUPIL_R  = 13;
static const int16_t BROW_Y   = EYE_Y - EYE_R - 16;
static const int16_t BROW_HALF = 26;

static constexpr float PI_F = 3.14159265358979f;

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_SCK, PIN_LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_LCD_RST, 0, false, SCREEN_W, SCREEN_H, 34, 0, 34, 0);
Arduino_Canvas *canvas = nullptr;

uint16_t COL_BG, COL_EYE, COL_LINE, COL_PUPIL, COL_SHINE;

enum Mood { MOOD_NORMAL, MOOD_CROSS, MOOD_ANNOYED };

struct Anim {
  float gazeX = 0, gazeY = 0;
  float tGazeX = 0, tGazeY = 0;
  float blink = 0;
  bool blinking = false;
  float blinkT = 0;
  float topLid = 0, tTopLid = 0;
  float cross = 0, tCross = 0;
  float brow = 0, tBrow = 0;
  Mood mood = MOOD_NORMAL;
  uint32_t moodEnd = 0, nextBlink = 0;
} a;

uint32_t lastNow = 0;

static void update(uint32_t now, float dt) {
  float k = 1.0f - expf(-dt / 110.0f);
  a.gazeX  += (a.tGazeX  - a.gazeX)  * k;
  a.gazeY  += (a.tGazeY  - a.gazeY)  * k;
  a.topLid += (a.tTopLid - a.topLid) * k;
  a.cross  += (a.tCross  - a.cross)  * k;
  a.brow   += (a.tBrow   - a.brow)   * k;

  if (a.blinking) {
    a.blinkT += dt / 170.0f;
    if (a.blinkT >= 1.0f) {
      a.blinking = false;
      a.blink = 0;
    } else {
      a.blink = sinf(PI_F * a.blinkT);
    }
  }
  if (now >= a.nextBlink) {
    a.blinking = true;
    a.blinkT = 0;
    a.nextBlink = now + random(1600, 4200);
    if (random(100) < 22) a.nextBlink = now + 300;
  }

  if (now >= a.moodEnd) {
    int r = random(100);
    if (r < 12) {
      a.mood = MOOD_CROSS;
      a.tCross = random(2) ? 0.9f : -0.9f;
      a.tGazeX = 0;
      a.tGazeY = 0;
      a.moodEnd = now + random(900, 1900);
    } else if (r < 28) {
      a.mood = MOOD_ANNOYED;
      a.tTopLid = 0.45f;
      a.tBrow = 1.0f;
      a.tGazeX = random(2) ? 0.6f : -0.6f;
      a.tGazeY = -0.2f;
      a.moodEnd = now + random(1800, 3400);
    } else {
      a.mood = MOOD_NORMAL;
      a.tCross = 0;
      a.tTopLid = 0;
      a.tBrow = 0;
      a.tGazeX = (float)random(-85, 86) / 100.0f;
      a.tGazeY = (float)random(-55, 66) / 100.0f;
      a.moodEnd = now + random(800, 2400);
    }
  }
}

static void drawEye(int16_t cx, int side) {
  float gx = a.gazeX + side * a.cross;
  float topC = a.topLid + a.blink * (1.0f - a.topLid);
  float botC = a.blink * (1.0f - a.topLid);

  canvas->fillCircle(cx, EYE_Y, EYE_R, COL_EYE);
  canvas->drawCircle(cx, EYE_Y, EYE_R, COL_LINE);
  canvas->drawCircle(cx, EYE_Y, EYE_R - 1, COL_LINE);

  int topEdge = EYE_Y - EYE_R + (int)(topC * 2 * EYE_R);
  int botEdge = EYE_Y + EYE_R - (int)(botC * 2 * EYE_R);

  if (topC > 0.01f) {
    canvas->fillRect(cx - EYE_R - 2, EYE_Y - EYE_R - 2, 2 * EYE_R + 4, topEdge - (EYE_Y - EYE_R) + 2, COL_BG);
    int dy = topEdge - EYE_Y;
    int hw = (int)sqrtf((float)(EYE_R * EYE_R - dy * dy));
    canvas->drawLine(cx - hw, topEdge, cx + hw, topEdge, COL_LINE);
    canvas->drawLine(cx - hw, topEdge + 1, cx + hw, topEdge + 1, COL_LINE);
  }
  if (botC > 0.01f) {
    canvas->fillRect(cx - EYE_R - 2, botEdge, 2 * EYE_R + 4, EYE_Y + EYE_R + 2 - botEdge, COL_BG);
    int dy = botEdge - EYE_Y;
    int hw = (int)sqrtf((float)(EYE_R * EYE_R - dy * dy));
    canvas->drawLine(cx - hw, botEdge, cx + hw, botEdge, COL_LINE);
  }

  float hv = EYE_R * (1.0f - topC - botC);
  if (hv > PUPIL_R + 2) {
    int px = cx + (int)(gx * (EYE_R - PUPIL_R));
    int py = EYE_Y + (int)(a.gazeY * (hv - PUPIL_R));
    canvas->fillCircle(px, py, PUPIL_R, COL_PUPIL);
    canvas->fillCircle(px - PUPIL_R / 3, py - PUPIL_R / 3, 3, COL_SHINE);
  }
}

static void drawBrow(int16_t cx, int side) {
  int t = (int)(a.brow * 8);
  int x0 = cx - BROW_HALF;
  int x1 = cx + BROW_HALF;
  int y0 = (side > 0) ? (BROW_Y - t) : (BROW_Y + t);
  int y1 = (side > 0) ? (BROW_Y + t) : (BROW_Y - t);
  for (int i = 0; i < 3; i++) {
    canvas->drawLine(x0, y0 + i, x1, y1 + i, COL_LINE);
  }
}

static void render() {
  canvas->fillScreen(COL_BG);
  drawEye(EYE_CX_L, +1);
  drawEye(EYE_CX_R, -1);
  drawBrow(EYE_CX_L, +1);
  drawBrow(EYE_CX_R, -1);
  canvas->flush();
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  gfx->begin();
  if (LCD_INVERT) {
    static const uint8_t inv_on[] = { BEGIN_WRITE, WRITE_COMMAND_8, 0x21, END_WRITE };
    bus->batchOperation(inv_on, sizeof(inv_on));
  }

  canvas = new Arduino_Canvas(SCREEN_W, SCREEN_H, gfx);

  COL_BG    = gfx->color565(14, 16, 34);
  COL_EYE   = 0xFFFF;
  COL_LINE  = gfx->color565(35, 38, 60);
  COL_PUPIL = gfx->color565(18, 22, 52);
  COL_SHINE = 0xFFFF;

  a.blinking = true;
  a.blinkT = 0.5f;
  a.blink = 1;
  a.nextBlink = millis() + 2500;
  a.moodEnd = millis() + 1200;
  lastNow = millis();
}

void loop() {
  uint32_t now = millis();
  float dt = (float)(now - lastNow);
  lastNow = now;
  if (dt > 100) dt = 100;
  update(now, dt);
  render();
  delay(15);
}
