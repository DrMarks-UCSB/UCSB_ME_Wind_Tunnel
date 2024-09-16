#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
  
  private:
    byte _pin;
    byte _state;
    byte _lastReading;
    
    unsigned long _debounceDelay;
    unsigned long _lastDebounceTime = 0;
    void update();
    
  public:
    Button(byte);
    Button(byte, unsigned long);
    
    void begin();
    byte getState();
};

#endif