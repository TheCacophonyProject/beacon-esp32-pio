#ifndef RTC_h_
#define RTC_h_

#include <RTClib.h>

class RTC {
    public:
        void setup();
        void init();
        DateTime getDateTime();
        bool isInActiveWindow(bool);
        int daysFromU();
        RTC_PCF8523 rtc;
    private:
      boolean dateTimeMatchEEPROMDateTime();
      void printDateTime(DateTime);

};

#endif