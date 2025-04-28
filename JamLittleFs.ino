#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <PrayerTimes.h>
#include <LittleFSRW.h>
#include <AlarmBuzzer.h>
#include <ShiftSegment.h>
#include <EdgeDetector.h>


RTC_DS3231 rtc;
DateTime dtNow;
LittleFSRW AccesFile;
AlarmBuzzer BuzzerJWS(2, false);                            //***fill GPIO pin Buzzer
ShiftSegment Display7Segment(SPI_, 12, 15, 6);              //***Use SPI latch, brightness, 6 IC
//ShiftSegment Display7Segment(CUSTOM_, 13, 14, 12, 15, 6); //***Use ShiftOur data, clock, latch, brightness, 6 IC
EDGEDTC EDGEDetect(5);


//***only one first setting
IPAddress local_IP(192, 168, 1, 99);    //***BlackPCB
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// IPAddress local_IP(192, 168, 1, 97); //***coba Wemos
// IPAddress gateway(192, 168, 1, 97);
// IPAddress subnet(255, 255, 255, 0);

//================================================================================
//Declaration var JWS
struct varJWSConfig {
  int getHijriandPasar[4];
  int correction[6];
  int loadCorrection[6];
  int valLoad;
  char* nameHijriMonth;
  char* nameJawaDay;
  char JWSWEB[220];
  char PrayTimetoWeb[8][16];
  double jamSholat[sizeof(TimeName)];
  const char* nameDandM[20] = {
    //***Nama Hari Dari 0
    "Minggu", "Senin", "Selasa", "Rabu",
    "Kamis", "Jumat", "Sabtu",
    //***Nama Bulan Dari 7
    "Desember", "Januari", "Februari", "Maret",
    "April", "Mei", "Juni", "Juli",
    "Agustus", "September", "Oktober", "November"
  };
};

varJWSConfig JWSConfig;

//================================================================================
//***Declaration Alarm

struct varAlarmConfig {
  byte tAlarms[2][2];
  byte jamSholat[5];
  byte menitSholat[5];
  String alarmSet[5][9];
  String loadAlarmSet[5][9];
  String valLoad;
};

varAlarmConfig AlarmConfig;

//================================================================================
//***Declaration Display
struct varDisplayConfig {
  bool onSegment;
  int playPattern;
};

varDisplayConfig DisplayConfig;

//================================================================================
//***Declaration WebServer
struct varWebConfig {
  int PWM;
  int lastPWM;
  int valLastSlider;
  bool loadingbuildWeb = false;
};

varWebConfig WebConfig;

//================================================================================
//***Declaration dfPlayer
struct vardfPlayerConfig {
  uint16_t sumMinutes[5];
  uint8_t trigHour[5];
  uint8_t trigMinute[5];
  uint8_t trigSecond[5];
  uint16_t doneMinutes[5];
  int tarkhimMinutes[6] = {48, 27, 27, 27, 32, 35};
  int tarkhimSeconds[6] = {18, 31, 31, 31, 0, 5};
  bool tarkhimTrig[5];
};

vardfPlayerConfig TarkhimConfig;

//================================================================================
//***Declaration All
struct varGlobal {
  unsigned long forMillis[6] = { 0, 0, 0, 0, 0, 0 };
};

varGlobal GlobalVar;

ESP8266WebServer server(80);


//=============================================================================================================================================================
//***Function for Handle Web
void handleRoot() {
  WebConfig.loadingbuildWeb = true;
  //***Send HTML to web Without Chunk if size file small
  //AccesFile.ReadFiletoString("/WebServer2025.html");

  //***Send HTML to web With Chunk if size file big
  //***if inside the function ther is bool chunk, if if true use chunk if else without chunk
  AccesFile.openFile("/WebServer2025.html", true);

  server.setContentLength(AccesFile.getFileSize());
  server.sendHeader("Content-Type", "text/html");
  server.send(200);

  size_t bytesSent = 0;
  uint8_t buffer[128];          //***use multiples such as 64, 128, 256
  while (AccesFile.fileAvailable(true)) {
    size_t bytesRead = AccesFile.readChunk(buffer, sizeof(buffer));
    server.sendContent((char*)buffer, bytesRead);
    bytesSent += bytesRead;     //***for logging summing from bytesRead
  }
  AccesFile.closeFile(true);
  server.sendContent("");
  WebConfig.loadingbuildWeb = false;
}

void handleForm() {
  if (server.hasArg("Jam") && server.hasArg("Menit") && server.hasArg("Detik") && server.hasArg("Tanggal") && server.hasArg("Bulan") && server.hasArg("Tahun")) {
    int hasilJam      = server.arg("Jam").toInt();
    int hasilMenit    = server.arg("Menit").toInt();
    int hasilDetik    = server.arg("Detik").toInt();
    int hasilTanggal  = server.arg("Tanggal").toInt();
    int hasilBulan    = server.arg("Bulan").toInt();
    int hasilTahun    = server.arg("Tahun").toInt();
    //Serial.printf("Set Now: %02d:%02d:%02d %02d/%02d/%d\n", hasilJam, hasilMenit, hasilDetik, hasilTanggal, hasilBulan, hasilTahun);
    rtc.adjust(DateTime(hasilTahun, hasilBulan, hasilTanggal, hasilJam, hasilMenit, (hasilDetik + 1)));
  }

  server.send(200, "text/plain", "Ok");
}

