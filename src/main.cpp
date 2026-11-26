#include <Arduino.h>
#include <Arduino_GFX_Library.h>

static const bool BOARD_TOUCH_VARIANT = true;

#if BOARD_TOUCH_VARIANT
static const int8_t PIN_LCD_DC   = 45;
static const int8_t PIN_LCD_CS   = 21;
static const int8_t PIN_LCD_SCK  = 38;
static const int8_t PIN_LCD_MOSI = 39;
static const int8_t PIN_LCD_RST  = 47;
static const int8_t PIN_LCD_BL   = 46;
#else
static const int8_t PIN_LCD_DC   = 41;
static const int8_t PIN_LCD_CS   = 42;
static const int8_t PIN_LCD_SCK  = 40;
static const int8_t PIN_LCD_MOSI = 45;
static const int8_t PIN_LCD_RST  = 39;
static const int8_t PIN_LCD_BL   = 48;
#endif

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
Arduino_GFX *out = nullptr;

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

  out->fillCircle(cx, EYE_Y, EYE_R, COL_EYE);
  out->drawCircle(cx, EYE_Y, EYE_R, COL_LINE);
  out->drawCircle(cx, EYE_Y, EYE_R - 1, COL_LINE);

  int topEdge = EYE_Y - EYE_R + (int)(topC * 2 * EYE_R);
  int botEdge = EYE_Y + EYE_R - (int)(botC * 2 * EYE_R);

  if (topC > 0.01f) {
    out->fillRect(cx - EYE_R - 2, EYE_Y - EYE_R - 2, 2 * EYE_R + 4, topEdge - (EYE_Y - EYE_R) + 2, COL_BG);
    int dy = topEdge - EYE_Y;
    int hw = (int)sqrtf((float)(EYE_R * EYE_R - dy * dy));
    out->drawLine(cx - hw, topEdge, cx + hw, topEdge, COL_LINE);
    out->drawLine(cx - hw, topEdge + 1, cx + hw, topEdge + 1, COL_LINE);
  }
  if (botC > 0.01f) {
    out->fillRect(cx - EYE_R - 2, botEdge, 2 * EYE_R + 4, EYE_Y + EYE_R + 2 - botEdge, COL_BG);
    int dy = botEdge - EYE_Y;
    int hw = (int)sqrtf((float)(EYE_R * EYE_R - dy * dy));
    out->drawLine(cx - hw, botEdge, cx + hw, botEdge, COL_LINE);
  }

  float hv = EYE_R * (1.0f - topC - botC);
  if (hv > PUPIL_R + 2) {
    int px = cx + (int)(gx * (EYE_R - PUPIL_R));
    int py = EYE_Y + (int)(a.gazeY * (hv - PUPIL_R));
    out->fillCircle(px, py, PUPIL_R, COL_PUPIL);
    out->fillCircle(px - PUPIL_R / 3, py - PUPIL_R / 3, 3, COL_SHINE);
  }
}

static void drawBrow(int16_t cx, int side) {
  int t = (int)(a.brow * 8);
  int x0 = cx - BROW_HALF;
  int x1 = cx + BROW_HALF;
  int y0 = (side > 0) ? (BROW_Y - t) : (BROW_Y + t);
  int y1 = (side > 0) ? (BROW_Y + t) : (BROW_Y - t);
  for (int i = 0; i < 3; i++) {
    out->drawLine(x0, y0 + i, x1, y1 + i, COL_LINE);
  }
}

static void render() {
  out->fillScreen(COL_BG);
  drawEye(EYE_CX_L, +1);
  drawEye(EYE_CX_R, -1);
  drawBrow(EYE_CX_L, +1);
  drawBrow(EYE_CX_R, -1);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("BOOT ok");
  randomSeed(esp_random());

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  Serial.printf("PSRAM size: %lu bytes\n", (unsigned long)ESP.getPsramSize());
  gfx->begin();
  Serial.println("gfx->begin() done");
  static const uint8_t jd9853_init[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 120,
    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23,
    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 4, 0x00, 0x47, 0x00, 0x6F,
    WRITE_COMMAND_8, 0xBB,
    WRITE_BYTES, 6, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,
    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16,
    WRITE_COMMAND_8, 0xC3,
    WRITE_BYTES, 8, 0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,
    WRITE_COMMAND_8, 0xC4,
    WRITE_BYTES, 12, 0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,
    WRITE_COMMAND_8, 0xC8,
    WRITE_BYTES, 32, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
    WRITE_COMMAND_8, 0xD0,
    WRITE_BYTES, 5, 0x04, 0x06, 0x6B, 0x0F, 0x00,
    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14,
    WRITE_C8_D8, 0xDE, 0x01,
    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 5, 0x03, 0x13, 0xEF, 0x35, 0x35,
    WRITE_COMMAND_8, 0xC1,
    WRITE_BYTES, 3, 0x14, 0x15, 0xC0,
    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00,
    WRITE_C8_D8, 0xDE, 0x02,
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x01, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x35, 0x00,
    WRITE_C8_D8, 0x3A, 0x05,
    WRITE_COMMAND_8, 0x2A,
    WRITE_BYTES, 4, 0x00, 0x22, 0x00, 0xCD,
    WRITE_COMMAND_8, 0x2B,
    WRITE_BYTES, 4, 0x00, 0x00, 0x01, 0x3F,
    WRITE_C8_D8, 0xDE, 0x02,
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,
    END_WRITE,
    DELAY, 10,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,
    END_WRITE
  };
  bus->batchOperation(jd9853_init, sizeof(jd9853_init));
  Serial.println("JD9853 init sent");

  gfx->fillScreen(0xF800);
  Serial.println("test: RED");
  delay(1200);
  gfx->fillScreen(0x07E0);
  Serial.println("test: GREEN");
  delay(1200);
  gfx->fillScreen(0x001F);
  Serial.println("test: BLUE");
  delay(1200);

  out = gfx;
  Serial.println("drawing directly to gfx (no canvas)");

  COL_BG    = gfx->color565(14, 16, 34);
  COL_EYE   = 0xFFFF;
  COL_LINE  = gfx->color565(35, 38, 60);
  COL_PUPIL = gfx->color565(18, 22, 52);
  COL_SHINE = 0xFFFF;

  Serial.println("setup done, starting loop");
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
