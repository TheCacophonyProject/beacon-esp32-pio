#include <Arduino.h>
#include <ESP32_Servo.h>

Servo sprayCan;

#define SPRAY_SERVO_PIN 22
#define SERVO_ENABLE_PIN 19
#define SPRAY_ENABLE_US 1250
#define SPRAY_DISABLE_US 1000
#define SPRAY_DURATION 1000   //Duration in milliseconds 

/*
  hibernate - Will go into low power mode. When waking up the program will start from the beginning.
*/
void hibernate(long seconds) {
  //TODO disable servo
  Serial.println("hibernating for " + String(seconds) + " seconds.");
  esp_sleep_enable_timer_wakeup(seconds * 1000L * 1000L);
  esp_deep_sleep_start();
}

void setup() {
  delay(200);
  Serial.begin(115200);
  Serial.println("\n\n\n======================");
  Serial.println("spray-can-v1");
  pinMode(SERVO_ENABLE_PIN, OUTPUT);
  digitalWrite(SERVO_ENABLE_PIN, HIGH);
  sprayCan.attach(SPRAY_SERVO_PIN);
  sprayCan.writeMicroseconds(SPRAY_DISABLE_US);
  delay(1000);
  Serial.println("spraying");
  sprayCan.writeMicroseconds(SPRAY_ENABLE_US);
  delay(SPRAY_DURATION);
  sprayCan.writeMicroseconds(SPRAY_DISABLE_US);
  Serial.println("finsihed spraying");
  delay(1000);
  sprayCan.detach();
  pinMode(SPRAY_SERVO_PIN, OUTPUT);
  digitalWrite(SPRAY_SERVO_PIN, LOW);
  delay(100);
  hibernate(30L*60L);
}

void loop() {}