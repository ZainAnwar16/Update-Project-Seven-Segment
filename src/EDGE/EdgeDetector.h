#ifndef EDGEDETECTOR_H
#define EDGEDETECTOR_H

#include <Arduino.h>

class EDGEDTC {
	public:
		EDGEDTC(int maxPins);
		bool EDGEPOS(int pin, bool signal); // Fungsi dengan pin sebagai parameter
		bool EDGENEG(int pin, bool signal); // Fungsi dengan pin sebagai parameter
		bool EDGE(int pin, bool signal); // Fungsi dengan pin sebagai parameter
	private:
		int _maxPins;
		bool* _previousStates;
};
	
#endif