#ifndef ShiftSegment_h
#define ShiftSegment_h

#include <Arduino.h>
#include <SPI.h>

enum CommType {
  SPI_ = 0,
  CUSTOM_ = 1
};

enum Direction {
  LEFT_TO_RIGHT = 0,
  RIGHT_TO_LEFT = 1
};

enum DisplayOrderTime {
	SECOND_FIRST_LEFT,
	HOUR_FIRST_LEFT,
	SECOND_FIRST_RIGHT,
	HOUR_FIRST_RIGHT
};

class ShiftSegment {
  public:
    ShiftSegment(CommType commType, uint8_t latchPin, uint8_t brightnessPin, uint8_t numICs);
    ShiftSegment(CommType commType, uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint8_t brightnessPin, uint8_t numICs);
    void setPatternType(bool isCommonAnode);
    void displayTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t startDisplay, DisplayOrderTime order, uint8_t icIndexSec=255, bool stateSec=false);
    void displayCustom(uint8_t number, uint8_t icIndex);
    void setDot(uint8_t icIndex, bool state);
    void setBrightness(uint8_t bitDepth, uint16_t value);
    // Fungsi animasi sederhana
	void turnOn(uint8_t icIndex);
    void animateRunning(Direction direction, uint16_t speedMs);
	void waveEffect(uint16_t speedMs);
	void randomFlash(uint16_t speedMs);
	void sweepBrightness(uint16_t speedMs);
	void pulseAll(uint16_t speedMs);
	void bounceEffect(uint16_t speedMs);

  private:
    uint8_t _dataPin;
    uint8_t _clockPin;
    uint8_t _latchPin;
    uint8_t _brightnessPin;
    uint8_t _numICs;
    bool _isCommonAnode;
    bool _useSPI;
    byte* _pattern;
    byte _displayData[30];
    static const byte _patternCA[11];
    static const byte _patternCC[11];
    void shiftData();
};

#endif
