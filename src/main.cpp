/*
Audio test.
- Audio will be played about every 30 minutes.
- Each night a different audio file will be chosen to be played for that night.
- Times of audio being played will be logged to the SD card.
- If an bluetooth beacon is detected in the 2 minutes before the audio is to be played it will be played at half volume.
*/

//TODO, top is highest priority.
/*
- Play audio at a quieter volume when there is a beacon.
- Only react from camera with set ID.
- Only play in the set time window.
//===== Extra TODO =======================
- Alert user by buzzer if something is wrong.
- Logs errors and such to file.
- Handle the case of not knowing the time.
- Don't log dir in audio file logs.
- Configure how often the audio will play.
- Text to voice so can comunicate with users more easily.
- Improve log/csv audio logs format.
- Audio range from 0 to 100 not 0 to 21
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

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>


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
#define AUDIO_DIR "/audio"


Audio audio;
RTC rtc;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

#define VOL_LOUD 21    // 0...21
#define VOL_QUIET 10  // 0...21


int audioFileCount = 0;
bool cacBeacon = false;

int scanTime = 20; //In seconds
BLEScan *pBLEScan;
File myFile;

void writeLog(String file, String log) {
  log = rtc.rtc.now().timestamp() + ", " + log;
  Serial.println(file + ": " + log);
  File f = SD.open(file, FILE_APPEND);
  f.println(log);
  f.close();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //Serial.println("Res");
    if (advertisedDevice.haveManufacturerData() == true) {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
      if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12) {
        Serial.println("Found Cacophony beacon!");
        cacBeacon = true;
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
  digitalWrite(SD_ENABLE, LOW);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(seconds * 1000L * 1000L);
  esp_light_sleep_start();
  Serial.println("waking up from sleep");
  digitalWrite(SD_ENABLE, HIGH);
};

File whatSoundToPlay() {
  int daysAudioFileIndex = TimeSpan(rtc.rtc.now().unixtime()).days()%audioFileCount;
  if (rtc.rtc.now().isPM()) {
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

void initAudioDir() {
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
}

void playSound(int volume) {
  String file = whatSoundToPlay().name();
  //String file = "/audio/3. S1.wav";
  writeLog(LOG_AUDIO, file + ", " + String(volume));
  audio.setVolume(volume);
  char buf[file.length()+1];
  file.toCharArray(buf, file.length()+1);
  audio.connecttoFS(SD, buf);
  while (audio.isRunning()) {
    audio.loop();
  }
  writeLog(LOG_AUDIO, "Finished playing: " + file);
}

void hibernate(long seconds) {
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, HIGH);
  esp_sleep_enable_timer_wakeup(seconds * 1000000L);
  esp_deep_sleep_start();
}

void setup() {
  delay(100);
  //======= init ======
  Serial.begin(115200);
  Serial.println("\n\n\n\n================================================");
  //while (true){}
  rtc.init();

  Serial.println("Init SD card");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);
  delay(100);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
    while (1);
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
  Serial.println("Init Audio");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  initAudioDir();
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); 

  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  int vol = VOL_LOUD;
  if (cacBeacon) {
    Serial.println("Playing quiet sound..");
    vol = VOL_QUIET;
  }
  playSound(vol);
  hibernate(10L);
}

void loop(){}