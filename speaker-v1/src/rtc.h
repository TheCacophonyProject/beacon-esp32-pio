#ifndef RTC_h_
#define RTC_h_

#include <RTClib.h>
#include <Wire.h>

class RTC {
    public:
        void setup();
        void init();
        int nightOfTheWeek();
        DateTime now();
        String timezoneStr();
        bool isInActiveWindow(bool);
        int daysFromU();
        //RTC_PCF8523 rtc;
        RTC_PCF8563 rtc;
    private:
      boolean dateTimeMatchEEPROMDateTime();
      void printDateTime(DateTime);
};

#endif