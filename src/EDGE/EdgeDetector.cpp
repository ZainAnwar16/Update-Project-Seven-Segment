#include "EdgeDetector.h" // Sertakan header library

EDGEDTC::EDGEDTC(int maxPins){
	_maxPins = maxPins;
	_previousStates = new bool[maxPins]();
}

bool EDGEDTC::EDGEPOS(int pin, bool signal) {
  if (pin >= 0 && pin < _maxPins) { // Cek batas
    bool result = signal && !_previousStates[pin]; // Rising edge: sekarang true, sebelumnya false
    _previousStates[pin] = signal; // Update status sebelumnya
    return result;
  }
  return false; // Jika pin di luar batas, kembalikan false
}

bool EDGEDTC::EDGENEG(int pin, bool signal) {
 if (pin >= 0 && pin < _maxPins) { // Cek batas
    bool result = !signal && _previousStates[pin]; // Rising edge: sekarang true, sebelumnya false
    _previousStates[pin] = signal; // Update status sebelumnya
    return result;
  }
  return false; // Jika pin di luar batas, kembalikan false
}

bool EDGEDTC::EDGE(int pin, bool signal) {
  if (pin >= 0 && pin < _maxPins) { // Cek batas
    bool result = (signal != _previousStates[pin]); // Rising edge: sekarang true, sebelumnya false
    _previousStates[pin] = signal; // Update status sebelumnya
    return result;
  }
  return false; // Jika pin di luar batas, kembalikan false
}