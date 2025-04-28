#include "ShiftSegment.h"

const byte ShiftSegment::_patternCA[11] = { 0x80, 0xF2, 0x48, 0x60, 0x32, 0x24, 0x04, 0xF0, 0x00, 0x20, 0x00 };
const byte ShiftSegment::_patternCC[11] = { 0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF, 0xFF };

//use SPI
ShiftSegment::ShiftSegment(CommType commType, uint8_t latchPin, uint8_t brightnessPin, uint8_t numICs) {
  _useSPI = (commType == SPI_);
  _latchPin = latchPin;
  _brightnessPin = brightnessPin;
  _numICs = numICs > 30 ? 30 : numICs;
  _isCommonAnode = true;
  _pattern = (byte*)_patternCA;

  pinMode(_latchPin, OUTPUT);
  pinMode(_brightnessPin, OUTPUT);
  digitalWrite(_latchPin, HIGH);
  analogWrite(_brightnessPin, 0);

  if (_useSPI) {
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
	pinMode(_latchPin, OUTPUT);  //karena pin 12 MISO digunakan sebagai Output
  }

  for (uint8_t i = 0; i < _numICs; i++) {
    _displayData[i] = _isCommonAnode ? 0xFF : 0x00;
  }
}


//use shifout
ShiftSegment::ShiftSegment(CommType commType, uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint8_t brightnessPin, uint8_t numICs) {
  _useSPI = (commType == SPI_);
  _dataPin = dataPin;
  _clockPin = clockPin;
  _latchPin = latchPin;
  _brightnessPin = brightnessPin;
  _numICs = numICs > 30 ? 30 : numICs;
  _isCommonAnode = true;
  _pattern = (byte*)_patternCA;

  if (!_useSPI) {
    pinMode(_dataPin, OUTPUT);
    pinMode(_clockPin, OUTPUT);
  }
  pinMode(_latchPin, OUTPUT);
  pinMode(_brightnessPin, OUTPUT);
  digitalWrite(_latchPin, HIGH);
  analogWrite(_brightnessPin, 0);

  for (uint8_t i = 0; i < _numICs; i++) {
    _displayData[i] = _isCommonAnode ? 0xFF : 0x00;
  }
}

void ShiftSegment::setPatternType(bool isCommonAnode) {
  _isCommonAnode = isCommonAnode;
  _pattern = isCommonAnode ? (byte*)_patternCA : (byte*)_patternCC;
}

void ShiftSegment::displayTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t startDisplay, DisplayOrderTime order, uint8_t icIndexSec, bool stateSec) {
  if (_numICs >= 6) {
	 //Seven segment yang terdekat dengan IN adalah Seven Segment no pertama
	//_displayData[0] adalah yang pertama
	if (order == SECOND_FIRST_LEFT){
		_displayData[0+startDisplay] = _pattern[second / 10];
		_displayData[1+startDisplay] = _pattern[second % 10];
		_displayData[2+startDisplay] = _pattern[minute / 10];
		_displayData[3+startDisplay] = _pattern[minute % 10];
		_displayData[4+startDisplay] = _pattern[hour / 10];
		_displayData[5+startDisplay] = _pattern[hour % 10];
	} else if (order == HOUR_FIRST_LEFT){
		_displayData[0+startDisplay] = _pattern[hour / 10];
		_displayData[1+startDisplay] = _pattern[hour % 10];
		_displayData[2+startDisplay] = _pattern[minute / 10];
		_displayData[3+startDisplay] = _pattern[minute % 10];
		_displayData[4+startDisplay] = _pattern[second / 10];
		_displayData[5+startDisplay] = _pattern[second % 10];
	} else if (order == SECOND_FIRST_RIGHT){
		_displayData[0+startDisplay] = _pattern[second % 10];
		_displayData[1+startDisplay] = _pattern[second / 10];
		_displayData[2+startDisplay] = _pattern[minute % 10];
		_displayData[3+startDisplay] = _pattern[minute / 10];
		_displayData[4+startDisplay] = _pattern[hour % 10];
		_displayData[5+startDisplay] = _pattern[hour / 10];
	} else {
		_displayData[0+startDisplay] = _pattern[hour % 10];
		_displayData[1+startDisplay] = _pattern[hour / 10];
		_displayData[2+startDisplay] = _pattern[minute % 10];
		_displayData[3+startDisplay] = _pattern[minute / 10];
		_displayData[4+startDisplay] = _pattern[second % 10];
		_displayData[5+startDisplay] = _pattern[second / 10];
	}
	if (icIndexSec < _numICs) {
		if (_isCommonAnode) {
		  if (stateSec) _displayData[icIndexSec] &= ~0x01;
		  else _displayData[icIndexSec] |= 0x01;
		} else {
		  if (stateSec) _displayData[icIndexSec] |= 0x01;
		  else _displayData[icIndexSec] &= ~0x01;
		}
    }
    shiftData();
  }
}
	

