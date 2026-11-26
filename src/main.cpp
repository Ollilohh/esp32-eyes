#include <Arduino.h>
#include <Arduino_GFX_Library.h>

Arduino_DataBus *bus = new Arduino_ESP32SPI(45, 21, 38, 39);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 47, 0, false, 172, 320, 34, 0, 34, 0);

void setup() {
  Serial.begin(115200);
  pinMode(46, OUTPUT);
  digitalWrite(46, HIGH);
  gfx->begin();
  gfx->fillScreen(0xF800);
  delay(1000);
  gfx->fillScreen(0x07E0);
  delay(1000);
  gfx->fillScreen(0x001F);
  delay(1000);
  gfx->fillScreen(0x0000);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(2);
  gfx->setCursor(10, 100);
  gfx->println("Hello Waveshare!");
  gfx->setCursor(10, 140);
  gfx->println("Board OK");
  Serial.println("helloworld done");
}

void loop() {}
