#include "PotBasedPosition.h"

PotBasedPosition::PotBasedPosition(byte aPin, float potVal, float calSlope, float calSetPoint, float axisLimit, int res){
  _potPin = aPin;
  _VALUE = potVal; // kΩ
  _AXIS_CAL_SLOPE = calSlope; // cm/kΩ
  _AXIS_CAL_POT_VAL = calSetPoint; // kΩ (pot value at full zero-position)
  _AXIS_LIMIT = axisLimit;
  _RESOLUTION = pow(2.0, float(res) ) - 1.0;
}

float PotBasedPosition::getPosition(){
  int temp = analogRead( _potPin );
  for(int i = 0; i < 4; i++){
    temp += analogRead(_potPin);
  }
  float val = float(temp) / 5.0;
  val *= _VALUE;
  val /= _RESOLUTION;
  val -= _AXIS_CAL_POT_VAL;
  val *= _AXIS_CAL_SLOPE;
  return val;
}

float PotBasedPosition::getPotValue(){
  float val = float( analogRead( _potPin ) );
  val *= _VALUE;
  val /= _RESOLUTION;
  return val;
}

float PotBasedPosition::getLimit(){
  return _AXIS_LIMIT;
}