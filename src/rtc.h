#ifndef RTC_h_
#define RTC_h_

#include <RTClib.h>
#include <Wire.h>

class RTC {
    public:
        void setup();
        void init();
        DateTime getDateTime();
        bool isInActiveWindow(bool);
        int daysFromU();
        //RTC_PCF8523 rtc;
        RTC_PCF8563 rtc;
        //TwoWire tw;
    private:
      boolean dateTimeMatchEEPROMDateTime();
      void printDateTime(DateTime);
      //TwoWire tw;
};

#endif