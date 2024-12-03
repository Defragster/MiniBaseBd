#include <ILI9341_t3.h>
#include <font_Arial.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <SerialFlash.h>

#define THIS_GOOD  //  USE THIS TO TOGGLE SERFLASH

// touchscreen offset for four corners
#if 0
#define TS_MINX 400
#define TS_MINY 400
#define TS_MAXX 3879
#define TS_MAXY 3843
#else  //EDIT ABOVR for your screen - this one seems odd
#define TS_MINX 650
#define TS_MINY 900
#define TS_MAXX 2879
#define TS_MAXY 3100
#endif
// LCD control pins defined by the baseboard
#define TFT_CS 10
#define TFT_DC 9
#define TIRQ_PIN 2
#define TS_CS 8
#define TFT_ROT 1
#define TS_ROT 1

// Use main SPI bus MOSI=11, MISO=12, SCK=13 with different control pins
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
XPT2046_Touchscreen ts(TS_CS);  // Param 2 = NULL - No interrupts

// Define Audio button location and size
#define AUDIO_X 10
#define AUDIO_Y 10
#define AUDIO_W 105
#define AUDIO_H 32

// Define Scan button location and size
#define SCAN_X 10
#define SCAN_Y 50
#define SCAN_W 105
#define SCAN_H 32

#define BUTTON_FONT Arial_14

#define FLASH_CS 37  //Serial flash chip CS pin

// Subroutine prototypes
void SetScanButton(boolean);   // Handles Scan button when touched
void SetAudioButton(boolean);  // Handles Audio button when touched

// Misc flags to keep track of things
boolean isTouched = false;             // Flag if a touch is in process
boolean scanRequested = false;         // Flag if WiFi scan is in process
boolean audioAdapterAttached = false;  // Flag if SD card installed
boolean audioPlaying = false;          // Flag if audio is currently playing
boolean esp32Attached = false;         // Flag if ESP32 is attached

//===============================================================================
//  Initialization
//===============================================================================
void setup() {
  Serial.begin(115200);  //Initialize USB serial port to computer

  // Required to get SPI1 to work with Flash chip
  //SPI1.setMOSI(26); // EXPECTED PIN
  //SPI1.setSCK(27); // EXPECTED PIN
  SPI1.setMISO(39);
  SPI1.setCS(FLASH_CS);
  //  SPI1.begin();  // DONE IN SERIALFLASH

  // Setup LCD screen
  tft.begin();
  tft.setRotation(TFT_ROT);  // Rotates screen to match the baseboard orientation
  // Setup touch Screen
  ts.begin();
  ts.setRotation(TS_ROT);  // Sets the touch screen orientation
  tft.fillScreen(ILI9341_BLUE);
  tft.setCursor(1, 110);  // Set initial cursor position
  tft.setFont(Arial_10);  // Set initial font style and size

#ifdef THIS_GOOD
  // This commented section kills touch when SerialFlash.begin is executed
  // Check for Flash on baseboard
  if (!SerialFlash.begin(SPI1, FLASH_CS)) {
    Serial.println(F("Unable to access SPI Flash chip"));
    tft.println("Unable to access SPI Flash Chip");
  }

  unsigned char id[5];
  SerialFlash.readID(id);
  Serial.print("ID=");
  Serial.println(id[0]);
  Serial.println(id[1]);
  Serial.println(id[2]);
  Serial.println(id[3]);
  Serial.println(id[4]);
  Serial.println();

  Serial.printf("ID: %02X %02X %02X\n", id[0], id[1], id[2]);
  unsigned long sizeFlash = SerialFlash.capacity(id);

  if (sizeFlash > 0) {
    Serial.print("Flash Memory has ");
    Serial.print(sizeFlash);
    Serial.println(" bytes");
    //  tft.printf("Flash Mfr Code EF = ID: %02X %02X %02X\n", id[0], id[1], id[2]);
    tft.printf("SPI NOR Flash Memory Size = %d Mbyte\n", sizeFlash / 1000000 * 8);
  }
  tft.println();
  tft.setFont(Arial_14);
  tft.printf("CAN'T TOUCH THIS!");
  tft.println();
  tft.setTextColor(ILI9341_RED);
  tft.printf("CAN'T TOUCH THIS!");
  tft.println();
#endif  // XXXXX */

  audioAdapterAttached = true;  // fake this to enable touch button
  tft.println();

  esp32Attached = true;
  Serial.print("\tUSB input like Touch: 'a'udio or 's'can ");

  // Draw buttons
  SetAudioButton(false);
  SetScanButton(false);
}
//===============================================================================
//  Main
//===============================================================================
void loop() {
  //    SetAudioButton(false);

  // Check to see if the touch screen has been touched
  TS_Point p, pT;
  p.x = 0;   // signal no touch
  pT.x = 0;  // signal no touch
  if (ts.touched())
    p = ts.getPoint();
  if (p.x && isTouched == false) {
    // Map the touch point to the LCD screen
    pT.x = p.x;
    pT.y = p.y;
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.width());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.height());
    isTouched = true;

    // Look for a Scan Button Hit
    if ((p.x > SCAN_X) && (p.x < (SCAN_X + SCAN_W))) {
      if ((p.y > SCAN_Y) && (p.y <= (SCAN_Y + SCAN_H))) {
        Serial.println("Scan Button Hit");
        if (esp32Attached) SetScanButton(true);
      }
    }
    // Look for an Audio Button Hit
    if ((p.x > AUDIO_X) && (p.x < (AUDIO_X + AUDIO_W))) {
      if ((p.y > AUDIO_Y) && (p.y <= (AUDIO_Y + AUDIO_H))) {
        Serial.println("Audio Button Hit");
        if (audioAdapterAttached && !audioPlaying) {
          SetAudioButton(true);
        } else if (audioAdapterAttached && audioPlaying) {
          SetAudioButton(false);
        }
      }
    }
  }
  if (pT.x) {              //} && p.x < 3000) {
    Serial.print("x = ");  // Show our touch coordinates for each touch
    Serial.print(p.x);
    Serial.print(", y = ");
    Serial.print(p.y);
    Serial.println();

    tft.fillRect(1, SCAN_Y + SCAN_H, 360, 240, ILI9341_BLUE);  // Clear previous scan
    tft.setCursor(1, 100);
    tft.setFont(BUTTON_FONT);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("x = ");  // Show our touch coordinates for each touch
    tft.print(p.x);
    tft.print(", y = ");
    tft.print(p.y);

    delay(50);  // Debounce touchscreen a bit
  }

  if (!ts.touched() && isTouched) {
    isTouched = false;  // touchscreen is no longer being touched, reset flag
  }
  // If we requested a scan, look for serial data coming back from the ESP32S
  if (scanRequested) {
    Serial.print("Read incoming data");
    tft.setCursor(10, 90);
    tft.setFont(Arial_10);
    scanRequested = false;  // Reset the scan flag and button
    SetScanButton(false);
  }
  checkUSB();  // Confirm full function when Touch fails
}

