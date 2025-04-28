#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include <Arduino.h>
#define MAX_ALARMS 10  // Maksimum 5 alarm per buzzer

class AlarmBuzzer {
private:
  // Struktur untuk menyimpan pola alarm
  typedef struct {
    int beepCount;      // Jumlah beep dalam satu siklus
    int beepDuration;   // Durasi beep (ms)
    int offDuration;    // Durasi off antar beep (ms)
    int longPause;      // Durasi jeda panjang (ms), 0 untuk flip-flop
    int totalDuration;  // Durasi total alarm (ms)
  } AlarmPattern;

  // Struktur untuk menyimpan info alarm
  typedef struct {
    int hour;           // Jam alarm
    int minute;         // Menit alarm
    int second;         // Detik alarm
    AlarmPattern pattern;  // Pola alarm
    bool active;        // Status aktif/tidak
	bool enabled;		// Status enabled 
    unsigned long startTime;  // Waktu mulai alarm
    int step;           // Langkah dalam pola
    unsigned long prevMillis;  // Waktu sebelumnya untuk timing
  } Alarm;

  int pin;              // Pin buzzer
  bool activeHigh;      // True = aktif HIGH, False = aktif LOW
  Alarm alarms[MAX_ALARMS];  // Array 5 alarm
  int alarmCount;       // Jumlah alarm yang digunakan
  unsigned long buzzPrevMillis;
  unsigned long buzzStartTime;
  bool buzzState;
  bool buzzing;

public:
  AlarmBuzzer(int buzzerPin, bool isActiveHigh = true);  // Konstruktor
  void alarm(int alarmIndex, int hour, int minute, int second,
             int cnt, int beepMs, int offMs, int longPause, int duration, bool enable=true);  // Tambah alarm
  void checkAlarm(int hour, int minute, int second);  // Cek dan update alarm dengan waktu
  void buzz(int timeOn, int timeOff, int duration=0); //for buzzing
  void stopBuzzing();
};

#endif