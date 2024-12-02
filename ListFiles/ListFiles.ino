#include <SerialFlash.h>
#include <SPI.h>
#include <ILI9341_t3.h>
#include <font_Arial.h>
#include <XPT2046_Touchscreen.h>

const int FlashChipSelect = 37;  // digital pin for flash chip CS pin
//const int FlashChipSelect = 6; // digital pin for flash chip CS pin
//const int FlashChipSelect = 21; // Arduino 101 built-in SPI Flash

// touchscreen offset for four corners
#define TS_MINX 400
#define TS_MINY 400
#define TS_MAXX 3879
#define TS_MAXY 3843

// LCD control pins defined by the baseboard
#define TFT_CS 40
#define TFT_DC 9
#define TIRQ_PIN 2

// Use main SPI bus MOSI=11, MISO=12, SCK=13 with different control pins
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

// Touch screen control pins defined by the baseboard
// TIRQ interrupt if used is on pin 2
#define TS_CS 14  // 24 // 41
//#define TIRQ_PIN  2
XPT2046_Touchscreen ts(TS_CS, TIRQ_PIN);  // Param 2 = NULL - No interrupts

void setup() {
  //uncomment these if using Teensy audio shield
  //SPI.setSCK(14);  // Audio shield has SCK on pin 14
  //SPI.setMOSI(7);  // Audio shield has MOSI on pin 7

  //uncomment these if you have other SPI chips connected
  //to keep them disabled while using only SerialFlash
  //pinMode(4, INPUT_PULLUP);
  //pinMode(10, INPUT_PULLUP);

  Serial.begin(9600);

  // wait for Arduino Serial Monitor
  while (!Serial)
    ;
  delay(100);
  Serial.println(F("All Files on SPI Flash chip:"));

  // Setup LCD screen
  tft.begin();
  tft.setRotation(1);  // Rotates screen to match the baseboard orientation
  // Setup touch Screen
  ts.begin();
  ts.setRotation(3);  // Sets the touch screen orientation
  tft.fillScreen(ILI9341_BLUE);
  tft.setCursor(1, 1);   // Set initial cursor position
  tft.setFont(Arial_8);  // Set initial font style and size


  //if (!SerialFlash.begin(FlashChipSelect)) {
  SPI1.setMISO(39);
  SPI1.setCS(FlashChipSelect);
  if (!SerialFlash.begin(SPI1, FlashChipSelect)) {
    error("Unable to access SPI Flash chip");
  }

  SerialFlash.opendir();
  uint32_t fCnt = 0;
  while (1) {
    char filename[64];
    uint32_t filesize;
    tft.printf("File %d ??? \n", fCnt++);
    if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
      Serial.print(F("  "));
      Serial.print(filename);
      spaces(20 - strlen(filename));
      Serial.print(F("  "));
      Serial.print(filesize);
      Serial.print(F(" bytes"));
      Serial.println();

      tft.print(F("  "));
      tft.print(filename);
      spaces(20 - strlen(filename));
      tft.print(F("  "));
      tft.print(filesize);
      tft.print(F(" bytes"));
      tft.println();
    } else {
      break;  // no more files
    }
  }
}

void spaces(int num) {
  for (int i = 0; i < num; i++) {
    Serial.print(' ');
  }
  for (int i = 0; i < num; i++) {
    tft.print(' ');
  }
}

void loop() {
}

void error(const char *message) {
  while (1) {
    Serial.println(message);
    delay(2500);
  }
}
