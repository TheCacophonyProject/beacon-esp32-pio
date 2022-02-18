#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
//#include <WiFi.h>
#include <esp_sleep.h>

#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEScan.h>
//#include <BLEAdvertisedDevice.h>
//#include <BLEEddystoneURL.h>
//#include <BLEEddystoneTLM.h>
//#include <BLEBeacon.h>

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

#define START_MINUTE 0
#define START_HOUR 12

Audio audio;

// RTC
//RTC_PCF8523 rtc;

RTC rtc;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = 5; //In seconds
BLEScan *pBLEScan;
File myFile;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData() == true) {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
      if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12) {
        Serial.println("Found Cacophony beacon!");
        for (int i = 0; i < strManufacturerData.length(); i++) {
          Serial.printf("[%X]", cManufacturerData[i]);
        }
        Serial.printf("\n");
        pBLEScan->stop();
        Serial.println("Stopping scan.");
      }
    }
  }
};

void sleep(long seconds) {
  Serial.print("Sleeping for ");
  Serial.print(seconds);
  Serial.println(" seconds");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(seconds * 1000L * 1000L);
  esp_light_sleep_start();
  Serial.println("waking up from sleep");
};

void initAudio() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
};

char* getAudioFile() {
  return (char *) "barking-05s.wav";
};

void playFile(int volume, const char *file) {
  Serial.print("Playing file: ");
  Serial.println(file);
  audio.setVolume(volume); // 0...21
  audio.connecttoFS(SD, file);
  while (audio.isRunning()) {
    audio.loop();
  }
  Serial.println("Finsihed playing file");
};

void setup() {
  delay(500);
  //======= init ======
  // init cpu speed
  Serial.begin(115200);
  Serial.println("\n\n\n\n================================================");
  rtc.init();
  //while (1) {};
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
    while (1);
  }
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  //================================

  Serial.println(rtc.daysFromU()%6);
  if (rtc.rtc.now().minute() > START_MINUTE && rtc.rtc.now().hour() > START_HOUR) {

  };

  Serial.println(TimeSpan(rtc.rtc.now().unixtime()).days());
  
  

  playFile(10, getAudioFile());
  
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
