#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Wire.h>
#include "SSD1306Wire.h"

// OLED Display Configuration for Heltec WiFi LoRa 32 V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// BLE Configuration - Update these with your camera's BLE details
#define CAMERA_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  // Replace with actual UUID
#define CAMERA_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"        // Replace with actual UUID
#define CAMERA_DEVICE_NAME "ESP32-CAM-BLE"                      // Replace with actual device name

// Button pins for control (optional - adjust based on your setup)
#define BTN_RECORD 0  // Use built-in PRG button or external button

// State Machine
enum CameraState {
  IDLE,
  SCANNING,
  CONNECTING,
  CONNECTED,
  RECORDING,
  STOPPING,
  SAVING,
  DISCONNECTED,
  ERROR_STATE
};

// Global Variables
SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);
CameraState currentState = IDLE;
BLEClient* pClient = nullptr;
BLEAdvertisedDevice* targetDevice = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
bool deviceFound = false;
bool scanComplete = false;
unsigned long scanStartTime = 0;
const unsigned long SCAN_TIMEOUT = 10000;  // 10 seconds
String errorMessage = "";
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 300;

// BLE Callbacks
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found BLE Device: ");
    Serial.println(advertisedDevice.toString().c_str());
    
    // Check if this is our camera device
    if (advertisedDevice.haveName() && 
        String(advertisedDevice.getName().c_str()).indexOf(CAMERA_DEVICE_NAME) >= 0) {
      Serial.println("Camera device found!");
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
      deviceFound = true;
      advertisedDevice.getScan()->stop();
      scanComplete = true;
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("BLE Connected");
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("BLE Disconnected");
    currentState = DISCONNECTED;
  }
};

// Function Declarations
void updateDisplay(const String& message, bool clear = true);
void initDisplay();
void startBLEScan();
bool connectToCamera();
void sendCommand(const String& command);
void handleButtonPress();
void setState(CameraState newState);

// Initialize OLED Display
void initDisplay() {
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);
  digitalWrite(OLED_RST, HIGH);
  
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.display();
  
  updateDisplay("Camera Controller\nInitializing...");
  delay(1000);
}

// Update Display with Status
void updateDisplay(const String& message, bool clear) {
  if (clear) {
    display.clear();
  }
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  // Parse multiline messages
  int lineHeight = 12;
  int yPos = 0;
  int startPos = 0;
  int newlinePos = message.indexOf('\n');
  
  while (newlinePos != -1 || startPos < message.length()) {
    String line;
    if (newlinePos != -1) {
      line = message.substring(startPos, newlinePos);
      startPos = newlinePos + 1;
      newlinePos = message.indexOf('\n', startPos);
    } else {
      line = message.substring(startPos);
      startPos = message.length();
    }
    
    display.drawString(0, yPos, line);
    yPos += lineHeight;
  }
  
  display.display();
}

// Set State and Update Display
void setState(CameraState newState) {
  currentState = newState;
  
  switch (currentState) {
    case IDLE:
      updateDisplay("Ready\nPress button to scan");
      break;
    case SCANNING:
      updateDisplay("Scanning...\nLooking for camera");
      break;
    case CONNECTING:
      updateDisplay("Connecting...\nPlease wait");
      break;
    case CONNECTED:
      updateDisplay("Connected!\nPress to record");
      break;
    case RECORDING:
      updateDisplay("Recording...\nPress to stop");
      break;
    case STOPPING:
      updateDisplay("Stopping...\nPlease wait");
      break;
    case SAVING:
      updateDisplay("Saving...\nProcessing file");
      break;
    case DISCONNECTED:
      updateDisplay("Disconnected\nPress to reconnect");
      break;
    case ERROR_STATE:
      updateDisplay("Error:\n" + errorMessage);
      break;
  }
}

// Start BLE Scan
void startBLEScan() {
  setState(SCANNING);
  deviceFound = false;
  scanComplete = false;
  scanStartTime = millis();
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(0, false);  // Scan continuously (0 = no timeout)
}

// Connect to Camera
bool connectToCamera() {
  if (!deviceFound || targetDevice == nullptr) {
    errorMessage = "Device not found";
    setState(ERROR_STATE);
    return false;
  }
  
  setState(CONNECTING);
  
  // Create BLE client
  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }
  
  // Connect to the camera
  if (!pClient->connect(targetDevice)) {
    errorMessage = "Connection failed";
    setState(ERROR_STATE);
    return false;
  }
  
  Serial.println("Connected to camera");
  
  // Get the service
  BLERemoteService* pRemoteService = pClient->getService(CAMERA_SERVICE_UUID);
  if (pRemoteService == nullptr) {
    errorMessage = "Service not found";
    setState(ERROR_STATE);
    pClient->disconnect();
    return false;
  }
  
  // Get the characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CAMERA_CHAR_UUID);
  if (pRemoteCharacteristic == nullptr) {
    errorMessage = "Characteristic not found";
    setState(ERROR_STATE);
    pClient->disconnect();
    return false;
  }
  
  setState(CONNECTED);
  return true;
}

// Send Command to Camera
void sendCommand(const String& command) {
  if (pRemoteCharacteristic == nullptr || !pClient->isConnected()) {
    errorMessage = "Not connected";
    setState(ERROR_STATE);
    return;
  }
  
  Serial.print("Sending command: ");
  Serial.println(command);
  
  pRemoteCharacteristic->writeValue(command.c_str(), command.length());
}

// Handle Button Press
void handleButtonPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress < DEBOUNCE_DELAY) {
    return;
  }
  lastButtonPress = currentTime;
  
  switch (currentState) {
    case IDLE:
    case DISCONNECTED:
    case ERROR_STATE:
      startBLEScan();
      break;
      
    case SCANNING:
      // Cancel scan
      BLEDevice::getScan()->stop();
      setState(IDLE);
      break;
      
    case CONNECTED:
      // Start recording
      sendCommand("START");
      setState(RECORDING);
      break;
      
    case RECORDING:
      // Stop recording
      sendCommand("STOP");
      setState(STOPPING);
      // Simulate waiting for camera to stop and save
      delay(500);
      setState(SAVING);
      delay(1000);
      setState(CONNECTED);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Camera LoRa Controller Starting...");
  
  // Initialize button
  pinMode(BTN_RECORD, INPUT_PULLUP);
  
  // Initialize display
  initDisplay();
  
  // Initialize BLE
  BLEDevice::init("Camera_Controller");
  Serial.println("BLE Initialized");
  
  setState(IDLE);
}

void loop() {
  // Check button press
  if (digitalRead(BTN_RECORD) == LOW) {
    handleButtonPress();
  }
  
  // Handle scanning
  if (currentState == SCANNING) {
    // Check if device was found
    if (scanComplete) {
      BLEDevice::getScan()->stop();
      if (deviceFound) {
        // Try to connect
        connectToCamera();
      } else {
        errorMessage = "Camera not found";
        setState(ERROR_STATE);
      }
    }
    // Check for scan timeout
    else if (millis() - scanStartTime > SCAN_TIMEOUT) {
      BLEDevice::getScan()->stop();
      if (!deviceFound) {
        errorMessage = "Camera not found";
        setState(ERROR_STATE);
      }
    }
  }
  
  // Monitor connection status
  if (pClient != nullptr && !pClient->isConnected() && 
      (currentState == CONNECTED || currentState == RECORDING)) {
    setState(DISCONNECTED);
  }
  
  delay(50);  // Small delay to prevent tight loop
}