void infoVoltage() {
  String voltage = String(valueVoltage(valuePWM()));
  // Serial.println(voltage);
  if (!WebConfig.loadingbuildWeb) {
    server.send(200, "text/plain", voltage);
  } else {
    server.send(503, "text/plain", "Func infoVoltage, The Server is Busy");
  }
}

void infoPWM() {
  String PWMnow = String(valuePWM());
  // Serial.println(PWMnow);
  if (!WebConfig.loadingbuildWeb) {
    server.send(200, "text/plain", PWMnow);
  } else {
    server.send(503, "text/plain", "Func infoPWM, The Server is Busy");
  }
}

void infoLastSlider() {
  String lastSlider = String(valueSlider());
  // Serial.println(lastSlider);
  if (!WebConfig.loadingbuildWeb) {
    server.send(200, "text/plain", lastSlider);
  } else {
    server.send(503, "text/plain", "Func infoLastSlider, The Server is Busy");
  }
}

void infoJWSWEB() {
  int Awal = ESP.getFreeHeap();
  if (!WebConfig.loadingbuildWeb) {
    snprintf(JWSConfig.JWSWEB, sizeof(JWSConfig.JWSWEB), "► SURABAYA, %s %s %02d %s %d / %d %s %d H - %s - %s - %s - %s - %s - %s - %s - %s ◄",
             JWSConfig.nameDandM[dtNow.dayOfTheWeek()], JWSConfig.nameJawaDay, dtNow.day(), JWSConfig.nameDandM[dtNow.month() + 7], dtNow.year(),
             JWSConfig.getHijriandPasar[0], JWSConfig.nameHijriMonth, JWSConfig.getHijriandPasar[2],
             JWSConfig.PrayTimetoWeb[0], JWSConfig.PrayTimetoWeb[1], JWSConfig.PrayTimetoWeb[2], JWSConfig.PrayTimetoWeb[3],
             JWSConfig.PrayTimetoWeb[4], JWSConfig.PrayTimetoWeb[5], JWSConfig.PrayTimetoWeb[6], JWSConfig.PrayTimetoWeb[7]);

    server.send(200, "text/plain", JWSConfig.JWSWEB);  //◄►
  } else {
    server.send(503, "text/plain", "Func infoJWSWEB, The Server is Busy");
  }

  int Akhir = ESP.getFreeHeap();  // Serial.print("JWSWEB = "); // Serial.println(Awal - Akhir);
}

void infoIDChip() {
  String ChipID = String(ESP.getChipId(), HEX);
  ChipID.toUpperCase();
  // Serial.println(ESP.getChipId(),HEX);
  if (!WebConfig.loadingbuildWeb) {
    server.send(200, "text/plain", "0x" + ChipID);
  } else {
    server.send(503, "text/plain", "Func infoIDChip, The Server is Busy");
  }
}

void TurnOnAll() {
  String buttonS = server.arg("valstate7Seg");
  DisplayConfig.onSegment = (buttonS == "1");
  server.send(200, "text/plain", "0");
}

void RestartESP() {
  String Trg = server.arg("valRestart");
  // Serial.println(Trg);
  server.send(200, "text/plain", Trg);
  if (Trg == "1") {
    ESP.restart();
  }
}

void infoSliderNow() {
  String sliderNow = server.arg("valueSlider");
  // Serial.println("test " + sliderNow);
  server.send(200, "text/plain", "0");
  //=============================================================================
  //***Handle For Save Value from web to file /InfoPWMandSlider.txt
  WebConfig.lastPWM = map(sliderNow.toInt(), 0, 100, 0, 255);
  if (WebConfig.lastPWM != WebConfig.PWM || WebConfig.valLastSlider != sliderNow.toInt()) {
    WebConfig.PWM = WebConfig.lastPWM;
    WebConfig.valLastSlider = sliderNow.toInt();
    int dataInt[] = { WebConfig.PWM, WebConfig.valLastSlider };
    AccesFile.FileWriteInt("/InfoPWMandSlider.txt", dataInt, 2, ";");
  }
}

void correctionTime() {
  if (server.hasArg("hijriahDay") && server.hasArg("subuh") && server.hasArg("dzuhur") && server.hasArg("asar") && server.hasArg("maghrib") && server.hasArg("isya")) {
    JWSConfig.correction[0] = server.arg("hijriahDay").toInt();
    JWSConfig.correction[1] = server.arg("subuh").toInt();
    JWSConfig.correction[2] = server.arg("dzuhur").toInt();
    JWSConfig.correction[3] = server.arg("asar").toInt();
    JWSConfig.correction[4] = server.arg("maghrib").toInt();
    JWSConfig.correction[5] = server.arg("isya").toInt();
    AccesFile.FileWriteInt("/infoCorrection.txt", JWSConfig.correction, sizeof(JWSConfig.correction) / sizeof(int), ";");
    // Serial.println("Correction values updated:");
    // Serial.printf("Hijriah Day: %d, Subuh: %d, Dzuhur: %d, Asar: %d, Maghrib: %d, Isya: %d\n",
    //               JWSConfig.correction[0], JWSConfig.correction[1], JWSConfig.correction[2],
    //               JWSConfig.correction[3], JWSConfig.correction[4], JWSConfig.correction[5]);
  }
  server.send(200, "text/plain", "OK");
}