void ShiftSegment::displayCustom(uint8_t number, uint8_t icIndex) {
  if (icIndex < _numICs && number <= 10) {
    _displayData[icIndex] = _pattern[number];
    shiftData();
  }
}

void ShiftSegment::setDot(uint8_t icIndex, bool state) {
  if (icIndex < _numICs) {
    if (_isCommonAnode) {
      if (state) _displayData[icIndex] &= ~0x01;
      else _displayData[icIndex] |= 0x01;
    } else {
      if (state) _displayData[icIndex] |= 0x01;
      else _displayData[icIndex] &= ~0x01;
    }
    //shiftData();
  }
}

void ShiftSegment::setBrightness(uint8_t bitDepth, uint16_t value) {
  uint16_t maxValue = (bitDepth == 10) ? 1023 : 255;
  value = constrain(value, 0, maxValue);
  if (_isCommonAnode) {
    analogWrite(_brightnessPin, maxValue - value);
  } else {
    analogWrite(_brightnessPin, value);
  }
}

void ShiftSegment::turnOn(uint8_t icIndex) {
  if (icIndex == 0) {
    for (uint8_t i = 0; i < _numICs; i++) {
      _displayData[i] = _isCommonAnode ? 0x00 : 0xFF;
    }
  } else if (icIndex <= _numICs) {
    _displayData[icIndex - 1] = _isCommonAnode ? 0x00 : 0xFF;
  }
  shiftData();
}

// Animasi: Bit berjalan melalui semua IC
void ShiftSegment::animateRunning(Direction direction, uint16_t speedMs) {
  static uint16_t bitPos = 0; // Posisi bit dalam total bit (8 * _numICs)
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate >= speedMs) {
    // Reset semua bit
    for (uint8_t i = 0; i < _numICs; i++) {
      _displayData[i] = _isCommonAnode ? 0xFF : 0x00; // Matikan semua bit
    }

    // Hitung IC dan bit dalam IC dari bitPos
    uint8_t icIndex = bitPos / 8;
    uint8_t bitIndex = bitPos % 8;
    byte runningPattern = _isCommonAnode ? ~(1 << bitIndex) : (1 << bitIndex); // Bit menyala
    if (icIndex < _numICs) {
      _displayData[icIndex] = runningPattern;
    }
    shiftData();

    // Update posisi bit
    if (direction == LEFT_TO_RIGHT) {
      bitPos = (bitPos + 1) % (_numICs * 8); // Loop melalui semua bit
    } else {
      bitPos = (bitPos == 0) ? (_numICs * 8 - 1) : bitPos - 1; // Loop mundur
    }
    lastUpdate = millis();
  }
}

// Animasi: Mengisi bit secara bertahap melalui semua IC
void ShiftSegment::waveEffect(uint16_t speedMs) {
  static uint16_t bitPos = 0;
  static unsigned long lastUpdate = 0;
  static uint8_t segIndex = 0;
  
  if(millis() - lastUpdate >= speedMs){
	for (uint8_t i = 0; i < _numICs; i++) {
	_displayData[i] = _isCommonAnode ? 0xFF : 0x00;
	}
	_displayData[bitPos / 8] = _isCommonAnode ? ~(1 << ((bitPos + segIndex) % 8)) : (1 << ((bitPos + segIndex) % 8));
	shiftData();
	bitPos = (bitPos + 1) % (_numICs * 8);
	segIndex = (segIndex + 1) % 8;
	lastUpdate = millis();
  } 
}

