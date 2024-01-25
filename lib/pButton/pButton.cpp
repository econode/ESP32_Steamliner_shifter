#include <pButton.h>

pButton::pButton(uint8_t pin, uint8_t pin_mode, uint8_t min_samples) {
    if( min_samples>8 ) min_samples = 8;
    if( min_samples<1 ) min_samples = 1;
    _pin = pin;
    _pin_mode = pin_mode;
    _shiftRegister = 0x00;
    _shiftRegisterMask = 0XFF >> (8 - min_samples);
    _lastPressedEpoch = 0;
    _pressTime = 0;
    _hasChanged = false;
    pinMode(_pin,_pin_mode);
    delay(5);
    _thisState = (bool) digitalRead(_pin);
    if( _thisState ){
        _shiftRegister = 0xFF;
    } else {
        _shiftRegister = 0x00;
    }
}

void pButton::poll(){
    // Button debounce through shift register
    // We have to have 8 consecutive reads for a state change based on a 10ms poll = 80ms debounce time.
    uint8_t shiftBefore = _shiftRegister;
    _shiftRegister = ( ( _shiftRegister << 1 ) | digitalRead(_pin) ) & _shiftRegisterMask;
    if( _shiftRegister == 0x00 && _thisState==true ){ // We've just been pressed
        _lastPressedEpoch = millis();
        _pressTime = 0;
        _hasChanged = true;
        _thisState = false;
    }
    if( _shiftRegister == _shiftRegisterMask && _thisState==false ){ // We've just been released
        _pressTime = (uint16_t) (millis() - _lastPressedEpoch);
        _hasChanged = true;
        _thisState = true;
    }
}

uint16_t pButton::pressTime(){
    uint16_t pressTime = _pressTime;
    _pressTime = 0;
    return pressTime;
}

bool pButton::hasChanged(){
    bool hasChanged = _hasChanged;
    _hasChanged = false;
    return hasChanged;
}

bool pButton::read(){
    return _thisState;
}