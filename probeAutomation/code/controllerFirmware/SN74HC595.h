#ifndef SN74HC595_H
#define SN74HC595_H

#include <Arduino.h>

class SN74HC595 {  
  private:
    byte _clockPin;
    byte _latchPin;
    byte _dataPin;
    byte _oePin;
    byte _clearPin;
    bool _msbf;
    
  public:
    SN74HC595(byte, byte, byte, byte, byte);// (clock pin, latch pin, data pin, OE pin, clear pin) default MSB first
    SN74HC595(byte, byte, byte, byte, byte, bool);// (clock pin, latch pin, data pin, OE pin, clear pin, LSB first?)
    void sendData(byte);
    void begin();
};

#endif