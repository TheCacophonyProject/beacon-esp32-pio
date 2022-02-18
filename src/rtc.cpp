#include "rtc.h"
#include "config.h"
#include "util.h"
#include <EEPROM.h>

#include <RTClib.h>     // https://github.com/adafruit/RTClib
#include <Dusk2Dawn.h>  // https://github.com/dmkishi/Dusk2Dawn

Dusk2Dawn d2d_chch(LAT, LONG, 13);

void RTC::setup() {
  pinMode(DAYTIME_MODE_PIN, INPUT_PULLUP);
}

void RTC::init() {
  Serial.print("Running RTC init...  ");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return; // STATUS_CODE_RTC_NOT_FOUND;
  } else if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running.");
    return; // STATUS_CODE_RTC_TIME_NOT_SET;
  }
  
  if (dateTimeMatchEEPROMDateTime()) {
    // RTC already written to. Check that the RTC hasn't lost track.
    printDateTime(DateTime(F(__DATE__), F(__TIME__)));
    
    if (rtc.now().unixtime() < DateTime(F(__DATE__), F(__TIME__)).unixtime()) {
      Serial.print("RTC has lost track of time. It thinks the time is ");
      printDateTime(rtc.now());
      Serial.print(" But the code was updated on");
      printDateTime(DateTime(F(__DATE__), F(__TIME__)));
      blinkStatus(STATUS_CODE_RTC_TIME_NOT_SET, true);
      return;
    }
    Serial.println("RTC set");
    return;
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  writeDateTimeToEEPROM();
  Serial.println("New time written to RTC");
}

// Check if the datetime has already been written to the RTC.
boolean RTC::dateTimeMatchEEPROMDateTime() {
  String datetime = String(__DATE__)+String(__TIME__);
  for (int i = 0; i<datetime.length(); i++) {
    if (EEPROM.read(i) != int(datetime[i])) {
      Serial.println("Writing new time to RTC");
      return false;
    }
  }
  return true;
}

//Save date time that was writtent to EEPROM so next boot it won't write to the RTC again.
void RTC::writeDateTimeToEEPROM() {
  String datetime = String(__DATE__)+String(__TIME__);
  for (int i = 0; i<datetime.length(); i++) {
    EEPROM.write(i, int(datetime[i]));
  }
}

DateTime RTC::getDateTime() {
    return rtc.now();
}

void RTC::printDateTime(DateTime d) {
  char buf1[20];
  sprintf(buf1, "%02d-%02d-%02d %02d:%02d:%02d", d.year(), d.month(), d.day(), d.hour(), d.minute(), d.second()); 
  Serial.print(F("Date/Time: "));
  Serial.println(buf1);  
}

bool RTC::isInActiveWindow(bool printMessages) {
  DateTime now = rtc.now();
  if (printMessages) {
    Serial.print("current datetime is");
    printDateTime(now);
  }
  if (digitalRead(DAYTIME_MODE_PIN) == LOW) {
    if (printMessages) {
      Serial.println("24/7 switch is on");
    }
    return true;
  }
  
  int minutesFromMidnight = now.hour()*60 + now.minute();
  int startMinute = d2d_chch.sunset(now.year(), now.month(), now.day(), false) + MINUTES_AFTER_SUNSET;
  int stopMinute = d2d_chch.sunrise(now.year(), now.month(), now.day(), false) - MINUTES_BEFORE_SUNRISE;
  
  if (printMessages) {  
    Serial.print("Time of day: ");
    printMIn24(minutesFromMidnight);
    Serial.print("Startig at : ");
    printMIn24(startMinute);
    Serial.print("Stopping at :");
    printMIn24(stopMinute);
  }

  if (minutesFromMidnight < stopMinute) {
    if (printMessages) {
      Serial.print("Before Sunrise, will stop in: ");
      int minutesLeft = stopMinute - minutesFromMidnight;
      printMIn24(minutesLeft);
    }
    return true;
  }
  else if (minutesFromMidnight > startMinute) {
    if (printMessages) {
      Serial.print("After Sunset, will stop in: ");
      int minutesLeft = stopMinute + 24*60 - minutesFromMidnight;
      printMIn24(minutesLeft);
    }
    return true;
  }
  else {
    if (printMessages) {
      Serial.print("During off period. Need to wait: ");
      printMIn24(startMinute - minutesFromMidnight);
    }
    return false;
  }
};
