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
- Add timezone to date  // Need to test
- 

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
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define SD_ENABLE     17
#define I2S_DOUT      12
#define I2S_BCLK      14
#define I2S_LRC       25

#define START_MINUTE 0
#define START_HOUR 12

#define LOG_DIR String("/logs")
#define LOG_AUDIO LOG_DIR + "/audio.log"
#define AUDIO_DIR String("/audio")
#define DEVICE_ID_FILE "/cameraDeviceID"

bool triggered = false;

String daysOfTheWeek[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

Audio audio;
RTC rtc;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

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

void addDeviceID(int newID) {
  for (int i = 0; i < DEVICE_ID_LEN; i++) {
    int id = deviceIDs[i];
    if (id == 0) {
      Serial.println("Adding beacon device ID: " + String(newID));
      deviceIDs[i] = newID;
      return;
    }
    if (id == newID) {
      return;
    }
  }
  Serial.println("No more room to store devices IDs from beacons!");
}

bool checkForID(int id) {
  for (int i = 0; i < DEVICE_ID_LEN; i++) {
    if (id == deviceIDs[i]) {
      return true;
    }
  }
  return false;
}

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

/*
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData() == true) {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
      if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12) {
        //Serial.println("Found Cacophony beacon!");
        cacBeacon = true;
        //Serial.printf("\n");
        int id = (cManufacturerData[3]<<8) + cManufacturerData[4];
        addDeviceID(id);
        //pBLEScan->stop();
      }
    }
  }
};
*/

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
  Serial.println("Init Audio");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  /*
  Serial.println("Audio files:");
  File audioDir = SD.open(AUDIO_DIR);
  audioFileCount = 0;
  while (true) {
    File audioFile = audioDir.openNextFile();
    if (!audioFile) {
      break;
    }
    if (!audioFile.isDirectory()) {
      Serial.println(audioFile.name());
      audioFileCount++;
    }
  }
  Serial.println("Found " + String(audioFileCount) + " audio files.");
  */
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

File whatSoundToPlay() {
  int daysAudioFileIndex = TimeSpan(rtc.now().unixtime()).days()%audioFileCount;
  if (rtc.now().isPM()) {
    daysAudioFileIndex = (daysAudioFileIndex+1)%audioFileCount;
  };
  Serial.println("Audio file index for today is: "+ String(daysAudioFileIndex));
  File audioDir = SD.open(AUDIO_DIR);
  File audioFile = audioDir.openNextFile();
  for (int i = 0; i < daysAudioFileIndex; i++) {
    audioFile = audioDir.openNextFile();
    if (!audioFile) {
      break;
    }
    if (!audioFile.isDirectory()) {
      audioFileCount++;
    }
  }
  Serial.println("Audio file to play is" + String(audioFile.name()));
  return audioFile;
}

void playSound(int volume) {
  //String file = whatSoundToPlay().name();
  String file = whatFileToPlay().name();
  //String file = "/audio/3. S1.wav";
  writeLog(LOG_AUDIO, "PLaying: " + file + ", " + String(volume));
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

/*

*/
void initSDCard() {
  Serial.println("Init SD card");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);
  delay(100);
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
    hibernate(5L);
  }
  Serial.println("Checking LOG_DIR exists.");
  if (!SD.exists(LOG_DIR)) {
    Serial.println("Making LOG_DIR");
    SD.mkdir(LOG_DIR);
  }
  Serial.println("Checking AUDIO_DIR exists.");
  if (!SD.exists(AUDIO_DIR)) {
    Serial.println("Making AUDIO_DIR");
    SD.mkdir(AUDIO_DIR);
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

void setup() {
  //setCpuFrequencyMhz(80);
  Serial.begin(115200);
  Serial.println("\n\n================================================");
  rtc.init();
  //initSDCard();
  initAudio();
  
  /*
  if (!rtc.isInActiveWindow(true)) {
    hibernate(60L*5L);
  }
  if (noSoundToPlayTonight()) {
    hibernate(60L*5L);
  }

  beaconScan(scanTime);
  if (triggered) {
    playSound(VOL_LOUD);
  } else {
    Serial.println("no trigger");
  }
  //playSound(VOL_LOUD);
  hibernate(3);
  */


  //int vol = VOL_LOUD;
  //if (checkForID(deviceID)) {
  //  Serial.println("Playing quiet sound..");
  //  vol = VOL_QUIET;
  //}
  //playSound(vol);
  //hibernate(60L*30L);
}

void loop(){
  initSDCard();
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