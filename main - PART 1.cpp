/* main.cpp -- DELIVERABLE 1
 *
 * Genesis Anne Villar (RED ID: 824435476)
 * Steven Gervacio (RedID: 825656527)
 * CS 596 IOT - Prof. Donyanavard
 * Due Date: 4/16/2025
 *
 * File Description:
 * -- Turn on/off LED using cell phone
 * -- Develop a client-server communication between ESP32 and a cellphone
 * -- Develop two commands: one to switch LED on, and another to switch LED off
 * -- Display LED status on TTGO Lily TFT display
 * 
 * Much of the following code was from given template that we modified!
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <TFT_eSPI.h>  // Include TFT library for TTGO Lily

// define the UUIDs (used ones provided in the lab)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_PIN 13
// define colors
#define TFT_BACKGROUND TFT_BLACK
#define TFT_TEXT       TFT_WHITE
#define TFT_LED_ON     TFT_GREEN
#define TFT_LED_OFF    TFT_RED

// create TFT display instance
TFT_eSPI tft = TFT_eSPI();

// helper function to update display with LED status
void updateDisplay(bool ledStatus) 
{
  // clear the screen
  tft.fillScreen(TFT_BACKGROUND);
  tft.setTextSize(2);
  tft.setTextColor(TFT_TEXT);
  
  // display title
  tft.setCursor(20, 10);
  tft.println("BLE LED Control");
  
  // display current status
  tft.setCursor(20, 50);
  tft.print("LED Status: ");
  
  if (ledStatus) 
  {
    tft.setTextColor(TFT_LED_ON);
    tft.println("ON");
    tft.fillCircle(120, 100, 20, TFT_LED_ON); // draw an circle with border to represent ON LED
  } 
  else 
  {
    tft.setTextColor(TFT_LED_OFF);
    tft.println("OFF");
    
    tft.drawCircle(120, 100, 20, TFT_LED_OFF); // draw an empty circle with border to represent OFF LED
  }
  
  //connection info
  tft.setTextColor(TFT_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 140);
  tft.println("Connect to: SDSUCS");
  tft.setCursor(10, 160);
  tft.println("Send 'ON' or 'OFF'");
}

class MyCallbacks: public BLECharacteristicCallbacks 
{
  void onWrite(BLECharacteristic *pCharacteristic) 
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) 
    {
      Serial.println("*********");
      Serial.print("New command received: ");
      for (int i = 0; i < value.length(); i++)
      {
        Serial.print(value[i]);
      }
      Serial.println();

      // MODIFIED : check if the received command is "ON" or "OFF"
      if (value == "ON") 
      {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED turned ON");
        updateDisplay(true);  // Update TFT display with ON status
      } 
      else if (value == "OFF") 
      {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED turned OFF");
        updateDisplay(false);  // Update TFT display with OFF status
      }
      else 
      {
        Serial.println("Unknown command. Use 'ON' or 'OFF'");
      }
      Serial.println("*********");
    }
  }
};

void setup() 
{
  Serial.begin(9600);
  
  // initialize TFT display
  tft.init();
  tft.setRotation(1);  // landscape orientation
  tft.fillScreen(TFT_BACKGROUND);
  
  pinMode(LED_PIN, OUTPUT); // set up the LED pinv
  digitalWrite(LED_PIN, LOW);  // make sure LED is off initially
  updateDisplay(false); // show initial status on display
  
  Serial.println("ESP32 BLE LED Control");
  Serial.println("1- Download and install the nRF Connect app on your phone");
  Serial.println("2- Scan for BLE devices in the app");
  Serial.println("3- Connect to SDSUCS");
  Serial.println("4- Send 'ON' to turn the LED on");
  Serial.println("5- Send 'OFF' to turn the LED off");
  
  // initialize BLE
  BLEDevice::init("SDSUCS");
  BLEServer *pServer = BLEDevice::createServer();
  
  // create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // create a BLE Characteristic
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                      );
  
  // set callbacks for characteristic
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  // set initial value
  pCharacteristic->setValue("Send ON or OFF");
  pService->start();
  
  // start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  
  Serial.println("BLE server started and waiting for commands");
  
  // update display with connection information
  tft.setTextColor(TFT_BLUE);
  tft.setTextSize(1);
  tft.setCursor(10, 180);
  tft.println("BLE Server Active");
}

void loop() 
{
  delay(2000);
}