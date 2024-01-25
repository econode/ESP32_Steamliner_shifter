#ifndef pButton_h
#define pButton_h
#include <Arduino.h>

class pButton
{
	private:
        uint8_t _pin;
        uint8_t _pin_mode;
        uint8_t _shiftRegisterMask;
        volatile uint8_t _shiftRegister;
        volatile bool _hasChanged;
	volatile unsigned long _lastPressedEpoch;
        volatile uint16_t _pressTime;
        volatile bool _thisState;

	public:
        pButton(uint8_t pin, uint8_t pin_mode, uint8_t min_samples);
        void poll();
        bool read();
        bool hasChanged();
        uint16_t pressTime();
};

#endif