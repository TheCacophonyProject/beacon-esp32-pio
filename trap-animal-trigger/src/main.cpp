/*
Audio test.
- Audio will be played about every 30 minutes.
- Each night a different audio file will be chosen to be played for that night.
- Times of audio being played will be logged to the SD card.
- If an bluetooth beacon is detected in the 1 minute before the audio is to be played it will be played at a quiter volume.
*/

//TODO
/*
//===== Extra TODO =======================
- Alert user by buzzer if something is wrong.
- Logs errors and such to file.
- Handle the case of not knowing the time.
- Don't log dir in audio file logs.
- Configure how often the audio will play.
- Text to voice so can comunicate with users more easily.
- Improve log/csv audio logs format.
- Audio range from 0 to 100 not 0 to 21
- Test active vs non-active scan power usage/reaction time/range.
*/


#include <Arduino.h>
#include <esp_sleep.h>

#include <BLEDevice.h>
#include "cacBeacons.h"

#define LED_PIN 2
#define TRIGGER_PIN 17
bool triggered = false;
BLEScan *pBLEScan;

/*
  hibernate - Will go into low power mode. When waking up the program will start from the beginning.
*/
void hibernate(long seconds) {
  Serial.println("hibernating for " + String(seconds) + " seconds.");
  esp_sleep_enable_timer_wakeup(seconds * 1000000L);
  esp_deep_sleep_start();
}

/*
  sleep - Low power delay.
*/
void sleep(long seconds) {
  Serial.print("Sleeping for ");
  Serial.print(seconds);
  Serial.println(" seconds");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(seconds * 1000L * 1000L);
  esp_light_sleep_start();
  Serial.println("waking up from sleep");
};


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData() == true) {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
      if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12) {
        for (int i = 0; i < strManufacturerData.length(); i++) {
          //Serial.printf("[%X]", cManufacturerData[i]);
        }
        if (strManufacturerData[5] == BEACON_CLASSIFICATION_TYPE) {
          ClassificationBeacon cBeacon = ClassificationBeacon(strManufacturerData);
          Serial.println(cBeacon.deviceID);
          Serial.println(cBeacon.animal[0]);
          int animal = cBeacon.animal[0];
          Serial.println(cBeacon.confidences[0]);
          int con = cBeacon.confidences[0];
          if (animal == 1 || animal == 6 || animal == 7 || animal == 8) {
            if (con > 60) {
              Serial.println("Trigger!!");
              triggered = true;
              pBLEScan->stop();
            }
          }
        }
      }
    }
  }
};

/*
  beaconScan - Will scan for bluetooth beacons for scanTime seconds.
*/
void beaconScan(int scanTime) {
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); 

  Serial.println("Scanning for beacons......");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  Serial.println("Finished scanning.");
}

void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println("\n\n=================");
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  setCpuFrequencyMhz(80);
  beaconScan(2);
  while (triggered) {
    digitalWrite(TRIGGER_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
    delay(2000);
    triggered = false;
    beaconScan(10);
  }
  digitalWrite(TRIGGER_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  hibernate(3);
}

void loop(){}