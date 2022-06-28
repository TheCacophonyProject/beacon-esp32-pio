#include "rtc.h"
#include "config.h"
#include "util.h"
#include <EEPROM.h>
#include "utcTime.h"
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

#include <RTClib.h>     // https://github.com/adafruit/RTClib
#include <Dusk2Dawn.h>  // https://github.com/dmkishi/Dusk2Dawn

TimeChangeRule NZDST = {"DST", Last, Sun, Sep, 2, 780};    //Daylight savings time = UTC + 13 hours
TimeChangeRule NZSTD = {"ST", First, Sun, Apr, 2, 720};    //Standard time = UTC + 12 hours
Timezone NZTZ(NZDST, NZSTD);

TimeChangeRule *tcr; 

Dusk2Dawn d2d_chch(LAT, LONG, 12);
TwoWire tw = TwoWire(0);

void RTC::setup() {}

void RTC::init() {
  delay(100);
  Serial.println("Running RTC init...  ");  
  tw.setPins(22, 21);
  if (!rtc.begin(&tw)) {
    Serial.println("Couldn't find RTC");
    return; // TODO STATUS_CODE_RTC_NOT_FOUND;
  } else if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running.");
    return; // TODO STATUS_CODE_RTC_TIME_NOT_SET;
  }

  DateTime uploadDateTime = DateTime(COMPILE_UTC_TIME);
  Serial.println("Software updated on (UTC)" + uploadDateTime.timestamp());
  
  EEPROM.begin(512);
  if (EEPROM.readString(0) != uploadDateTime.timestamp()) {
    Serial.println("New compile time. Writing to RTC and saving to EEPROM");
    rtc.adjust(uploadDateTime);
    EEPROM.writeString(0, uploadDateTime.timestamp());
    EEPROM.commit();
  } else if (rtc.now().unixtime() < uploadDateTime.unixtime()) {
    Serial.println("### Invalid RTC time. Time is before compile time, making it invalid. "+rtc.now().timestamp());
    return; // TODO STATUS_CODE_INVALID_RTC_TIME;
  } else if (rtc.now().year() > 2050) {
    Serial.println("### Invalid RTC time. Time is past 2050, should be done by now right? "+rtc.now().timestamp());
    return; // TODO STATUS_CODE_INVALID_RTC_TIME
  }

  Serial.println(DateTime(NZTZ.toLocal(rtc.now().unixtime(), &tcr)).timestamp());
  Serial.println("RTC time (UTC) "+rtc.now().timestamp());
}

int RTC::nightOfTheWeek(){
  DateTime n = now();
  int d = n.dayOfTheWeek();
  if (!n.isPM()) {
    d--;
  }
  return d%7;
}

DateTime RTC::now(){
  return DateTime(NZTZ.toLocal(rtc.now().unixtime(), &tcr));
}

int RTC::daysFromU() {
  return TimeSpan(rtc.now().unixtime()).days();
}

void printMIn24(int m) {
  int h = m/60;
  m = m - 60*h;
  if (h<10) {
    Serial.print("0");
  }
  Serial.print(h);
  Serial.print(":");
  if (m<10) {
    Serial.print("0");
  }
  Serial.println(m);
}

bool RTC::isInActiveWindow(bool printMessages) {
  DateTime now = rtc.now();
  if (printMessages) {
    Serial.println("current datetime is " + now.timestamp());
  }
  
  int minutesFromMidnight = now.hour()*60 + now.minute();
  int startMinute = d2d_chch.sunset(now.year(), now.month(), now.day(), false) - MINUTES_BEFORE_SUNSET;
  int stopMinute = d2d_chch.sunrise(now.year(), now.month(), now.day(), false) + MINUTES_AFTER_SUNRISE;
  
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