void alarmSet() {

  bool allParamsPresent = true;
  for (int i = 0; i < 5; i++) {
    if (!server.hasArg("alarm" + String(i + 1) + "Hour") || !server.hasArg("alarm" + String(i + 1) + "Minute") || !server.hasArg("alarm" + String(i + 1) + "Second") || !server.hasArg("alarm" + String(i + 1) + "Cnt") || !server.hasArg("alarm" + String(i + 1) + "Ton") || !server.hasArg("alarm" + String(i + 1) + "Toff") || !server.hasArg("alarm" + String(i + 1) + "longP") || !server.hasArg("alarm" + String(i + 1) + "Dur") || !server.hasArg("alarm" + String(i + 1) + "Active")) {
      allParamsPresent = false;
      break;
    }
  }

  if (allParamsPresent) {
    for (int i = 0; i < 5; i++) {
      String alarmPrefix = "alarm" + String(i + 1);
      AlarmConfig.alarmSet[i][0] = server.arg(alarmPrefix + "Hour");
      AlarmConfig.alarmSet[i][1] = server.arg(alarmPrefix + "Minute");
      AlarmConfig.alarmSet[i][2] = server.arg(alarmPrefix + "Second");
      AlarmConfig.alarmSet[i][3] = server.arg(alarmPrefix + "Cnt");
      AlarmConfig.alarmSet[i][4] = server.arg(alarmPrefix + "Ton");
      AlarmConfig.alarmSet[i][5] = server.arg(alarmPrefix + "Toff");
      AlarmConfig.alarmSet[i][6] = server.arg(alarmPrefix + "longP");
      AlarmConfig.alarmSet[i][7] = server.arg(alarmPrefix + "Dur");
      AlarmConfig.alarmSet[i][8] = server.arg(alarmPrefix + "Active");
    }
  }

  String alarmArray[5 * 9];
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 9; j++) {
      alarmArray[i * 9 + j] = AlarmConfig.alarmSet[i][j];
    }
  }

  AccesFile.FileWriteString("/infoAlarm.txt", alarmArray, 5 * 9, ";");

  //   Serial.println("Alarm values updated:");
  //   for (int i = 0; i < 15; i += 3) {
  //       Serial.printf("Alarm %d: %02d:%02d:%02d\n", (i / 3) + 1, AlarmConfig.alarmSet[i], AlarmConfig.alarmSet[i + 1], AlarmConfig.alarmSet[i + 2]);
  //   }
  // }
  server.send(200, "text/plain", "OK");
}

void loadcorrectionTimeJWS() {
  String data = "";
  for (int i = 0; i < sizeof(JWSConfig.loadCorrection) / sizeof(int); i++) {
    AccesFile.FileReadInt("/infoCorrection.txt", i + 1, ";", JWSConfig.valLoad);
    JWSConfig.loadCorrection[i] = JWSConfig.valLoad;
    data += String(JWSConfig.valLoad);
    if (i < 5) data += ",";
  }
  server.send(200, "text/plain", data);
}

void loadalarmSet() {
  String data = "";
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 9; j++) {
      AccesFile.FileReadString("/infoAlarm.txt", i * 9 + j + 1, ";", AlarmConfig.valLoad);
      AlarmConfig.loadAlarmSet[i][j] = AlarmConfig.valLoad;
      data += AlarmConfig.valLoad;
      if (i < 4 || j < 8) {
        data += ",";
      }
    }
  }

  // Serial.println("Load Alarm value updates: ");
  // for(int i=0; i< 5; i++){
  //   Serial.print("Alarm ke-");
  //   Serial.print(i+1);
  //   Serial.print(" : ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][0]); Serial.print(":");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][1]); Serial.print(":");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][2]); Serial.print(", cnt= ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][3]); Serial.print(" ton= ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][4]); Serial.print(" toff= ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][5]); Serial.print(" longP= ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][6]); Serial.print(" dur= ");
  //   Serial.print(AlarmConfig.loadAlarmSet[i][7]); Serial.print(" active= ");
  //   Serial.println(AlarmConfig.loadAlarmSet[i][8]);
  // }

  server.send(200, "text/plain", data);
}



//=============================================================================================================================================================
//***Handle For Load Value from file /InfoPWMandSlider.txt

int valuePWM() {
  int valPWM;
  AccesFile.FileReadInt("/InfoPWMandSlider.txt", 1, ";", valPWM);
  return valPWM;
}
double valueVoltage(int input) {
  return (input * 3.3) / 255;
}

int valueSlider() {
  int valSlider;
  AccesFile.FileReadInt("/InfoPWMandSlider.txt", 2, ";", valSlider);
  return valSlider;
}




