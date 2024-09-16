#include "ButtonClass.h"

Button::Button(byte p) {
  _pin = p;
  _debounceDelay = 30;
  _lastReading = HIGH;
  _state = HIGH;
}

Button::Button(byte p, unsigned long t) {
  _pin = p;
  _debounceDelay = t;
  _lastReading = HIGH;
  _state = HIGH;
}

void Button::begin() {
  pinMode(_pin, INPUT_PULLUP);
  delay(10);
  update();
}

void Button::update() {
    byte newReading = digitalRead(_pin);
    if( newReading != _lastReading ){
      _lastDebounceTime = millis();
    }
    if( millis() - _lastDebounceTime > _debounceDelay ) {
      _state = newReading;
    }
    _lastReading = newReading;
}

byte Button::getState() {
  update();
  return _state;
}