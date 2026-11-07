# ESP32-S3-Touch-LCD-1.47 – Lustige Augen

Animierte Cartoon-Augen auf dem 1,47"-Display (172×320, ST7789/JD9853):
Blinzeln, nach links/rechts schauen, Schielen und „genervt" schauen – alles
zufällig gesteuert.

## Vorgehen

1. Display über **Arduino_GFX** ansteuern (Pin-Belegung des Boards ist fix,
   siehe unten).
2. Zeichenfläche als **Canvas-Puffer** im PSRAM – flüssige Animation ohne
   Flackern (komplettes Frame wird per SPI gepusht).
3. Augen als **State-Machine mit weichen Übergängen** (Lerp pro Frame):
   - Blinzeln = Lid, das von oben und unten über das Auge fährt
   - Blickrichtung = Pupillen-Offset
   - Schielen = gegenläufige Pupillen beider Augen (Innen = schielend,
     Außen = „wall-eyed", beides lustig)
   - Genervt = halb geschlossenes Oberlid + schräge Brauen + seitlicher Blick
4. **Zufalls-Scheduler** wählt alle 1–3 s eine neue Aktion.

## Pin-Belegung (Waveshare ESP32-S3-Touch-LCD-1.47)

| LCD-Signal | GPIO |
|---|---|
| MOSI | 39 |
| SCK  | 38 |
| CS   | 21 |
| DC   | 45 |
| RST  | 47 |
| BL (Backlight) | 46 |

Display: 172×320, Spalten-Offset 34, Zeilen-Offset 0 (Rotation 0 = Hochformat).

Hinweis: Falls die Farben invertiert aussehen, in `src/main.cpp` die Konstante
`LCD_INVERT` auf `false` setzen. Es gibt zwei Board-Revisionen – falls das
Display dunkel bleibt, versuche `PIN_LCD_RST = 40`.

## Build & Flash (PlatformIO / VS Code)

1. VS Code + PlatformIO-Erweiterung installieren.
2. Diesen Ordner in VS Code öffnen (`esp32-eyes`).
3. Board per USB-C anschließen, unten in der Statusleiste den Port wählen.
4. `pio run -t upload` (bzw. „Upload"-Pfeil in PlatformIO).
5. Seriellen Monitor: `pio device monitor` (115200 Baud).

Falls das Flashen nicht startet: **BOOT** gedrückt halten, **RESET** drücken
und loslassen, dann **BOOT** loslassen (Download-Modus).

## Alternative: Arduino IDE

1. Board „ESP32S3 Dev Module" installieren, Library **Arduino_GFX**
   (moononournation) installieren.
2. `src/main.cpp` als Sketch `augen/augen.ino` anlegen, Board + Port wählen,
   Upload.

## Anpassen

Alles Relevante steht oben in `src/main.cpp`:

- `EYE_R`, `EYE_Y`, `EYE_CX_L`, `EYE_CX_R`, `PUPIL_R` – Größe/Position der Augen
- `BROW_Y`, `BROW_HALF` – Brauen
- Farben: `COL_BG`, `COL_EYE`, `COL_LINE`, `COL_PUPIL`, `COL_SHINE`
- Verhalten in `update()`: Wahrscheinlichkeiten in `random(100)`
  (12 % Schielen, 16 % genervt, sonst normaler Blick), Blink-Intervall
  `random(1600, 4200)` ms, Doppelblinzeln 22 %

## Erweiterung: Touch

Das Board hat einen AXS5106L-Touch-Controller (I2C-Adresse `0x63`,
SDA = GPIO42, SCL = GPIO41). Idee: Tippen auf das Display → Augen reagieren
erschrocken oder genervt. Dafür z. B. den AXS5106L-Treiber aus den
Waveshare-Demos (Arduino) einbinden und in `loop()` einen Tap erkennen.