//=============================================================================================================================================================
//=============================================================================================================================================================
//=============================================================================================================================================================
//***Process
void setup() {
  //=============================================================================================================================================================
  //***Setup For All
  Serial.begin(115200);
  Display7Segment.setPatternType(true);  //true jika menggunakan CA
  rtc.begin();
  //***Additional for Debug
  pinMode(16,1);

  //=============================================================================================================================================================
  //***Begin For WiFi
  WiFi.mode(WIFI_AP);
  //***only one first setting
  // WiFi.softAP(ssid, password, channel(1), hidden(T(1)/F(0)), max_connection)
  // WiFi.softAP("ESP826612FCoba", "12345678");  //Connect to your WiFi router coba WEMOS
  WiFi.softAP("ESPTC7SEGTRIX", "#ESP826612F#");  //Black PCB
  WiFi.softAPConfig(local_IP, gateway, subnet);
  // Serial.println("");
 
  //***If connection successful show IP address in serial monitor
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println("WiFi");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.softAPIP());  //IP address assigned to your ESP

  //=============================================================================================================================================================
  //***Server on (EndPoint)
  server.on("/", handleRoot);                 //***Which routine to handle at root location
  server.on("/setDateTime", handleForm);
  server.on("/brightnessRead", infoVoltage);
  server.on("/vPWMRead", infoPWM);
  server.on("/lastSliderRead", infoLastSlider);
  server.on("/sliderValue", infoSliderNow);
  server.on("/JWSWeb", infoJWSWEB);
  server.on("/IDEsp", infoIDChip);
  server.on("/set7Seg", TurnOnAll);
  server.on("/RestartESP", RestartESP);
  server.on("/correctionTimeJWS", correctionTime);
  server.on("/alarmSet", alarmSet);
  server.on("/getCorrection", loadcorrectionTimeJWS);
  server.on("/getAlarms", loadalarmSet);
  //***http://192.168.1.99/action_page?Jam=15&Tanggal=7&Menit=41&Bulan=8&Detik=7&Tahun=2022&vPWM=15&nowPWM=valuePWM
  server.begin();  //Start server
  // Serial.println("HTTP server started");

  //=============================================================================================================================================================
  //***PrayerTimes Setup
  set_calc_method(MWL);
  set_asr_method(Shafii);
  set_high_lats_adjust_method(AngleBased);
  //***1 derajat kurleb 4 menit
  set_fajr_angle(20);
  set_isha_angle(18);
  set_imsak_angle(22.38);  //***ex : imsk angel - fjr angel =  22.3 -20 = 2.3 degree * 4mnt/degree = 9.2 menit dibulatkan menjadi 10mnt
  set_dhuha_angle(-5.8);   //***ex : 6.7 degree * 4mnt/degree = 27.3 mnt dibulatkan 27mnt

  //=============================================================================================================================================================
  //***load file because first load
  for (int i = 0; i < sizeof(JWSConfig.loadCorrection) / sizeof(int); i++) {
    AccesFile.FileReadInt("/infoCorrection.txt", i + 1, ";", JWSConfig.valLoad);
    JWSConfig.loadCorrection[i] = JWSConfig.valLoad;
  }

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 9; j++) {
      AccesFile.FileReadString("/infoAlarm.txt", i * 9 + j + 1, ";", AlarmConfig.valLoad);
      AlarmConfig.loadAlarmSet[i][j] = AlarmConfig.valLoad;
    }
  }
}


