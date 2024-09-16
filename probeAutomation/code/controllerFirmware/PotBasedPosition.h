#ifndef POTBASEDPOSITION_H
#define POTBASEDPOSITION_H

#include <Arduino.h>

class PotBasedPosition{  
  private:
    byte _potPin;
    float _VALUE; // kΩ
    float _AXIS_CAL_SLOPE; // cm/kΩ
    float _AXIS_CAL_POT_VAL; // kΩ (pot value at full zero-position)
    float _AXIS_LIMIT;
    float _RESOLUTION;
    
  public:
    PotBasedPosition(byte, float, float, float, float, int);// Analog pin, Pot Value, Cal Slope, Cal Set Value, Axis Limit, analog resolution
    float getPosition();
    float getPotValue();
    float getLimit();
};

#endif