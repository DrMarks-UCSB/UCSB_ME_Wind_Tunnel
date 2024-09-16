#include "MY_STEPPER.h"

STEPPER::STEPPER(byte sp, byte dp, byte enbl) { 
  _stepPin = sp;
  _directionPin = dp;
  _enablePin = enbl;
  _enablePinUsed = true;
  _activeState = false;
}

void STEPPER::begin() {
  pinMode( _enablePin , OUTPUT );
  digitalWrite( _enablePin , HIGH );
  delay(10);
  pinMode( _stepPin, OUTPUT );
  pinMode( _directionPin, OUTPUT );
}

void STEPPER::activate() {
  digitalWrite( _enablePin , LOW );
  _activeState = true;
}

void STEPPER::release() {
  digitalWrite( _enablePin , HIGH );
  _activeState = false;
}

bool STEPPER::isActive(){
  return _activeState;
}

void STEPPER::step(stepperDirection direction){
  switch(direction){
    case FORWARD:
      digitalWrite(_directionPin, HIGH);
      break;
    case BACKWARD:
      digitalWrite(_directionPin, LOW);
      break;
  }
  digitalWrite( _stepPin, HIGH);
  delayMicroseconds(75);
  digitalWrite( _stepPin, LOW);
}