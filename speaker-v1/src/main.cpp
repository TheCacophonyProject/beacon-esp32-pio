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
- Improve log/csv audio logs format.
- Test active vs non-active scan power usage/reaction time/range.
*/


#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sleep.h>

#include <BLEDevice.h>
#include "RTClib.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "rtc.h"
#include "cacBeacons.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <EEPROM.h>


// Digital I/O used
/*
// V0.1 
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define SD_ENABLE     17
#define I2S_DOUT      12
#define I2S_BCLK      14
#define I2S_LRC       25
#define BUZZER_PIN 16
*/

//V0.2
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define SD_ENABLE     17
#define I2S_DOUT      33
#define I2S_BCLK      25
#define I2S_LRC       26
#define BUZZER_PIN    4
#define STATUS_LED    16
#define EN_5V         2

#define LOG_DIR String("/logs")
#define LOG_AUDIO LOG_DIR + "/audio.log"
#define AUDIO_DIR String("/audio")
#define DEVICE_ID_FILE "/cameraDeviceID.txt"

#define PWM1_Ch 0
#define PWM1_Freq 2700
#define PWM1_Res 8

bool triggered = false;

String daysOfTheWeek[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

Audio audio;
RTC rtc;

#define VOL_LOUD 21    // 0...21
#define VOL_QUIET 10  // 0...21


int audioFileCount = 0;
bool cacBeacon = false;
#define DEVICE_ID_LEN 20
int deviceIDs[DEVICE_ID_LEN];
int deviceID = 0;

//int scanTime = 30; //In seconds
int scanTime = 1; //In seconds
BLEScan *pBLEScan;
File myFile;

void writeLog(String file, String log) {
  log = rtc.now().timestamp() + ", " + log;
  Serial.println(file + ": " + log);
  File f = SD.open(file, FILE_APPEND);
  f.println(log);
  f.close();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData() == true) {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
      if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12) {
        //for (int i = 0; i < strManufacturerData.length(); i++) {
        //  Serial.printf("[%X]", cManufacturerData[i]);
        //}
        if (strManufacturerData[5] == BEACON_CLASSIFICATION_TYPE) {
          ClassificationBeacon cBeacon = ClassificationBeacon(strManufacturerData);
          Serial.print("Device ID: ");
          Serial.println(cBeacon.deviceID);
          Serial.println(deviceID);
          if (deviceID != 0 && cBeacon.deviceID != deviceID) {
            Serial.println("Beacon from different device.");
            return;
          }
          int animal = cBeacon.animal[0];
          Serial.println("Animal: ");
          Serial.println(animal);
          int con = cBeacon.confidences[0];
          Serial.print("Confidence: ");
          Serial.println(con);
          if (animal == 1 || animal == 7) {
            if (con > 80) {
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

int countFilesInDir(String dirStr) {
  File dir = SD.open(dirStr);
  int count = 0;
  while (true) {
    File f = dir.openNextFile();
    if (!f) {
      return count;
    }
    count++;
  }
}

void initAudio() {
  pinMode(EN_5V, OUTPUT);
  digitalWrite(EN_5V, HIGH);
  Serial.println("Init Audio");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
}

int getSoundIndex() {
  EEPROM.begin(512);
  if (EEPROM.read(100) != rtc.nightOfTheWeek()){
    //First sound for this night called
    Serial.println("new night");
    EEPROM.write(100, rtc.nightOfTheWeek());
    EEPROM.write(101, 0);
    EEPROM.commit();  
    return 0;
  }
  int index = EEPROM.read(101);
  index++;
  EEPROM.write(101, index);
  EEPROM.commit();
  return index;
}

File whatFileToPlay(){
  String dir = AUDIO_DIR+"/"+daysOfTheWeek[rtc.nightOfTheWeek()];
  Serial.println(dir);
  int filesForTonight = countFilesInDir(dir);
  File audioDir = SD.open(dir);
  File audioFile = audioDir.openNextFile();
  int daysAudioFileIndex = getSoundIndex()%filesForTonight;
  Serial.println(daysAudioFileIndex);
  for (int i = 0; i < daysAudioFileIndex; i++) {
    audioFile = audioDir.openNextFile();
    if (!audioFile) {
      break;
    }
  }
  Serial.println("Audio file to play is" + String(audioFile.name()));
  return audioFile;
}

bool noSoundToPlayTonight() {
  return countFilesInDir(AUDIO_DIR+"/"+daysOfTheWeek[rtc.nightOfTheWeek()]) == 0;
}

void playSound(int volume) {
  if (noSoundToPlayTonight()) {
    Serial.println("No sound to play for tonight");
    return;
  }
  String file = whatFileToPlay().name();
  writeLog(LOG_AUDIO, "Playing: " + file + ", " + String(volume));
  audio.setVolume(volume);
  char buf[file.length()+1];
  file.toCharArray(buf, file.length()+1);
  audio.connecttoFS(SD, buf);
  while (audio.isRunning()) {
    audio.loop();
  }
  writeLog(LOG_AUDIO, "Finished playing: " + file);
}

/*
  hibernate - Will go into low power mode. When waking up the program will start from the beginning.
*/
void hibernate(long seconds) {
  Serial.println("hibernating for " + String(seconds) + " seconds.");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, HIGH);
  esp_sleep_enable_timer_wakeup(seconds * 1000000L);
  esp_deep_sleep_start();
}

/*
  sleep - Low power delay.
*/
void sleep(long seconds) {
  SD.end();
  delay(100);
  Serial.print("Sleeping for ");
  Serial.print(seconds);
  Serial.println(" seconds");
  digitalWrite(SD_ENABLE, HIGH);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(seconds * 1000L * 1000L);
  esp_light_sleep_start();
  Serial.println("waking up from sleep");
  digitalWrite(SD_ENABLE, LOW);
};

void initSDCard() {
  Serial.println("Init SD card");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);
  delay(100);
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
    hibernate(5L);
  }

  String audioDirs[9] = {};
  audioDirs[0] = LOG_DIR;
  audioDirs[1] = AUDIO_DIR;
  for (int i = 0; i<7; i++) {
    audioDirs[i+2] = AUDIO_DIR + String("/") + daysOfTheWeek[i];
  }
  
  for (int i = 0; i<9; i++) {
    String dir = audioDirs[i];
    if (!SD.exists(dir)) {
      Serial.println("Making folder: "+dir);
      SD.mkdir(dir);
    }
  }

  Serial.println("Checking audio logs file exist");
  if (!SD.exists(LOG_AUDIO)) {
    File f = SD.open(LOG_AUDIO, FILE_APPEND);
    f.println("Datetime, file, volume (0 to 21)");
    f.close();
  }
  if (SD.exists(DEVICE_ID_FILE)) {
    File deviceIDFile = SD.open(DEVICE_ID_FILE, FILE_READ);
    String deviceIDStr = deviceIDFile.readString();
    deviceID = deviceIDStr.toInt();
    Serial.println("Device ID to listen to: " + String(deviceID));
  }
}

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
}

esp_sleep_wakeup_cause_t wakeupReason;

void buzzerOn() {
  ledcWrite(PWM1_Ch, 125);
}

void buzzerOff() {
  ledcWrite(PWM1_Ch, 0);
}

void setup() {
  wakeupReason = esp_sleep_get_wakeup_cause();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(115200);
  Serial.println("\n\n====================");
  rtc.init();
  initAudio();
  ledcAttachPin(BUZZER_PIN, PWM1_Ch);
  ledcSetup(PWM1_Ch, PWM1_Freq, PWM1_Res);
}

void loop(){
  initSDCard();

  if (wakeupReason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.println("Device reset, listen for beacons for testing.");
    buzzerOn();
    delay(1000);
    buzzerOff();
    beaconScan(20); 
    while (triggered) {
      playSound(VOL_LOUD);
      triggered = false;
      beaconScan(20);
    }
    hibernate(1);
  }

  if (!rtc.isInActiveWindow(true)) {
    hibernate(60L*5L);
  }
  if (noSoundToPlayTonight()) {
    hibernate(60L*5L);
  }

  beaconScan(scanTime);
  if (triggered) {
    playSound(VOL_LOUD);
    hibernate(10);
  } else {
    Serial.println("no trigger");
  }

  sleep(3L);
}