// Animasi: Mengisi bit secara bertahap melalui semua IC
void ShiftSegment::randomFlash(uint16_t speedMs) {
  static uint16_t bitPos = 0;
  static unsigned long lastUpdate = 0;
  static uint8_t segIndex = 0;
  
  if(millis() - lastUpdate >= speedMs){
	for (uint8_t i = 0; i < _numICs; i++) {
	_displayData[i] = _isCommonAnode ? 0xFF : 0x00;
	}
	bitPos = random(_numICs * 8);
	segIndex = random(8);
	_displayData[bitPos / 8] = _isCommonAnode ? ~(1 << segIndex) : (1 << segIndex);
	shiftData();
	lastUpdate = millis();
  }
}

// Animasi: Mengisi bit secara bertahap melalui semua IC
void ShiftSegment::sweepBrightness(uint16_t speedMs) {
  static unsigned long lastUpdate = 0;
  static uint8_t brightnessLevel = 0;
  static bool isIncreasing = true;
	
  if(millis() - lastUpdate >= speedMs){
	for (uint8_t i = 0; i < _numICs; i++) {
	_displayData[i] = _isCommonAnode ? 0x00 : 0xFF;
	}
	if(isIncreasing){
	  brightnessLevel += 2;
	  if(brightnessLevel >= 255) isIncreasing = false;
	} else {
	  brightnessLevel -= 2;
	  if(brightnessLevel <= 0) isIncreasing = true;
	}
	setBrightness(brightnessLevel, 512);
	shiftData();
	lastUpdate = millis();
  } 
  
}

// Animasi: Mengisi bit secara bertahap melalui semua IC
void ShiftSegment::pulseAll(uint16_t speedMs) {
  static unsigned long lastUpdate = 0;
  static uint8_t brightnessLevel = 0;
  static bool isIncreasing = true;
  
  if(millis() - lastUpdate >= speedMs){
	for (uint8_t i = 0; i < _numICs; i++) {
	_displayData[i] = _isCommonAnode ? 0x00 : 0xFF;
	}
	if(isIncreasing){
	  brightnessLevel += 20;
	  if(brightnessLevel >= 255) isIncreasing = false;
	} else {
	  brightnessLevel -= 20;
	  if(brightnessLevel <= 0) isIncreasing = true;
	}
	setBrightness(brightnessLevel, 512);
	shiftData();
	lastUpdate = millis();
  } 
}

// Animasi: Mengisi bit secara bertahap melalui semua IC
void ShiftSegment::bounceEffect(uint16_t speedMs) {
  static uint16_t bitPos = 0;
  static unsigned long lastUpdate = 0;
  static int8_t directionFlag = 1;
  
  if(millis() - lastUpdate >= speedMs){
	for (uint8_t i = 0; i < _numICs; i++) {
	_displayData[i] = _isCommonAnode ? 0xFF : 0x00;
	}
	_displayData[bitPos / 8] = _isCommonAnode ? ~(1 << (bitPos % 8)) : (1 << (bitPos % 8));
	shiftData();
	bitPos += directionFlag;
	if(bitPos >= (_numICs * 8) - 1 || bitPos == 0) directionFlag = -directionFlag;
	lastUpdate = millis();
  }
  
}

void ShiftSegment::shiftData() {
  digitalWrite(_latchPin, LOW);
  if (_useSPI) {
    for (int8_t i = _numICs - 1; i >= 0; i--) {
      SPI.transfer(_displayData[i]);
    }
  } else {
    for (int8_t i = _numICs - 1; i >= 0; i--) {
      shiftOut(_dataPin, _clockPin, MSBFIRST, _displayData[i]);
    }
  }
  digitalWrite(_latchPin, HIGH);
}
