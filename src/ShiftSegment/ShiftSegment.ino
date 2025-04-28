#include <SevenSegment595.h>

// Display 1: SPI
SevenSegment595 display1(SPI_, 15, 0, 6); // latch=D8, brightness=D0, 6 IC

// Display 2: CUSTOM
SevenSegment595 display2(CUSTOM_, 2, 3, 4, 5, 6); // data=D2, clock=D3, latch=D4, brightness=D1, 6 IC

void setup() {
  display1.setPatternType(true); // CA
  display2.setPatternType(true); // CA
  display1.setBrightness(10, 512); // Setengah terang
  display2.setBrightness(10, 256); // Seperempat terang
}

void loop() {
  display1.displayTime(12, 34, 56); // Display 1: SPI
  display2.displayTime(23, 45, 10); // Display 2: CUSTOM

  // Dot berkedip pada display 2
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis >= 1000) {
    static bool dotState = false;
    display2.setDot(2, dotState);
    dotState = !dotState;
    lastMillis = millis();
  }

// Jika trig true, nyalakan semua segmen
  if (trig) {
    display.turnOn(0); // Nyalakan semua segmen
    delay(1000);       // Tampilkan selama 1 detik
  }
  // Tidak perlu else, setelah delay selesai loop akan kembali ke displayTime
}
