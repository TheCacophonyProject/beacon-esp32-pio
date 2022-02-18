#include "util.h"
#include "config.h"

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
    case STATUS_CODE_CAUGHT_PEST:
      Serial.println(F("Switching state to: caught something"));
      break;
    default: 
      Serial.print("Unknown status code: ");
      Serial.println(code);
      break;
  }
  for (int x = 0; x < code; x++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(500);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(500);
  }
  if (loopForever) {
    delay(2000);
    blinkStatus(code, loopForever);
  }
}