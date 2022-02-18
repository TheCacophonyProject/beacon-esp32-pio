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

String uploadDateTime = String(__DATE__)+" "+String(__TIME__);

void RTC::init() {
  Serial.println("Running RTC init...  ");
  EEPROM.begin(512);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return; // STATUS_CODE_RTC_NOT_FOUND;
  } else if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running.");
    return; // STATUS_CODE_RTC_TIME_NOT_SET;
  }
  
  if (EEPROM.readString(0) == uploadDateTime) {
    // RTC already written to. Check that the RTC hasn't lost track.
    if (rtc.now().unixtime() < DateTime(F(__DATE__), F(__TIME__)).unixtime()) {
      Serial.println("RTC has lost track of time. It thinks the time is "+rtc.now().timestamp());
      Serial.println("But the code was updated on "+DateTime(F(__DATE__), F(__TIME__)).timestamp());
      blinkStatus(STATUS_CODE_RTC_TIME_NOT_SET, true);
      return;
    }
    Serial.println("RTC set. Time is: "+rtc.now().timestamp());
    return;
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.print("Writing uploadDateTime to EEPROM: ");
  Serial.println(uploadDateTime);
  EEPROM.writeString(0, uploadDateTime);
  EEPROM.commit();
  Serial.println("New time written to RTC of "+rtc.now().timestamp());
}

int RTC::daysFromU() {
  return rtc.now().unixtime()/86400; 
};

bool RTC::isInActiveWindow(bool printMessages) {
  DateTime now = rtc.now();
  if (printMessages) {
    Serial.print("current datetime is ");
    Serial.println(now.timestamp());
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