void loop() {

  //***Task 5ms, prioritas Server
  if ((millis() - GlobalVar.forMillis[0]) >= 5) {
    server.handleClient();
    GlobalVar.forMillis[0] = millis();
  }

  //***Task, 50ms for setting Display
  if ((millis() - GlobalVar.forMillis[1]) >= 50 && !WebConfig.loadingbuildWeb) {

    //***Setting brightness Display
    Display7Segment.setBrightness(8, valuePWM());

    //***Animation Test
    if (EDGEDetect.EDGEPOS(0, DisplayConfig.onSegment)) {
      DisplayConfig.playPattern++;
      if (DisplayConfig.playPattern == 9) {
        DisplayConfig.playPattern = 1;
      }
    }
    if (EDGEDetect.EDGENEG(1, DisplayConfig.onSegment) && DisplayConfig.playPattern == 8) {
      BuzzerJWS.stopBuzzing();
    }

    if (DisplayConfig.onSegment) {
      if (DisplayConfig.playPattern == 1) Display7Segment.turnOn(0);
      if (DisplayConfig.playPattern == 2) Display7Segment.animateRunning(LEFT_TO_RIGHT, 20);
      if (DisplayConfig.playPattern == 3) Display7Segment.animateRunning(RIGHT_TO_LEFT, 20);
      if (DisplayConfig.playPattern == 4) Display7Segment.waveEffect(50);
      if (DisplayConfig.playPattern == 5) Display7Segment.randomFlash(50);
      if (DisplayConfig.playPattern == 6) Display7Segment.pulseAll(200);
      if (DisplayConfig.playPattern == 7) Display7Segment.bounceEffect(50);
      if (DisplayConfig.playPattern == 8) BuzzerJWS.buzz(500, 500);
    }

    //***Check Alarm
    BuzzerJWS.checkAlarm(dtNow.hour(), dtNow.minute(), dtNow.second());

    GlobalVar.forMillis[1] = millis();
  }

  //***Task 1s, for Display
  if ((millis() - GlobalVar.forMillis[2]) >= 1000 && !WebConfig.loadingbuildWeb) {

    dtNow = rtc.now();
    if (!DisplayConfig.onSegment) {
      Display7Segment.displayTime(dtNow.hour(), dtNow.minute(), dtNow.second(), 0, SECOND_FIRST_RIGHT, 3, (((dtNow.second() % 10) % 2) == 0) ? false : true);
    }

    GlobalVar.forMillis[2] = millis();
  }

  //***Task 1s, Calculate JWS, Settign and Correction Alarm, & Start Tarkhim
  if ((millis() - GlobalVar.forMillis[3]) >= 1000 && !WebConfig.loadingbuildWeb) {

    //***Calculate JWS
    get_prayer_times(dtNow.year(), dtNow.month(), dtNow.day(), -7.3357650, 112.7669901, 7, JWSConfig.jamSholat);  //-7.3357650, 112.7669901 -7.2575, 112.7521

    for (int i = 0; i < 8; i++) {
      int pryHour, pryMnt;
      get_float_time_parts(JWSConfig.jamSholat[i], pryHour, pryMnt);
      snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour, pryMnt);
      //***Correct Time JWS & Assign Var Alarm
      if (i == 1) { 
        //***Info & Alarm Shubuh
        snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour + ((pryMnt + JWSConfig.loadCorrection[i]) / 60), (pryMnt + JWSConfig.loadCorrection[i]) % 60);
        AlarmConfig.jamSholat[i - 1] = pryHour + ((pryMnt + JWSConfig.loadCorrection[i]) / 60);
        AlarmConfig.menitSholat[i - 1] = (pryMnt + JWSConfig.loadCorrection[i]) % 60;

        //***Calculate Start Tarkhim
        calcSumMinutesTarkhim(pryHour, pryMnt, JWSConfig.loadCorrection[i], TarkhimConfig.tarkhimMinutes[i - 1], TarkhimConfig.tarkhimSeconds[i - 1], TarkhimConfig.sumMinutes[i - 1], TarkhimConfig.trigSecond[i - 1]);
        TarkhimConfig.trigMinute[i - 1] = TarkhimConfig.sumMinutes[i - 1] % 60;
        TarkhimConfig.trigHour[i - 1] = TarkhimConfig.sumMinutes[i - 1] / 60;
        TarkhimConfig.doneMinutes[i - 1] = ((pryHour * 60) + pryMnt);

      } else if (i >= 4 && i <= 7) { 
        //***Info & Alarm Dhuhur, Ashar, Maghrib, Isya
        snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour + ((pryMnt + JWSConfig.loadCorrection[i - 2]) / 60), (pryMnt + JWSConfig.loadCorrection[i - 2]) % 60);
        AlarmConfig.jamSholat[i - 3] = pryHour + ((pryMnt + JWSConfig.loadCorrection[i - 2]) / 60);
        AlarmConfig.menitSholat[i - 3] = (pryMnt + JWSConfig.loadCorrection[i - 2]) % 60;

        //***Calculate Start Tarkhim
        calcSumMinutesTarkhim(pryHour, pryMnt, JWSConfig.loadCorrection[i - 2], TarkhimConfig.tarkhimMinutes[i - 3], TarkhimConfig.tarkhimSeconds[i - 3], TarkhimConfig.sumMinutes[i - 3], TarkhimConfig.trigSecond[i - 3]);
        if(dtNow.dayOfTheWeek() == 5 && (i - 3) == 3){
          calcSumMinutesTarkhim(pryHour, pryMnt, JWSConfig.loadCorrection[i - 2], TarkhimConfig.tarkhimMinutes[i - 3], TarkhimConfig.tarkhimSeconds[i - 3], TarkhimConfig.sumMinutes[i - 3], TarkhimConfig.trigSecond[i - 3]);
        } else if(dtNow.dayOfTheWeek() == 6 && (i - 3) == 1){
          calcSumMinutesTarkhim(pryHour, pryMnt, JWSConfig.loadCorrection[i - 2], TarkhimConfig.tarkhimMinutes[i - 3], TarkhimConfig.tarkhimSeconds[i - 3], TarkhimConfig.sumMinutes[i - 3], TarkhimConfig.trigSecond[i - 3]);
        }
        TarkhimConfig.trigMinute[i - 3] = TarkhimConfig.sumMinutes[i - 3] % 60;
        TarkhimConfig.trigHour[i - 3] = TarkhimConfig.sumMinutes[i - 3] / 60;
        TarkhimConfig.doneMinutes[i - 3] = ((pryHour * 60) + pryMnt);
      }
    }

    //***Calculate Hijri & Java Dates
    get_hijri_date(dtNow.year(), dtNow.month(), dtNow.day(), JWSConfig.getHijriandPasar[2], JWSConfig.getHijriandPasar[1], JWSConfig.getHijriandPasar[0], &JWSConfig.nameHijriMonth, JWSConfig.loadCorrection[0]);
    get_pasaranjawa_date(dtNow.year(), dtNow.month(), dtNow.day(), JWSConfig.getHijriandPasar[3], &JWSConfig.nameJawaDay);

    //***Set alarm JWS
    BuzzerJWS.alarm(0, AlarmConfig.jamSholat[0], AlarmConfig.menitSholat[0], 0, 2, 250, 100, 1000, 1.5 * 60000);  //***Shubuh
    BuzzerJWS.alarm(1, AlarmConfig.jamSholat[1], AlarmConfig.menitSholat[1], 0, 4, 250, 100, 2000, 1.5 * 60000);  //***Dhuhur
    BuzzerJWS.alarm(2, AlarmConfig.jamSholat[2], AlarmConfig.menitSholat[2], 0, 4, 250, 100, 2000, 1.5 * 60000);  //***Ashar
    BuzzerJWS.alarm(3, AlarmConfig.jamSholat[3], AlarmConfig.menitSholat[3], 0, 3, 250, 100, 1500, 1.5 * 60000);  //***Maghrib
    BuzzerJWS.alarm(4, AlarmConfig.jamSholat[4], AlarmConfig.menitSholat[4], 0, 4, 250, 100, 2000, 1.5 * 60000);  //***Isya
    
    //***Set alarm
    for (int a = 0; a < 5; a++) {
      BuzzerJWS.alarm(a + 5, AlarmConfig.loadAlarmSet[a][0].toInt(), AlarmConfig.loadAlarmSet[a][1].toInt(), AlarmConfig.loadAlarmSet[a][2].toInt(),
                      AlarmConfig.loadAlarmSet[a][3].toInt(), AlarmConfig.loadAlarmSet[a][4].toFloat() * 1000, AlarmConfig.loadAlarmSet[a][5].toFloat() * 1000,
                      AlarmConfig.loadAlarmSet[a][6].toFloat() * 1000, AlarmConfig.loadAlarmSet[a][7].toFloat() * 60000, AlarmConfig.loadAlarmSet[a][8] == "1");
      //***baris 1 = index, jam, menit, detik,
      //***baris 2 = xBeep, tOn, tOff,
      //***baris 3 = longP, Duration, enabled
    }

    /*for(int i=0; i< 5; i++){
    Serial.print("Alarm ke-");
    Serial.print(i+1);
    Serial.print(" : ");
    Serial.print(AlarmConfig.loadAlarmSet[i][0]); Serial.print(":");
    Serial.print(AlarmConfig.loadAlarmSet[i][1]); Serial.print(":");
    Serial.print(AlarmConfig.loadAlarmSet[i][2]); Serial.print(",\tcnt= ");
    Serial.print(AlarmConfig.loadAlarmSet[i][3]); Serial.print("\tton= ");
    Serial.print(AlarmConfig.loadAlarmSet[i][4]); Serial.print("\ttoff= ");
    Serial.print(AlarmConfig.loadAlarmSet[i][5]); Serial.print("\tlongP= ");
    Serial.print(AlarmConfig.loadAlarmSet[i][6]); Serial.print("\tdur= ");
    Serial.print(AlarmConfig.loadAlarmSet[i][7]); Serial.print("\tactive= ");
    Serial.println(AlarmConfig.loadAlarmSet[i][8]); 
    Serial.print("Alarm Sholat ke-"); Serial.print(i+1); Serial.print("\t");  Serial.print(AlarmConfig.jamSholat[i]); Serial.print(":"); Serial.println(AlarmConfig.menitSholat[i]);    
    Serial.print("Correction "); Serial.print(i+1); Serial.print(". "); Serial.println(JWSConfig.loadCorrection[i]);
  }*/
    Serial.println("=========================================================================================================================================================");

    GlobalVar.forMillis[3] = millis();
  }

  //***Task 1s, Triggered Start Tarkhim
  if ((millis() - GlobalVar.forMillis[4]) >= 1000 && !WebConfig.loadingbuildWeb) {

    for (int t = 0; t < 5; t++) {
      
      // AccesFile.FileReadInt("/infoCorrection.txt", (t*2)+1, ";", TarkhimConfig.tarkhimMinutes[t]);
      // AccesFile.FileReadInt("/infoCorrection.txt", (t*2)+2, ";", TarkhimConfig.tarkhimSeconds[t]);

      if(dtNow.dayOfTheWeek() == 5 && t == 3){
        //***Day of The Week Kamis=5 Maghrib=3
        Serial.println("==");
        // AccesFile.FileReadInt("/infoCorrection.txt", 11, ";", TarkhimConfig.tarkhimMinutes[t]);
        // AccesFile.FileReadInt("/infoCorrection.txt", 12, ";", TarkhimConfig.tarkhimSeconds[t]);
      } else if(dtNow.dayOfTheWeek() == 6 && t == 1){
        //***Day of The Week Jumat=6 Dhuhur=1
        Serial.println("==");
        // AccesFile.FileReadInt("/infoCorrection.txt", 11, ";", TarkhimConfig.tarkhimMinutes[t]);
        // AccesFile.FileReadInt("/infoCorrection.txt", 12, ";", TarkhimConfig.tarkhimSeconds[t]);
      }

      if ((dtNow.hour() == TarkhimConfig.trigHour[t] && dtNow.minute() == TarkhimConfig.trigMinute[t] && dtNow.second() == TarkhimConfig.trigSecond[t]) && !TarkhimConfig.tarkhimTrig[t]) {
        //playdfPlayer(t+1); //file shubuh 001.mp3
        digitalWrite(16, 1);
        if(dtNow.dayOfTheWeek() == 5 && t == 3){
          Serial.println("==");
          //playdfPlayer(6); //file shubuh 001.mp3
        } else if(dtNow.dayOfTheWeek() == 6 && t == 1){
          Serial.println("==");
          //playdfPlayer(6); //file shubuh 001.mp3
        }
        TarkhimConfig.tarkhimTrig[t] = true;
      }

      if ((((dtNow.hour() * 60) + dtNow.minute()) == TarkhimConfig.doneMinutes[t]) && (dtNow.second() >= 10 && dtNow.second() <= 15)) {
        digitalWrite(16, 0);
        TarkhimConfig.tarkhimTrig[t] = false;
      }

      // Serial.print(t+1); Serial.print("\t");
      // Serial.print(TarkhimConfig.trigHour[t]); Serial.print(":"); Serial.print(TarkhimConfig.trigMinute[t]); Serial.print(":"); Serial.println(TarkhimConfig.trigSecond[t]);
    }
    
    GlobalVar.forMillis[4] = millis();
  }
}

