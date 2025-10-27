#include <SPI.h>
#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <image800.h>
#include <config.h>

#include <Fonts/FreeMonoBold9pt7b.h>
static const uint8_t EPD_BUSY = D5; // to EPD BUSY
static const uint8_t EPD_RST = D4;  // to EPD RST
static const uint8_t EPD_DC = D3;   // to EPD DC
static const uint8_t EPD_CS = D2;   // to EPD CS
static const uint8_t EPD_SCK = D1;  // to EPD CLK
static const uint8_t EPD_MOSI = D0; // to EPD DIN (SDI)
// static const uint8_t EPD_MISO = D10; // not used

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(GxEPD2_750c_Z08(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void InitialiseDisplay();
void helloWorld();
void imageDisplay();

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(5000); // Take some time to open up the Serial Monitor
  Serial.println("E-Paper Display setup");

  InitialiseDisplay();
  // helloWorld();
  imageDisplay();
}

void loop()
{
  delay(1000); // Take some time to open up the Serial Monitor
  Serial.println("E-Paper Display loop");
}

void InitialiseDisplay()
{
  // Explicitly map SPI pins for Seeed XIAO ESP32C3 (or your chosen wiring)
  // If you wire SCK/MOSI to different pins, adjust EPD_SCK/EPD_MOSI above.
  SPI.end();
  SPI.begin(EPD_SCK, /*MISO=*/-1, EPD_MOSI, /*SS=*/EPD_CS);

  // Some big tri-color panels need a moment after power-on
  delay(100);

  Serial.print("busy:");
  Serial.println(digitalRead(EPD_BUSY));
  display.init(115200);
  Serial.print("busy:");
  Serial.println(digitalRead(EPD_BUSY));
}

const char HelloWorld[] = "Hello World!";

void imageDisplay()
{
  Serial.println("imageDisplay");

  const uint16_t W = display.epd2.WIDTH;
  const uint16_t H = display.epd2.HEIGHT;

  // display.setRotation(0);     // adjust if your image was rotated during export
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    // display.drawInvertedBitmap(0, 0, d, W, H, GxEPD_BLACK);
    display.drawRGBBitmap(0, 0, (uint16_t *)IMAGE, W, H);

  } while (display.nextPage());
  Serial.println("imageDisplay done");
}

void helloWorld()
{
  Serial.println("helloWorld");
  // display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
  } while (display.nextPage());
  Serial.println("helloWorld done");
}