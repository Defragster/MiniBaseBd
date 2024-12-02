/*
  Teensy 4.1 Mini Baseboard Example

  This program checks for presence of PSRAM/Flash memory, NOR SPI Flash,
  SD card in Teensy and the ESP32.  Sends the results to the LCD and serial 
  port and then draws 2 buttons on the LCD screen - Audio and Scan

  If SD card is installed, the Audio   button plays the wave file "SDTEST2.WAV" 
  from the Teensy audio tutorial:   
  https://www.pjrc.com/teensy/td_libs_AudioDataFiles.html

  The Scan button sends a command to the ESP32-C3 requesting a scan of
  available WiFi networks.  When the ESP32-C3 returns the scan results,
  the Teensy 4.1 updates those results on the LCD screen and serial port.
  This requires the sample program ESP32_Demo_Mini_Baseboard be loaded on it, 
  which is just a modified version of the ESP32 WiFiScan example program.

  This example code is in the public domain.
*/
#include <ILI9341_t3.h>
#include <font_Arial.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SerialFlash.h>
#include "LittleFS.h"
extern "C" uint8_t external_psram_size;

#define THIS_GOOD  // Activate the SerialFlash chip

// Setup from audio library
AudioPlaySdWav playSdWav1;
AudioOutputI2S i2s1;
AudioConnection patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection patchCord2(playSdWav1, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;

// Pins used with the SD card
#define SDCARD_CS_PIN BUILTIN_SDCARD

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
#define TS_CS 14 // 24 // 41
//#define TIRQ_PIN  2
XPT2046_Touchscreen ts(TS_CS, TIRQ_PIN);  // Param 2 = NULL - No interrupts

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

#define ESP32SERIAL Serial1  // ESP32 is attached to Serial1 port
#define FLASH_CS 37          //1Gb NOR flash chip CS pin

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
  Serial.begin(115200);       //Initialize USB serial port to computer
  ESP32SERIAL.begin(115200);  //Initialize Seria1 1 connected to ESP32

  // Required to get SPI1 to work with NOR Flash chip
  //SPI1.setMOSI(26);
  //SPI1.setSCK(27);
  SPI1.setMISO(39);
  SPI1.setCS(37);
  //  SPI1.begin();

  // Setup LCD screen
  tft.begin();
  tft.setRotation(1);  // Rotates screen to match the baseboard orientation
  // Setup touch Screen
  ts.begin();
  ts.setRotation(3);  // Sets the touch screen orientation
  tft.fillScreen(ILI9341_BLUE);
  tft.setCursor(1, 110);  // Set initial cursor position
  tft.setFont(Arial_10);  // Set initial font style and size

  // Check for PSRAM chip installed on Teensy
  uint8_t size = external_psram_size;
  Serial.printf("PSRAM Memory Size = %d Mbyte\n", size);
  tft.printf("PSRAM Memory Size = %d Mbyte\n", size);
  if (size == 0) {
    Serial.println("No PSRAM Installed");
    tft.println("No PSRAM Installed");
  }
  tft.println();

  LittleFS_QSPIFlash myfs_NOR;  // NOR FLASH
  LittleFS_QPINAND myfs_NAND;   // NAND FLASH 1Gb

  // Check for NOR Flash chip installed on Teensy
  if (myfs_NOR.begin()) {
    Serial.printf("NOR Flash Memory Size = %d Mbyte / ", myfs_NOR.totalSize() / 1048576);
    Serial.printf("%d Mbit\n", myfs_NOR.totalSize() / 131072);
    tft.printf("NOR Flash Memory Size = %d Mbyte / ", myfs_NOR.totalSize() / 1048576);
    tft.printf("%d Mbit\n", myfs_NOR.totalSize() / 131072);
  }
  // Check for NAND Flash chip installed on Teensy
  else if (myfs_NAND.begin()) {
    Serial.printf("NAND Flash Memory Size =  %d bytes / ", myfs_NAND.totalSize());
    Serial.printf("%d Mbyte / ", myfs_NAND.totalSize() / 1048576);
    Serial.printf("%d Gbit\n", myfs_NAND.totalSize() * 8 / 1000000000);
    tft.print("NAND Flash Memory Size = ");
    tft.printf("%d Mbyte\n", myfs_NAND.totalSize() / 1000000 * 8);
  } else {
    Serial.printf("No Teensy Flash Installed\n");
    tft.printf("No Teensy Flash Installed\n");
  }
  tft.println();

#ifdef THIS_GOOD
  // XXXX   /*
  // This commented section kills touch when SerialFlash.begin is executed
  //
  // Check for NOR Flash on baseboard
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
#endif                  // XXXXX */

  // Check for SD card installed
  if (!(SD.begin(SDCARD_CS_PIN))) {
    Serial.println("SD card not found");
    tft.println("SD card not found");
    audioAdapterAttached = false;
  } else {
    Serial.println("SD card is installed");
    tft.println("SD card is installed");
    audioAdapterAttached = true;
  }
  tft.println();

  // Check for ESP32 installed
  ESP32SERIAL.print("?");         // Ask ESP32 if it is there
  delay(100);                     // Wait a bit for ESP32 to respond
  if (ESP32SERIAL.available()) {  // If there is a response
    String returnData = ESP32SERIAL.readString();
    if (returnData == 'Y') {  // ESP32 responded with Y for Yes, I'm here
      esp32Attached = true;
      Serial.println("ESP32 was found");
      tft.println("ESP32 was found");
    } else {  // No response or invalid response
      Serial.println("ESP32 not found");
      tft.println("ESP32 not found");
      esp32Attached = false;
    }
    Serial.print("\tUSB input like Touch: 'a'udio or 's'can ");
  }

  // Draw buttons
  SetAudioButton(false);
  SetScanButton(false);

  // Setup audio
  if (audioAdapterAttached) {  // Setup the audio
    AudioMemory(8);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.3);
  } else {  // If no audio, gray out button
    tft.setCursor(AUDIO_X + 8, AUDIO_Y + 8);
    tft.setFont(BUTTON_FONT);
    tft.setTextColor(ILI9341_WHITE);
    tft.fillRoundRect(AUDIO_X, AUDIO_Y, AUDIO_W, AUDIO_H, 4, ILI9341_DARKGREY);
    tft.print("No Audio");
  }

  // Setup ESP32 - Gray out button if not installed
  if (!esp32Attached) {  // If no ESP32 gray out button
    tft.setCursor(SCAN_X + 8, SCAN_Y + 8);
    tft.setFont(BUTTON_FONT);
    tft.setTextColor(ILI9341_WHITE);
    tft.fillRoundRect(SCAN_X, SCAN_Y, SCAN_W, SCAN_H, 4, ILI9341_DARKGREY);
    tft.print("No Scan");
  }
}
//===============================================================================
//  Main
//===============================================================================
void loop() {
  // Keep an eye on any audio that may be playing and reset button when it ends
  if (playSdWav1.isStopped() && audioPlaying) {  // Audio finished playing
    SetAudioButton(false);
    Serial.println("Audio finished playing");
  }

  // Check to see if the touch screen has been touched
  TS_Point p;
  p.x = 0; // signal no touch
  if (ts.tirqTouched()) {
    if (ts.touched())
      p = ts.getPoint();
  }
  if (p.x && isTouched == false) {
    // Map the touch point to the LCD screen
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
  if (p.x && p.x < 3000) {
    Serial.print("x = ");  // Show our touch coordinates for each touch
    Serial.print(p.x);
    Serial.print(", y = ");
    Serial.print(p.y);
    Serial.println();
    delay(100);  // Debounce touchscreen a bit
  }

  if (!ts.touched() && isTouched) {
    isTouched = false;  // touchscreen is no longer being touched, reset flag
  }
  // If we requested a scan, look for serial data coming back from the ESP32S
  if (scanRequested && ESP32SERIAL.available()) {
    Serial.print("Read incoming data");
    tft.setCursor(10, 90);
    tft.setFont(Arial_10);
    while (ESP32SERIAL.available()) {  // Print the scan data to the LCD & USB
      String returnData = ESP32SERIAL.readString();
      tft.println(returnData);
      Serial.println(returnData);
    }
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
  static char WavNum = '2';

  if (!audio) {  // button is set inactive, redraw button inactive
    tft.fillRoundRect(AUDIO_X, AUDIO_Y, AUDIO_W, AUDIO_H, 4, ILI9341_RED);
    tft.print("Play Audio");
    audioPlaying = false;
    if (playSdWav1.isPlaying()) {  // Stop any audio that is playing
      playSdWav1.stop();
      Serial.println("Audio being stopped");
    }
  } else {  // button is active, redraw button active
    tft.fillRoundRect(AUDIO_X, AUDIO_Y, AUDIO_W, AUDIO_H, 4, ILI9341_GREEN);
    tft.print("Playing");
    audioPlaying = true;
    if (audioAdapterAttached && !playSdWav1.isPlaying()) {  // Play audio file
      Serial.print("Audio being played: ");
      char szWav[32];
      sprintf(szWav, "SDTEST%c.WAV", WavNum);
      Serial.println(szWav);
      WavNum++;
      if (WavNum > '4') WavNum = '1';
      playSdWav1.play(szWav);
      delay(10);  // wait for library to parse WAV info
    }
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
    ESP32SERIAL.println("S");  // Send command to ESP32 to start scan
    scanRequested = true;      // Set flag that we requested scan
    Serial.println("Scan being requested");
  }
}
