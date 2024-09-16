#ifndef MY_STEPPER_H
#define MY_STEPPER_H

#include <Arduino.h>

enum stepperDirection {FORWARD,BACKWARD};

class STEPPER {  
  private:
    byte _stepPin;
    byte _directionPin;
    byte _enablePin;
    bool _enablePinUsed;
    bool _activeState;
    
  public:
    STEPPER(byte, byte, byte);// (Step pin, Direction pin, Enable pin)
    void step(stepperDirection);
    void begin();
    void activate();
    void release();
    bool isActive();
};

#endif