#ifndef ezButton_h
#define smartButton_h
#include <Arduino.h>

class smartButton
{
	private:
        uint8_t _pin;
        uint8_t _GPIOmode;
        uint16_t _minimumTime;
        volatile bool _hasChanged;
	volatile unsigned long _lastTime;
        volatile unsigned long _lastPressmSeconds;
        volatile bool _lastPressRead;
        bool _thisState;
        void IRAM_ATTR buttonIsr();

	public:
        smartButton(uint8_t pin, uint8_t GPIOmode, uint16_t minimumTimeMs);
        uint16_t pressTime();
        bool hasChanged();
        bool read();
        volatile uint32_t changeCount = 0;
        void begin();
};

#endif