#include "util.h"
#include "config.h"

void blinkStatus(int code, bool loopForever) {
  switch (code) {
    case STATUS_CODE_RTC_NOT_FOUND:
      Serial.println("RTC not found");
      break;
    case STATUS_CODE_RTC_TIME_NOT_SET:
      Serial.println("RTC Time not set");
      break;
    case STATUS_STARTING:
      Serial.println("Starting LED blink");
      break;
    default: 
      Serial.print("Unknown status code: ");
      Serial.println(code);
      break;
  }
  for (int x = 0; x < code; x++) {
    //digitalWrite(LED_STATUS_PIN, HIGH);
    //delay(500);
    //digitalWrite(LED_STATUS_PIN, LOW);
    //delay(500);
  }
  if (loopForever) {
    delay(2000);
    blinkStatus(code, loopForever);
  }
}