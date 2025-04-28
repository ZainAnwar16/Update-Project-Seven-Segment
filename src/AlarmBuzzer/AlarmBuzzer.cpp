#include "AlarmBuzzer.h"

AlarmBuzzer::AlarmBuzzer(int buzzerPin, bool isActiveHigh) {
  pin = buzzerPin;
  activeHigh = isActiveHigh;
  alarmCount = 0;
  pinMode(pin, OUTPUT);
 
  // Inisialisasi alarm
  for (int i = 0; i < MAX_ALARMS; i++) {
    alarms[i].active = false;
    alarms[i].step = 0;
    alarms[i].prevMillis = 0;
  }
  
  // Beep 1000ms blocking
    unsigned long start = millis();
    digitalWrite(pin, activeHigh ? HIGH : LOW);  // Nyalakan buzzer
    while (millis() - start < 1000) {}  // Tunggu 1000ms
    digitalWrite(pin, activeHigh ? LOW : HIGH);  // Matikan buzzer
}

void AlarmBuzzer::alarm(int alarmIndex, int hour, int minute, int second,
                       int cnt, int beepMs, int offMs, int longPause, int duration, bool enable) {
  
  if (alarmIndex >= 0 && alarmIndex < MAX_ALARMS) {
	
	alarms[alarmIndex].enabled = enable; //harus disini agar dipindah terlebih dahulu
	
	if(!alarms[alarmIndex].active && !alarms[alarmIndex].enabled){
		alarms[alarmIndex].active		= false;
		alarms[alarmIndex].step			= 0;
		alarms[alarmIndex].prevMillis	= 0;
	}
	
	if(alarms[alarmIndex].active && !alarms[alarmIndex].enabled){
		alarms[alarmIndex].active		= false;
		alarms[alarmIndex].step			= 0;
		alarms[alarmIndex].prevMillis	= 0;
		digitalWrite(pin, activeHigh ? LOW : HIGH);
	}
	
    alarms[alarmIndex].hour = hour;
    alarms[alarmIndex].minute = minute;
    alarms[alarmIndex].second = second;
    alarms[alarmIndex].pattern.beepCount = cnt;
	if(activeHigh){
	  alarms[alarmIndex].pattern.beepDuration = beepMs;
      alarms[alarmIndex].pattern.offDuration = offMs;
	} else {
	  alarms[alarmIndex].pattern.beepDuration = offMs;
      alarms[alarmIndex].pattern.offDuration = beepMs;
	}
    alarms[alarmIndex].pattern.longPause = longPause;
    alarms[alarmIndex].pattern.totalDuration = duration;
    if (alarmIndex + 1 > alarmCount) {
      alarmCount = alarmIndex + 1;  // Update jumlah alarm
    }
	
	// Serial.print("After "); Serial.print(alarmIndex); 
	// Serial.print("\t"); Serial.print(alarms[alarmIndex].hour); Serial.print(":"); Serial.print(alarms[alarmIndex].minute);
	// Serial.print("\t"); Serial.println(alarms[alarmIndex].active);
  } 
}

void AlarmBuzzer::checkAlarm(int hour, int minute, int second) {
  // Cek dan jalankan alarm
  for (int i = 0; i < alarmCount; i++) {
    Alarm& alarm = alarms[i];
	
	if(!alarm.enabled){
		continue;
	}
	
	//skip alarm jika waktu masih jauh
	if(!alarm.active && (hour < alarm.hour || (hour == alarm.hour && minute < alarm.minute))){
		continue;
	}
	
    // Cek apakah alarm harus mulai
    if (!alarm.active &&
        hour == alarm.hour &&
        minute == alarm.minute &&
        second == alarm.second) {
      alarm.active = true;
      alarm.startTime = millis();
      alarm.step = 0;
      alarm.prevMillis = millis();
    }

    // Jalankan alarm jika aktif
    if (alarm.active) {
      unsigned long elapsedTime = millis() - alarm.startTime;
      int totalSteps = alarm.pattern.longPause > 0
          ? (alarm.pattern.beepCount * 2) + 1
          : (alarm.pattern.beepCount * 2);
      unsigned long timing = (alarm.step < alarm.pattern.beepCount * 2)
          ? ((alarm.step % 2 == 0) ? alarm.pattern.beepDuration : alarm.pattern.offDuration)
          : abs((alarm.pattern.longPause - alarm.pattern.offDuration));
	  

      if (elapsedTime < alarm.pattern.totalDuration || alarm.step > 0) {
        if (millis() - alarm.prevMillis >= timing) {
		  int state = (alarm.step < alarm.pattern.beepCount * 2)
					  ? (alarm.step % 2 == 0 ? HIGH : LOW) : LOW;
          digitalWrite(pin, activeHigh ? state : !state);
		  
          alarm.prevMillis = millis();
          alarm.step++;
          if (alarm.step >= totalSteps) {
            alarm.step = 0;  // Reset pola
          }
        }
      } else {
        alarm.active = false;
        alarm.step = 0;
        digitalWrite(pin, activeHigh ? LOW : HIGH);  // Matikan buzzer
      }
    }
  }
}

void AlarmBuzzer::buzz(int timeOn, int timeOff, int duration){
	if(!buzzing){
		buzzing	= true;
		buzzStartTime = millis();
		buzzPrevMillis = millis();
		buzzState = false;
		digitalWrite(pin, activeHigh ? LOW : HIGH);
	}
	
	if(duration > 0 && (millis() - buzzStartTime >= duration)){
		buzzing = false;
		digitalWrite(pin, activeHigh ? LOW : HIGH);
		return;
	}
	
	if(millis() - buzzPrevMillis >= (buzzState ? timeOn : timeOff)){
		buzzState = !buzzState;
		digitalWrite(pin, activeHigh ? buzzState : !buzzState);
		buzzPrevMillis = millis();
	}
}

void AlarmBuzzer::stopBuzzing(){
	if(buzzing){
		buzzState = false;
		digitalWrite(pin, activeHigh ? buzzState : !buzzState);
		buzzing = false;
	}
}