//=============================================================================================================================================================
//=============================================================================================================================================================
//=============================================================================================================================================================
//***Additional Function

void calcSumMinutesTarkhim(int inHour, int inMinute, int Correct, int tMinute, int tSecond, uint16_t &rSum, uint8_t &rSec){
  
  int sumWCorrectMin  = (inMinute + Correct) % 60;
  int sumWCorrectHour = inHour + ((inMinute + Correct) / 60);

  if (tSecond>0) {
    rSum = (((sumWCorrectHour * 60) + sumWCorrectMin) - 1) - tMinute; 
    rSec = 60 - tSecond;
  } else {
    rSum = ((sumWCorrectHour * 60) + sumWCorrectMin) - tMinute;
    rSec = tSecond;
  }

  // TarkhimConfig.sumMinutes[i - 1] = (((pryHour * 60) + pryMnt) - 1) - TarkhimConfig.tarkhimMinutes[i - 1];
  // TarkhimConfig.trigSecond[i - 1] = 60 - TarkhimConfig.tarkhimSeconds[i - 1];
}



// //Task 1Sec
// if ((millis() - GlobalVar.forMillis[0]) >= 1000) {
//   dtNow = rtc.now();
//   if(!DisplayConfig.onSegment){
//     Display7Segment.displayTime(dtNow.hour(), dtNow.minute(), dtNow.second(), 0, SECOND_FIRST_RIGHT, 3, (((dtNow.second() % 10) % 2) == 0) ? false : true);
//   }

