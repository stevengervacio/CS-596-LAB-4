/* main.cpp -- deliverable part b
 *
 * genesis anne villar (red id: 824435476)
 * steven gervacio (redid: 825656527)
 * cs 596 iot - prof. donyanavard
 * due date: 4/16/2025
 *
 * file description:
 * -- develop a very simple bluetooth step counter
 * -- using lsm6dso sensor and esp32 based ttgo
 * -- count number of steps and send via bluetooth to cell phone
 * -- display step count on ttgo lily tft display
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "SparkFunLSM6DSO.h" // correct include for lsm6dso library

// define the uuids (used ones provided in the lab)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// define colors for tft display
#define TFT_BACKGROUND TFT_BLACK
#define TFT_TEXT       TFT_WHITE
#define TFT_HIGHLIGHT  TFT_GREEN
#define TFT_WARNING    TFT_RED

TFT_eSPI tft = TFT_eSPI(); // create instance
LSM6DSO myIMU; // create instance of the lsm6dso class

BLECharacteristic *pStepCharacteristic; // ble characteristic for step count

// step counter variables
int stepCount = 0;
float thresholdAcceleration = 1.2; // initial threshold value - will be calibrated
bool isAboveThreshold = false;     // flag to detect step patterns
unsigned long lastStepTime = 0;    // timestamp of last step for debouncing
unsigned long debounceTime = 300;  // minimum time between steps (300ms)

// variables for calibration
float baselineX = 0.0;
float baselineY = 0.0;
float baselineZ = 0.0;
bool isCalibrated = false;

// update display with step count
void updateStepDisplay() 
{
  tft.fillScreen(TFT_BACKGROUND); // clear the screen
  
  // display title
  tft.setTextSize(2);
  tft.setTextColor(TFT_TEXT);
  tft.setCursor(20, 10);
  tft.println("Step Counter");
  
  tft.setCursor(20, 50);
  tft.print("Steps: ");
  tft.setTextColor(TFT_HIGHLIGHT);
  tft.println(stepCount); // display step count
  
  tft.setTextColor(TFT_TEXT);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  
  tft.setTextColor(TFT_TEXT); // display ble status
  tft.setCursor(20, 100);
  tft.println("BLE Device: SDSUCS");
  
  tft.setCursor(20, 120); // display threshold
  tft.print("Threshold: ");
  tft.println(thresholdAcceleration);
}

// calibrate the accelerometer (takes average readings as baseline)
void calibrateAccelerometer() 
{
  Serial.println("Calibrating accelerometer...");
  tft.setTextColor(TFT_WARNING);
  tft.setTextSize(1);
  tft.setCursor(20, 150);
  tft.println("Keep device steady for calibration");
  
  const int numReadings = 50; // take multiple readings for better accuracy
  float sumX = 0, sumY = 0, sumZ = 0;
  
  for (int i = 0; i < numReadings; i++) 
  {
    // read acceleration values directly using library functions
     sumX += myIMU.readFloatAccelX();
    sumY += myIMU.readFloatAccelY();
    sumZ += myIMU.readFloatAccelZ();
    delay(10);
  }
  
  // calculate average as baseline
  baselineX = sumX / numReadings;
  baselineY = sumY / numReadings;
  baselineZ = sumZ / numReadings;
  
  // account for gravity (1g) in the z-axis
  if (baselineZ > 0.7) 
  {
    baselineZ -= 1.0; // approximate value for gravity when z is pointing up
  } 
  else if (baselineZ < -0.7) 
  {
    baselineZ += 1.0; // approximate value for gravity when z is pointing down
  }
  
  isCalibrated = true;
  Serial.println("Calibration complete!");
  Serial.print("Baseline X: "); Serial.println(baselineX);
  Serial.print("Baseline Y: "); Serial.println(baselineY);
  Serial.print("Baseline Z: "); Serial.println(baselineZ);
  
  updateStepDisplay(); // update display after calibration
}

void sendStepCount() 
{ // send step count via ble
  char stepString[10];
  sprintf(stepString, "%d", stepCount);
  pStepCharacteristic->setValue(stepString);
  pStepCharacteristic->notify();
}

// ble callbacks class
class MyCallbacks: public BLECharacteristicCallbacks 
{
  void onWrite(BLECharacteristic *pCharacteristic) 
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) 
    {
      Serial.println("*********");
      Serial.print("Command received: ");
      for (int i = 0; i < value.length(); i++) 
      {
        Serial.print(value[i]);
      }
      Serial.println();
      
      if (value == "RESET") 
      { // check if it's a reset command
        stepCount = 0;
        Serial.println("Step count reset to 0");
        updateStepDisplay();
        sendStepCount();
      } 
      else if (value == "CALIBRATE") 
      { // check if it's a calibration command
        calibrateAccelerometer();
        Serial.println("Accelerometer recalibrated");
      }
      Serial.println("*********");
    }
  }
};

void setup() 
{
  Serial.begin(9600);
  
  // initialize tft display
  tft.init();
  tft.setRotation(1);  // landscape orientation
  tft.fillScreen(TFT_BACKGROUND);
  tft.setTextSize(2);
  tft.setTextColor(TFT_TEXT);
  tft.setCursor(20, 10);
  tft.println("Step Counter");
  tft.setTextSize(1);
  tft.setCursor(20, 50);
  tft.println("Initializing...");
  
  Wire.begin(); // initialize the lsm6dso sensor
  delay(10);  // short delay to allow i2c setup
  
  if (myIMU.begin()) 
  {
    Serial.println("LSM6DSO sensor initialized");
    
    // use the initialize function with basic settings instead of specific setters
    if (myIMU.initialize(BASIC_SETTINGS)) 
    {
      Serial.println("IMU settings loaded successfully");
    } 
    else 
    {
      Serial.println("Failed to load IMU settings");
    }
    
    tft.setCursor(20, 70);
    tft.setTextColor(TFT_HIGHLIGHT);
    tft.println("Accelerometer connected!");
  } 
  else 
  {
    Serial.println("LSM6DSO sensor initialization failed");
    tft.setCursor(20, 70);
    tft.setTextColor(TFT_WARNING);
    tft.println("ERROR: Accelerometer not found!");
    while (1); // halt if sensor not found
  }
  
  BLEDevice::init("SDSUCS"); // initialize ble
  BLEServer *pServer = BLEDevice::createServer();
  
  BLEService *pService = pServer->createService(SERVICE_UUID); // create the ble service
  
  // create ble characteristic for step count
  pStepCharacteristic = pService->createCharacteristic(
                              CHARACTERISTIC_UUID,
                              BLECharacteristic::PROPERTY_READ |
                              BLECharacteristic::PROPERTY_WRITE |
                              BLECharacteristic::PROPERTY_NOTIFY
                            );
  
  pStepCharacteristic->setCallbacks(new MyCallbacks()); // set callbacks
  
  pStepCharacteristic->setValue("0"); // set initial value
  
  pService->start(); // start the service
  
  // start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("BLE server started. Connect with your phone!");
  
  delay(1000); // initial calibration after short delay
  calibrateAccelerometer();
  updateStepDisplay();
}

void loop() 
{
  // check if accelerometer data is available and system is calibrated
  if (isCalibrated) 
  {
    // read acceleration values directly using library functions
    float currentX = myIMU.readFloatAccelX();
    float currentY = myIMU.readFloatAccelY();
    float currentZ = myIMU.readFloatAccelZ();
    
    // calculate adjusted values (removing baseline)
    float adjustedX = currentX - baselineX;
    float adjustedY = currentY - baselineY;
    float adjustedZ = currentZ - baselineZ;
    
    float accelMagnitude = sqrt(adjustedX*adjustedX + adjustedY*adjustedY + adjustedZ*adjustedZ); // calculate root mean square of acceleration
    
    // debug output once every 50 loops
    static int debugCounter = 0;
    if (++debugCounter >= 50) 
    {
      Serial.print("Acceleration magnitude: ");
      Serial.println(accelMagnitude);
      debugCounter = 0;
    }
    
    unsigned long currentTime = millis(); // step detection with threshold
    
    if (accelMagnitude > thresholdAcceleration) { // check if acceleration crosses threshold
      // if we weren't above threshold before and debounce time has passed
      if (!isAboveThreshold && (currentTime - lastStepTime > debounceTime)) 
      {
        stepCount++;
        lastStepTime = currentTime;
        
        updateStepDisplay(); // update display and send ble notification
        sendStepCount();
        
        Serial.print("Step detected! Count: ");
        Serial.println(stepCount);
      }
      isAboveThreshold = true;
    } 
    else 
    {
      isAboveThreshold = false; // reset the threshold flag when acceleration drops below threshold
    }
  }
  
  delay(20); // small delay to reduce cpu usage, read accelerometer approximately every 20ms
}