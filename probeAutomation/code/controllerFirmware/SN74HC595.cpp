#include "SN74HC595.h"

SN74HC595::SN74HC595(byte cp, byte lp, byte dp, byte oe, byte clr) { // (clock pin, latch pin, data pin, true if LSB First (default is false, i.e., MSB First)
  _clockPin = cp;
  _latchPin = lp;
  _dataPin = dp;
  _oePin = oe;
  _clearPin = clr;
  _msbf = true;
}

SN74HC595::SN74HC595(byte cp, byte lp, byte dp, byte oe, byte clr, bool lsbF) { // (clock pin, latch pin, data pin, true if LSB First (default is false, i.e., MSB First)
  _clockPin = cp;
  _latchPin = lp;
  _dataPin = dp;
  _oePin = oe;
  _clearPin = clr;
  if( lsbF ){_msbf = false;} else {_msbf = true;}
}

void SN74HC595::begin() {
  pinMode( _latchPin, OUTPUT );
  pinMode( _clockPin, OUTPUT );
  pinMode( _dataPin, OUTPUT );
  pinMode( _oePin, OUTPUT );
  pinMode( _clearPin, OUTPUT );
  delay(10);
  digitalWrite(_latchPin,HIGH);
  digitalWrite(_clearPin, HIGH);
  digitalWrite(_oePin, LOW);
}

void SN74HC595::sendData(byte data){
// data is byte to be sent to 74HC595
  digitalWrite( _latchPin, LOW );
  if( _msbf ){
    shiftOut( _dataPin, _clockPin, MSBFIRST, data );
  } else {
    shiftOut( _dataPin, _clockPin, LSBFIRST, data );
  }
  digitalWrite(_latchPin, HIGH);
}