//   GlobalVar.forMillis[0] = millis();
// }

// //Task 10ms
// if ((millis() - GlobalVar.forMillis[1]) >= 10 ) {
//   server.handleClient();
//   Display7Segment.setBrightness(8, valuePWM());

//   //Animation Test
//   if(EDGEDetect.EDGEPOS(0, DisplayConfig.onSegment)){
//     DisplayConfig.playPattern++;
//     if(DisplayConfig.playPattern==9) {
//       DisplayConfig.playPattern = 1;
//     }
//   }
//   if(EDGEDetect.EDGENEG(1, DisplayConfig.onSegment) && DisplayConfig.playPattern==8){
//       BuzzerJWS.stopBuzzing();
//   }

//   if(DisplayConfig.onSegment){
//     if(DisplayConfig.playPattern==1) Display7Segment.turnOn(0);
//     if(DisplayConfig.playPattern==2) Display7Segment.animateRunning(LEFT_TO_RIGHT, 20);
//     if(DisplayConfig.playPattern==3) Display7Segment.animateRunning(RIGHT_TO_LEFT, 20);
//     if(DisplayConfig.playPattern==4) Display7Segment.waveEffect(50);
//     if(DisplayConfig.playPattern==5) Display7Segment.randomFlash(50);
//     if(DisplayConfig.playPattern==6) Display7Segment.pulseAll(200);
//     if(DisplayConfig.playPattern==7) Display7Segment.bounceEffect(50);
//     if(DisplayConfig.playPattern==8) BuzzerJWS.buzz(500, 500);
//   }

//   //Check Alarm
//   BuzzerJWS.checkAlarm(dtNow.hour(), dtNow.minute(), dtNow.second());

//   GlobalVar.forMillis[1] = millis();
// }

// //Task for Calculate JWS and pasaran day
// if ((millis() - GlobalVar.forMillis[2]) >= 1000) {
//   get_prayer_times(dtNow.year(), dtNow.month(), dtNow.day(), -7.3357650, 112.7669901, 7, JWSConfig.jamSholat); //-7.3357650, 112.7669901 -7.2575, 112.7521

//   for (int i=0; i<8; i++) {
//     int pryHour, pryMnt;
//     get_float_time_parts(JWSConfig.jamSholat[i], pryHour, pryMnt);
//     snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour, pryMnt);
//     //correct time jws & assign var alarm
//     if(i==1){                 //Shubuh
//       snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour+((pryMnt+JWSConfig.loadCorrection[i])/60), (pryMnt+JWSConfig.loadCorrection[i])%60);
//       AlarmConfig.jamSholat[i-1]    = pryHour+((pryMnt+JWSConfig.loadCorrection[i])/60);
//       AlarmConfig.menitSholat[i-1]  = (pryMnt+JWSConfig.loadCorrection[i])%60;
//     } else if(i>=4 && i<=7){  //Dhuhur, Ashar, Maghrib, Isya
//       snprintf(JWSConfig.PrayTimetoWeb[i], 16, "%s %02d:%02d", TimeName[i], pryHour+((pryMnt+JWSConfig.loadCorrection[i-2])/60), (pryMnt+JWSConfig.loadCorrection[i-2])%60);
//       AlarmConfig.jamSholat[i-3]    = pryHour+((pryMnt+JWSConfig.loadCorrection[i-2])/60);
//       AlarmConfig.menitSholat[i-3]  = (pryMnt+JWSConfig.loadCorrection[i-2])%60;
//     }
//   }

