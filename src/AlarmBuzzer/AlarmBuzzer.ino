#include <Wire.h>
#include "RTClib.h"
#include "AlarmBuzzer.h"

RTC_DS3231 rtc;

AlarmBuzzer Buzzer1(D5, true);   // Buzzer 1 di D5, aktif HIGH
AlarmBuzzer Buzzer2(D6, false);  // Buzzer 2 di D6, aktif LOW

void setup() {
  Serial.begin(115200);
 
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // rtc.adjust(DateTime(2023, 11, 1, 7, 0, 0)); // Uncomment untuk set waktu

  // Set alarm untuk Buzzer1 (aktif HIGH)
  Buzzer1.alarm(0, 7, 0, 0, 3, 100, 100, 1000, 60000);    // Alarm 1: 3x beep, 1 menit
  Buzzer1.alarm(1, 7, 1, 0, 5, 2000, 500, 2000, 120000);  // Alarm 2: 5x beep, 2 menit
  Buzzer1.alarm(2, 7, 2, 0, 30, 1000, 1000, 0, 60000);    // Alarm 3: Flip-flop, 1 menit

  // Set alarm untuk Buzzer2 (aktif LOW)
  Buzzer2.alarm(0, 7, 3, 0, 4, 200, 200, 1500, 60000);    // Alarm 1: 4x beep, 1 menit
}

void loop() {
  DateTime now = rtc.now();
  Buzzer1.checkAlarm(now.hour(), now.minute(), now.second());
  Buzzer2.checkAlarm(now.hour(), now.minute(), now.second());

  // Debug waktu
  unsigned long currentMillis = millis();
  if (currentMillis % 1000 == 0) {
    Serial.print(now.hour());
    Serial.print(':');
    Serial.print(now.minute());
    Serial.print(':');
    Serial.println(now.second());
    delay(1);  // Hindari print berulang dalam 1 detik
  }
}