#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
//#include <BLEEddystoneURL.h>
//#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>

#include "RTClib.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "rtc.h"

// Digital I/O used
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define I2S_DOUT      17 //25
#define I2S_BCLK      16 //27
#define I2S_LRC       26

Audio audio;

// RTC
//RTC_PCF8523 rtc;

RTC rtc;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = 5; //In seconds
BLEScan *pBLEScan;
File myFile;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      if (advertisedDevice.haveManufacturerData() == true)
      {
        std::string strManufacturerData = advertisedDevice.getManufacturerData();
        
        uint8_t cManufacturerData[100];
        strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

        if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12)
        {
          Serial.println("Found Cacophony beacon!");
          for (int i = 0; i < strManufacturerData.length(); i++)
          {
            Serial.printf("[%X]", cManufacturerData[i]);
          }
          Serial.printf("\n");
          pBLEScan->stop();
          Serial.println("Stopping scan.");
        }
      }
    }
};

void setup() {
  //======= init ======
  
  // init cpu speed

  // init rtc

  // sd card

  Serial.begin(115200);
  rtc.init();



  //if (!rtc.begin()) {
  //  Serial.println("Couldn't find RTC");
  //  Serial.flush();
  //  while (1) delay(10);
  //}


  Serial.println("Connected to RTC");
  Serial.println("Sleeping for 5 seconds");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(5 * 1000 * 1000);
  esp_light_sleep_start();
  Serial.print("Initializing SD card...");

  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  if (SD.exists("/example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }

  // open a new file and immediately close it:
  Serial.println("Creating example.txt...");
  myFile = SD.open("/example.txt", FILE_WRITE);
  myFile.close();
  

  // Check to see if the file exists:
  if (SD.exists("/example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }
  

  //pinMode(SD_CS, OUTPUT);
  //digitalWrite(SD_CS, HIGH);
  //SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  //Serial.begin(115200);
  //SD.begin(SD_CS);
  Serial.println("Playing audio");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10); // 0...21
  audio.connecttoFS(SD, "barking-05s.wav");

  

  while (1) {audio.loop();}
  
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value




}

void loop()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(2000);
}


