/*
    This sketch interfaces to the Teensy 4.1 via serial port 2.
    It demonstrates how to receive a request for a scan for WiFi networks and 
    report the results back via serial port.

    This is a simple variation of the ESP32 WiFiScan example program
*/
#include "WiFi.h"

#define RX2 20  // Teensy 4.1 is connected to serial port #2
#define TX2 21

const int LED_PIN = 2;  // We will light the LED when Scan is in process.
//===============================================================================
//  Initialization
//===============================================================================
void setup()
{
  Serial.begin(115200);   // USB port
  Serial1.begin(115200, SERIAL_8N1, RX2, TX2);  //Port connected to Teensy 4.1

  // Set WiFi to station mode and disconnect from an AP if previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("Setup done");
}
//===============================================================================
//  Main
//===============================================================================
void loop()
{
  if (Serial1.available()) {
    char command = Serial1.read();
    Serial.println(command);
    if (command == '?') { // Are you there?
      Serial.println("Y");
      Serial1.print("Y");  // Acknowledge I'm attached
    }
    if (command == 'S'){
      Serial.println("scan start");
      digitalWrite(LED_PIN, HIGH);   // turn the LED on
      // WiFi.scanNetworks will return the number of networks found
      int n = WiFi.scanNetworks();
      Serial.println("scan done");
      if (n == 0) {
        Serial.println("no networks found");
        Serial1.println("No networks found");
      } else {
        Serial.print(n);
        Serial1.print(n);
        Serial.println(" Networks Found");
        Serial1.println(" Networks Found");
        for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found to both USB and
          // out Serial2 to attached Teensy 4.1
          Serial.print(i + 1);
          Serial1.print(i + 1);
          Serial.print(": ");
          Serial1.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial1.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial1.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial1.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial1.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
          Serial1.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        }
       }
      }
  digitalWrite(LED_PIN, LOW);    // turn the LED off
  }
}