void checkUSB() {
  if (Serial.available()) {
    char inC;
    while (Serial.available()) {
      inC = Serial.read();
      switch (inC) {
        case 'a':
          Serial.println("Audio Button Hit");
          if (audioAdapterAttached && !audioPlaying) {
            SetAudioButton(true);
          } else if (audioAdapterAttached && audioPlaying) {
            SetAudioButton(false);
          }
          break;
        case 's':
          Serial.println("Scan Button Hit");
          if (esp32Attached) SetScanButton(true);
          break;
      }
    }
  }
}

//===============================================================================
//  Routine to draw Audio button current state and control audio playback
//===============================================================================
void SetAudioButton(boolean audio) {
  tft.setCursor(AUDIO_X + 8, AUDIO_Y + 8);
  tft.setFont(BUTTON_FONT);
  tft.setTextColor(ILI9341_WHITE);

  if (!audio) {  // button is set inactive, redraw button inactive
    tft.fillRoundRect(AUDIO_X, AUDIO_Y, AUDIO_W, AUDIO_H, 4, ILI9341_RED);
    tft.print("Play Audio");
    audioPlaying = false;
    Serial.println("Audio being stopped");
  } else {
    tft.fillRoundRect(AUDIO_X, AUDIO_Y, AUDIO_W, AUDIO_H, 4, ILI9341_GREEN);
    tft.print("Playing");
    audioPlaying = true;
    Serial.print("Audio being played: ");
    delay(1000);  // wait for library to parse WAV info
  }
}
//===============================================================================
//  Routine to draw scan button current state and initiate scan request
//===============================================================================
void SetScanButton(boolean scanning) {
  tft.setCursor(SCAN_X + 8, SCAN_Y + 8);
  tft.setFont(BUTTON_FONT);
  tft.setTextColor(ILI9341_WHITE);

  if (!scanning) {  // Button is inactive, redraw button
    tft.fillRoundRect(SCAN_X, SCAN_Y, SCAN_W, SCAN_H, 4, ILI9341_RED);
    tft.print("Scan WiFi");
  } else {                                                     // Button is active, redraw button
    tft.fillRect(1, SCAN_Y + SCAN_H, 360, 240, ILI9341_BLUE);  // Clear previous scan
    tft.fillRoundRect(SCAN_X, SCAN_Y, SCAN_W, SCAN_H, 4, ILI9341_GREEN);
    tft.print("Scanning");
    scanRequested = true;  // Set flag that we requested scan
    Serial.println("Scan being requested");
  }
  delay(1000);  // wait for library to parse WAV info
}