//   get_hijri_date(dtNow.year(),dtNow.month(), dtNow.day(), JWSConfig.getHijriandPasar[2], JWSConfig.getHijriandPasar[1], JWSConfig.getHijriandPasar[0], &JWSConfig.nameHijriMonth, JWSConfig.loadCorrection[0]);
//   get_pasaranjawa_date(dtNow.year(), dtNow.month(), dtNow.day(), JWSConfig.getHijriandPasar[3], &JWSConfig.nameJawaDay);

//   // Set alarm JWS
//   BuzzerJWS.alarm(0, AlarmConfig.jamSholat[0], AlarmConfig.menitSholat[0], 0, 2, 250, 100, 1000, 1.5*60000); //Shubuh
//   BuzzerJWS.alarm(1, AlarmConfig.jamSholat[1], AlarmConfig.menitSholat[1], 0, 4, 250, 100, 2000, 1.5*60000); //Dhuhur
//   BuzzerJWS.alarm(2, AlarmConfig.jamSholat[2], AlarmConfig.menitSholat[2], 0, 4, 250, 100, 2000, 1.5*60000); //Ashar
//   BuzzerJWS.alarm(3, AlarmConfig.jamSholat[3], AlarmConfig.menitSholat[3], 0, 3, 250, 100, 1500, 1.5*60000); //Maghrib
//   BuzzerJWS.alarm(4, AlarmConfig.jamSholat[4], AlarmConfig.menitSholat[4], 0, 4, 250, 100, 2000, 1.5*60000); //Isya
//   // Set alarm
//   for(int a=0; a<5; a++){
//       BuzzerJWS.alarm(a+5, AlarmConfig.loadAlarmSet[a][0].toInt(), AlarmConfig.loadAlarmSet[a][1].toInt(), AlarmConfig.loadAlarmSet[a][2].toInt(),
//                            AlarmConfig.loadAlarmSet[a][3].toInt(), AlarmConfig.loadAlarmSet[a][4].toFloat()*1000, AlarmConfig.loadAlarmSet[a][5].toFloat()*1000,
//                            AlarmConfig.loadAlarmSet[a][6].toFloat()*1000, AlarmConfig.loadAlarmSet[a][7].toFloat()*60000, AlarmConfig.loadAlarmSet[a][8] == "1");
//                       //baris 1 = index, jam, menit, detik,
//                       //baris 2 = xBeep, tOn, tOff,
//                       //baris 3 = longP, Duration, enabled
//   }

// /*for(int i=0; i< 5; i++){
//   Serial.print("Alarm ke-");
//   Serial.print(i+1);
//   Serial.print(" : ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][0]); Serial.print(":");
//   Serial.print(AlarmConfig.loadAlarmSet[i][1]); Serial.print(":");
//   Serial.print(AlarmConfig.loadAlarmSet[i][2]); Serial.print(",\tcnt= ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][3]); Serial.print("\tton= ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][4]); Serial.print("\ttoff= ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][5]); Serial.print("\tlongP= ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][6]); Serial.print("\tdur= ");
//   Serial.print(AlarmConfig.loadAlarmSet[i][7]); Serial.print("\tactive= ");
//   Serial.println(AlarmConfig.loadAlarmSet[i][8]);
//   Serial.print("Alarm Sholat ke-"); Serial.print(i+1); Serial.print("\t");  Serial.print(AlarmConfig.jamSholat[i]); Serial.print(":"); Serial.println(AlarmConfig.menitSholat[i]);
//   Serial.print("Correction "); Serial.print(i+1); Serial.print(". "); Serial.println(JWSConfig.loadCorrection[i]);
// }*/
//   Serial.println("=========================================================================================================================================================");
//   // BuzzerJWS.alarm(5, AlarmConfig.loadAlarmSet[0], AlarmConfig.loadAlarmSet[1], AlarmConfig.loadAlarmSet[2], 2, 250, 100, 1000, 1.5*60000);
//   // BuzzerJWS.alarm(6, AlarmConfig.loadAlarmSet[3], AlarmConfig.loadAlarmSet[4], AlarmConfig.loadAlarmSet[5], 4, 250, 100, 2000, 1.5*60000);
//   // BuzzerJWS.alarm(7, AlarmConfig.loadAlarmSet[6], AlarmConfig.loadAlarmSet[7], AlarmConfig.loadAlarmSet[8], 4, 250, 100, 2000, 1.5*60000);
//   // BuzzerJWS.alarm(8, AlarmConfig.loadAlarmSet[9], AlarmConfig.loadAlarmSet[10], AlarmConfig.loadAlarmSet[11], 3, 250, 100, 1500, 1.5*60000);
//   // BuzzerJWS.alarm(9, AlarmConfig.loadAlarmSet[12], AlarmConfig.loadAlarmSet[13], AlarmConfig.loadAlarmSet[14], 4, 250, 100, 2000, 1.5*60000);


//   GlobalVar.forMillis[2] = millis();
// }