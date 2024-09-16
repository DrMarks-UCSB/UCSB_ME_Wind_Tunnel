#ifndef DEBUGUTILS_H
  #define DEBUGUTILS_H
  
  #ifdef debug_printXYposition
    #ifndef debug_serial
      #define debug_serial
    #endif
    #define printXY(x) Serial.print(x)
    #define printXYln(x) Serial.println(x)
  #else
    #define printXY(x)
    #define printXYln(x)
  #endif

  #ifdef debug_pot
    #ifndef debug_serial
      #define debug_serial
    #endif
    #define printPOT(x) Serial.print(x)
    #define printPOTln(x) Serial.println(x)
  #else
    #define printPOT(x)
    #define printPOTln(x)
  #endif

  #ifdef debug_print
    #ifndef debug_serial
      #define debug_serial
    #endif
    #define debug(x) Serial.print(x)
    #define debugln(x) Serial.println(x)
  #else
     #define debug(x)
   #  define debugln(x)
  #endif

  #ifdef debug_serial
     #define debug_begin(x) Serial.begin(x)
  #else
    #define debug_begin(x)
  #endif

#endif