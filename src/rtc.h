#ifndef RTC_h_
#define RTC_h_

#include <RTClib.h>

class RTC {
    public:
        void setup();
        void init();
        DateTime getDateTime();
        bool isInActiveWindow(bool);
        RTC_DS1307 rtc;
    private:
      boolean dateTimeMatchEEPROMDateTime();
      void writeDateTimeToEEPROM();
      void printDateTime(DateTime);

};

#endif