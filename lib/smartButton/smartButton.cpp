#include <smartButton.h>
#include "FunctionalInterrupt.h"

smartButton::smartButton(uint8_t pin, uint8_t GPIOmode, uint16_t minimumTimeMs) {
    _pin = pin;
    _GPIOmode = GPIOmode;
    _minimumTime = minimumTimeMs;
}

void smartButton::begin(){
    _hasChanged = false;
    pinMode(_pin,_GPIOmode);
    _thisState = digitalRead(_pin);
    _lastTime = millis();
    _lastPressmSeconds = 0;
    _lastPressRead = false;
  	attachInterrupt(digitalPinToInterrupt(_pin), std::bind(&smartButton::buttonIsr, this), CHANGE);
}

uint16_t smartButton::pressTime(){
    if( _lastPressRead==true ){
        _lastPressRead=false;
        return (uint16_t) _lastPressmSeconds;
    }
    return 0;
}

void IRAM_ATTR smartButton::buttonIsr() {
    bool thisRead = digitalRead(_pin);
    unsigned long now = millis();
    _thisState = thisRead;
    changeCount++;
    _hasChanged = true;
	if( thisRead == LOW ){
        // Begin of button press
        _lastPressRead = false;
        _lastPressmSeconds = 0;
        _lastTime = now;
        return;
    }
    unsigned long pressedTime = now - _lastTime;
    if( pressedTime > _minimumTime ){
        _lastPressRead = true;
        _lastPressmSeconds = pressedTime;
        return;
    }
    // Bounce
    _lastPressRead = false;
    _lastPressmSeconds = 0;
    return;
}

bool smartButton::hasChanged(){
    bool tempHasChanged = _hasChanged;
    _hasChanged = false;
    return tempHasChanged;
}

bool smartButton::read(){
    return _thisState;
}