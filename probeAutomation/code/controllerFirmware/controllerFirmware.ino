/*
  UC, Santa Barbara
  Project: Wind tunnel Pitot placement automation
  This program: Read encoder input and write to serial
  (C) Dr. Trevor Marks, 2024

  08/24: Jog only

  09/24: Added ability to run ASCII program vai serial communication 
    Format "< $A??.??; $A??.??; ... >" 
      '<' File start delimiter.
      '$' Line start delimiter. 
      'A' should be either x or y or d  (case does not matter)
      ??.?? desired move-to position or dwell time (in seconds)
        Example $Y0.05 will move the machine along the y-axis to position 0.05 cm
        Example $X22.5 will move the machine along the x-axis to the postion 22.5 cm
        Example $D2.5 will pause the machine for 2.5 second then run next move
      ';' line end delimiter.
      '>' file end delimiter.
      Multiple lines are ok
        Example <$X0.01;$Y10.0;$D5;$Y10.1;>
          Machine move forward to x = 0.01, then imediately to y = 10.0 cm absolute, pause for 5 seconds, then move one millimeter to 10.1 cm absolute
        
    MATLAB Code: 
      serialportlist("available") % determine correct comport from printout of port list
      windtunnelPitotProbe = serialport("COMx",BAUD_RATE) % COMx: see above line. BAUD: see below, default 74880
      writeline(windtunnelPitotProbe,"<message>")
      % if data is available on the buffer, it can be read using
      read(windtunnelPitotProbe);          
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "ButtonClass.h"
#include "SN74HC595.h"
#include "PotBasedPosition.h"
#include "MY_STEPPER.h"

/*----------------------*/
/*--- DEBUG HANDLING ---*/
/*----------------------*/
#define debug_print // uncomment to print debug comments to serial monitor
// #define debug_pot // uncomment to print x and y resistance (pot) values to serial monitor
// #define debug_printXYposition // uncomment to print x and y posistions to serial monitor
#include "DebugUtils.h"  // must be placed BELOW the above define values!

/*-----------------------------*/
/*--- CONSTANT DECLERATIONS ---*/
/*-----------------------------*/
const int ANALOG_RESOLUTION = 12;
const byte MAX_INPUT_ARRAY = 255;  //Max number of commands that can be received
const unsigned long BAUD_RATE = 74880;
const unsigned long ENCODER_SAMPLE_TIME = 150;  // encoder sample rate, in milliseconds [ms]
const unsigned long UPDATE_DISPLAY_TIME = 200;  // alphanumberic display refresh rate, in milliseconds [ms]
const unsigned long MOTOR_TIMEOUT = 3 * 60 * 1000;

const byte ILLUM_XBUTTON = 130, ILLUM_YBUTTON = 66, ILLUM_RUNNING = 52, ILLUM_WAITING = 56, ILLUM_NO_PROGRAM = 24;

enum machine_mode { PROGRAM_MODE, JOG_MODE } machineMode;
enum program_state { RUNNING, WAITING } programState;
enum drive_axis { X_AXIS_ACTIVE, Y_AXIS_ACTIVE} activeAxis;

/*------------------------------------*/
/*--- GLOBAL VARIABLE DECLERATIONS ---*/
/*------------------------------------*/
volatile int encoderCount = 0;
float userDesiredPosition[MAX_INPUT_ARRAY];
char userActiveAxis[MAX_INPUT_ARRAY];
byte commandsIndex = 0;
bool isProgramLoaded = false;
float positionSetPoint = 0.0;

/*--------------------------*/
/*--- CLASS CONSTRUCTORS ---*/
/*--------------------------*/
Adafruit_AlphaNum4 xDisplay = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 yDisplay = Adafruit_AlphaNum4();

Button xSelButton(7), ySelButton(8), goButton(9), noGoButton(10), modeSwitch(11);//, xLimitSwitch(32), yLimitSwitch(29);

SN74HC595 shiftReg(4, 3, 1, 2, 5);  // (clock pin, latch pin, data pin) sending MSB first

STEPPER xMotor(34, 33, 35), yMotor(37, 36, 38);  // (Step pin, Direction pin, Enable pin)

// PotBasedPosition class constructor:
//   (Analog pin, Pot Value, Cal Slope [cm/kΩ], Cal Set Value [kΩ], Axis Limit [cm], analog bit resolution)
//   Note, Axis Limit is the distance limit from Forward- or Lowered-most position.
PotBasedPosition xPot(39, 2.5, 1.0, 0.0, 2.6, ANALOG_RESOLUTION);  // FOR TESTING
PotBasedPosition yPot(15, 2.5, 1.0, 0.0, 2.6, ANALOG_RESOLUTION);  // FOR TESTING
// PotBasedPosition xPot(39, 50.0, -1.2509, 40.06, 44.45, ANALOG_RESOLUTION); // EMPERICAL
// PotBasedPosition yPot(15, 50.0, -1.4838, 44.45, 27.90, ANALOG_RESOLUTION); // EMPERICAL

// Encoder Pins
const int CHAN_A = 30;
const int CHAN_B = 31;

/*------------------------------------*/
/*------------------------------------*/
/*---     FUNCTION DEFINITIONS     ---*/
/*------------------------------------*/
/*------------------------------------*/

/*-------------------*/
/*--- ENCODER ISR ---*/
/*-------------------*/
void encoderISR() {
  if (digitalRead(CHAN_B) == HIGH) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

/*------------------------------------*/
/*--- ALPHA NUMERIC DISPLAY UPDATE ---*/
/*------------------------------------*/
void updateDisplay(int errorCode) {
  int buffer[5];
  int factor = 10000;
  int tempVal = int(1000.0 * xPot.getPosition());

  for (int i = 0; i < 5; i++) {
    buffer[i] = tempVal / factor;
    tempVal = tempVal % factor;
    factor /= 10;
  }
  if (buffer[4] > 5) { buffer[3]++; }
  if (buffer[3] == 10) {
    buffer[3] = 0;
    buffer[2]++;
  }
  if (buffer[2] == 10) {
    buffer[2] = 0;
    buffer[1]++;
  }
  if (buffer[1] == 10) {
    buffer[1] = 0;
    buffer[0]++;
  }

  xDisplay.writeDigitAscii(0, 48 + buffer[0]);
  xDisplay.writeDigitAscii(1, 48 + buffer[1], true);
  xDisplay.writeDigitAscii(2, 48 + buffer[2]);
  xDisplay.writeDigitAscii(3, 48 + buffer[3]);

  printXY("X: ");
  printXY(buffer[0]);
  printXY(buffer[1]);
  printXY(".");
  printXY(buffer[2]);
  printXY(buffer[3]);
  printXY("cm Y: ");

  factor = 10000;
  tempVal = int(1000.0 * yPot.getPosition());
  for (int i = 0; i < 5; i++) {
    buffer[i] = tempVal / factor;
    tempVal = tempVal % factor;
    factor /= 10;
  }
  if (buffer[4] > 5) { buffer[3]++; }
  if (buffer[3] == 10) {
    buffer[3] = 0;
    buffer[2]++;
  }
  if (buffer[2] == 10) {
    buffer[2] = 0;
    buffer[1]++;
  }
  if (buffer[1] == 10) {
    buffer[1] = 0;
    buffer[0]++;
  }
  yDisplay.writeDigitAscii(0, 48 + buffer[0]);
  yDisplay.writeDigitAscii(1, 48 + buffer[1], true);
  yDisplay.writeDigitAscii(2, 48 + buffer[2]);
  yDisplay.writeDigitAscii(3, 48 + buffer[3]);

  printXY(buffer[0]);
  printXY(buffer[1]);
  printXY(".");
  printXY(buffer[2]);
  printXY(buffer[3]);
  printXYln("cm");

  switch (errorCode) {
    case 0:  // No error
      break;
    case 1:  // Out of Bounds
      xDisplay.writeDigitAscii(0, ' ');
      xDisplay.writeDigitAscii(1, 'O');
      xDisplay.writeDigitAscii(2, 'U');
      xDisplay.writeDigitAscii(3, 'T');
      yDisplay.writeDigitAscii(0, 'B');
      yDisplay.writeDigitAscii(1, 'N');
      yDisplay.writeDigitAscii(2, 'D');
      yDisplay.writeDigitAscii(3, 'S');
      break;
    case 2:  // No Program Loaded
      xDisplay.writeDigitAscii(0, ' ');
      xDisplay.writeDigitAscii(1, ' ');
      xDisplay.writeDigitAscii(2, 'N');
      xDisplay.writeDigitAscii(3, 'O');
      yDisplay.writeDigitAscii(0, 'P');
      yDisplay.writeDigitAscii(1, 'R');
      yDisplay.writeDigitAscii(2, 'G');
      yDisplay.writeDigitAscii(3, 'M');
      break;
    case 3:  // Motor Time Out (did not reach position in allotted time)
      xDisplay.writeDigitAscii(0, 'T');
      xDisplay.writeDigitAscii(1, 'I');
      xDisplay.writeDigitAscii(2, 'M');
      xDisplay.writeDigitAscii(3, 'E');
      yDisplay.writeDigitAscii(0, ' ');
      yDisplay.writeDigitAscii(1, 'O');
      yDisplay.writeDigitAscii(2, 'U');
      yDisplay.writeDigitAscii(3, 'T');
      break;
    default:  // Some other error -> see serial monitor?
      xDisplay.writeDigitAscii(0, 'O');
      xDisplay.writeDigitAscii(1, 'T');
      xDisplay.writeDigitAscii(2, 'H');
      xDisplay.writeDigitAscii(3, 'R');
      yDisplay.writeDigitAscii(0, ' ');
      yDisplay.writeDigitAscii(1, 'E');
      yDisplay.writeDigitAscii(2, 'R');
      yDisplay.writeDigitAscii(3, 'R');
      break;
  }
  xDisplay.writeDisplay();
  yDisplay.writeDisplay();

  printPOT("Resistance Values --> Xpot: ");
  printPOT(xPot.getPotValue());
  printPOT("  Ypot: ");
  printPOTln(yPot.getPotValue());
}

/*------------------------------------*/
/*--- PROCESS INCOMING SERIAL DATA ---*/
/*------------------------------------*/
bool processIncomingData() {
  static bool fileLoading = false;
  static bool record = false;
  static byte msgIndex = 0;
  static char msgVessel[16];

  bool messageError = false;
  bool parseLine = false;
  bool decimalDigitsPresent = false;
  float tempVal = 0.0;
  int decimalDigitCount = 0;

  char RxData = Serial.read();

  switch (RxData) {
    case '<':  // File Start delimiter
      if (isProgramLoaded) {
        Serial.println("** Warning: file already loaded");
        Serial.println("     To load a new file, press the 'NO GO' button twice (this deletes the old file)");
        messageError = true;
      } else {
        debugln("** LOADING FILE **");
        commandsIndex = 0;  // Global varialble
        msgIndex = 0;
        fileLoading = true;
      }
    break;
    case '>':  // File end delimiter
      if (fileLoading) {
        if (commandsIndex > 0) {
          Serial.println("** FILE LOADED **");
          Serial.print("   * Commands to be excuted: ");
          Serial.println(commandsIndex);
          Serial.println("   * Press the 'GO' button to run program");
          Serial.println("   * Press the 'NO GO' button to pause");
          shiftReg.sendData(ILLUM_WAITING);  // Green Light Activated!
          isProgramLoaded = true;            // Global Variable Change!
        } else {
          Serial.println("ERROR: no commands loaded");
          messageError = true;
        }
      } else {
        Serial.println("Error: expected < before >");
        messageError = true;
      }
      fileLoading = false;
    break;
    case '$':  // Line Start delimiter
      if (fileLoading) {
        if (record) {
          Serial.println("Error: line incorrectly terminated");
          messageError = true;
          record = false;
        } else {
          debugln(" Recording line");
          record = true;
          msgIndex = 0;
        }
      } else {
        Serial.println("Error: expected < before $");
        messageError = true;
      }
    break;
    case ';':  // Line end delimiter
      if (fileLoading) {
        if (record) {
          if (fileLoading && commandsIndex < MAX_INPUT_ARRAY) {
            record = false;
            parseLine = true;
          } else {
            Serial.print("ERROR: max file size of ");
            Serial.print(MAX_INPUT_ARRAY);
            Serial.println(" bytes exceeded!");
            messageError = true;
          }
        } else {
          Serial.println("Error: expected $ before ;");
          messageError = true;
        }
      } else {
        Serial.println("Error: expected < before ;");
      }
    break;
    default:
      if (fileLoading && record) {
        if (msgIndex > 16) {
          Serial.println("ERROR: line length exceeded");
          messageError = true;
        } else {
          msgVessel[msgIndex] = RxData;
          msgIndex++;
        }
      }
    break;
  }

  if (parseLine) {
    switch (msgVessel[0]) {
      case 'X': case 'x': // X or x
        userActiveAxis[commandsIndex] = 'X';  // global variable change
      break;
      case 'Y': case 'y': // Y or y
        userActiveAxis[commandsIndex] = 'Y';  // global variable change
      break;
      case 'D': case 'd': // D or d -- dwell
        userActiveAxis[commandsIndex] = 'D';  // global variable change
      break;
      default:
        Serial.println("ERROR: line formating - A");
        messageError = true;
      break;
    }
    for (int i = 1; i < msgIndex; i++) {
      switch (msgVessel[i]) {
        case '.':
          if (decimalDigitsPresent) {  // check for double '.'
            Serial.println("ERROR: line formating - DP");
            messageError = true;
          } else {
            decimalDigitsPresent = true;
            decimalDigitCount = 0;  // resets count
          }
        break;
        default:
          if (msgVessel[i] < '0' || msgVessel[i] > '9') {
            Serial.println("ERROR: NaN");
            messageError = true;
          } else {
            msgVessel[i] -= '0';
            tempVal *= 10.0;
            tempVal += float(msgVessel[i]);
            decimalDigitCount++;
          }
        break;
      }
    }
    if( ! messageError ){
      if (decimalDigitsPresent) {
        for (int i = 0; i < decimalDigitCount; i++ ){
          tempVal /= 10.0;
        }
      }
      userDesiredPosition[commandsIndex] = tempVal;  // global variable change
      commandsIndex++;                               // global variable change
      debug("   Line ");
      debug(commandsIndex);
      debug(" recorded ");
      debug(userActiveAxis[commandsIndex - 1]);
      debugln(userDesiredPosition[commandsIndex - 1]);
    }
  }
  return messageError;
}

/*-------------------------------------------*/
/*--- BUTTON POLLING AND SERVICE ROUTINES ---*/
/*-------------------------------------------*/
void pollButtonStates() {
  static int modeSwitchPriorState = LOW;
  static int goButtonPriorState = LOW;
  static int noGoButtonPriorState = LOW;
  static int xSelectButtonPriorState = LOW;
  static int ySelectButtonPriorState = LOW;
  static byte noGoButtonPressCounter = 0;

  int modeSwitchState = modeSwitch.getState();
  int xSelectButtonState = xSelButton.getState();
  int ySelectButtonState = ySelButton.getState();
  int goButtonState = goButton.getState();
  int noGoButtonState = noGoButton.getState();

  if (modeSwitchState != modeSwitchPriorState) {
    modeSwitchPriorState = modeSwitchState;
    if (xMotor.isActive()) { xMotor.release(); }
    if (yMotor.isActive()) { yMotor.release(); }
    switch (modeSwitchState) {
      case HIGH:
        shiftReg.sendData(ILLUM_XBUTTON);
        machineMode = JOG_MODE;
        activeAxis = X_AXIS_ACTIVE;
        positionSetPoint = xPot.getPosition();
        encoderCount = 0;
        break;
      case LOW:
        if (isProgramLoaded) {
          shiftReg.sendData(ILLUM_WAITING);
        } else {
          shiftReg.sendData(ILLUM_NO_PROGRAM);
        }
        machineMode = PROGRAM_MODE;
        programState = WAITING;
        noGoButtonPressCounter = 0;
        break;
    }
  }

  if (xSelectButtonState != xSelectButtonPriorState) {
    xSelectButtonPriorState = xSelectButtonState;
    if (xSelectButtonState == LOW && machineMode == JOG_MODE) {
      if (xMotor.isActive()) { xMotor.release(); }
      if (yMotor.isActive()) { yMotor.release(); }
      shiftReg.sendData(ILLUM_XBUTTON);
      positionSetPoint = xPot.getPosition();
      activeAxis = X_AXIS_ACTIVE;
      encoderCount = 0;
    }
  }

  if (ySelectButtonState != ySelectButtonPriorState) {
    ySelectButtonPriorState = ySelectButtonState;
    if (ySelectButtonState == LOW && machineMode == JOG_MODE) {
      if (xMotor.isActive()) { xMotor.release(); }
      if (yMotor.isActive()) { yMotor.release(); }
      shiftReg.sendData(ILLUM_YBUTTON);
      positionSetPoint = yPot.getPosition();
      activeAxis = Y_AXIS_ACTIVE;
      encoderCount = 0;
    }
  }

  if (goButtonState != goButtonPriorState) {
    goButtonPriorState = goButtonState;
    if (goButtonState == LOW && machineMode == PROGRAM_MODE) {
      shiftReg.sendData(ILLUM_RUNNING);
      programState = RUNNING;
      noGoButtonPressCounter = 0;
    }
  }

  if (noGoButtonState != noGoButtonPriorState) {
    noGoButtonPriorState = noGoButtonState;
    if (noGoButtonState == LOW && machineMode == PROGRAM_MODE) {
      if (xMotor.isActive()) { xMotor.release(); }
      if (yMotor.isActive()) { yMotor.release(); }
      programState = WAITING;
      if (isProgramLoaded) {
        shiftReg.sendData(ILLUM_WAITING);
        noGoButtonPressCounter++;
        if (noGoButtonPressCounter == 1) {
          Serial.println("   * Paused");
          Serial.println("   * Press the 'GO' button to resume");
          Serial.println("   * Press the 'NO GO' button again to delete current program");
        }
        if (noGoButtonPressCounter == 2) {
          Serial.println("** Program deleted **");
          shiftReg.sendData(ILLUM_NO_PROGRAM);
          isProgramLoaded = false;
          noGoButtonPressCounter = 0;
        }
      } else {
        shiftReg.sendData(ILLUM_NO_PROGRAM);
      }
    }
  }
}

/*------------------*/
/*--- SETUP LOOP ---*/
/*------------------*/
void setup() {
  analogReadResolution(ANALOG_RESOLUTION);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(BAUD_RATE);
  delay(10);
  debugln("Active:: Debug Comments");
  printPOTln("Active:: Print POT Values in kOhm");
  printXYln("Active:: Print X and Y position in cm");

  xDisplay.begin(0x70);
  delay(10);
  yDisplay.begin(0x71);
  delay(10);
  
  xDisplay.writeDigitAscii(0, 'S');
  xDisplay.writeDigitAscii(1, 'T');
  xDisplay.writeDigitAscii(2, 'R');
  xDisplay.writeDigitAscii(3, 'T');
  yDisplay.writeDigitAscii(0, ' ');
  yDisplay.writeDigitAscii(1, ' ');
  yDisplay.writeDigitAscii(2, 'U');
  yDisplay.writeDigitAscii(3, 'P');
  xDisplay.writeDisplay();
  yDisplay.writeDisplay();
  delay(1000);

  xSelButton.begin();
  ySelButton.begin();
  goButton.begin();
  noGoButton.begin();
  modeSwitch.begin();
//  xLimitSwitch.begin();
//  yLimitSwitch.begin();

  xMotor.begin();
  yMotor.begin();

  pinMode(CHAN_A, INPUT);
  pinMode(CHAN_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(CHAN_A), encoderISR, FALLING);

  shiftReg.begin();
  delay(10);
  shiftReg.sendData(0);
  delay(10);
  byte j = 1;
  for(int i = 0; i < 8; i++){
    shiftReg.sendData(j);
    j = j << 1;
    delay(500);
  }
  shiftReg.sendData(0);
  delay(30);
  shiftReg.sendData(255);
  delay(800);
  shiftReg.sendData(0);
  delay(500);

  encoderCount = 0;
}

/*-------------------------*/
/*--- +++ MAIN LOOP +++ ---*/
/*-------------------------*/
void loop() {
  static unsigned long pastEncoderReadTime = 0;
  static unsigned long pastPositionReadTime = 0;
  static unsigned long startMoveTime = 0;
  static unsigned long dwellEndTime = 0;
  static unsigned long displayTimeModifier = 1;
  static byte processingIndex = 0;
  static bool checkMoveTimer = false;
  static bool isMachineActive = false;
  static bool firstPassPrinting = true;
  static bool dwellActive = false;

  PotBasedPosition* feedbackPot;
  STEPPER* activeStepper;

  float currentPosition = 0.0;
  unsigned long currentTime = millis();

  /*--- check buttons & switches ---*/
  pollButtonStates();
  /*--- Check Serial Buffer ---*/
  if (Serial.available() > 0) {
    if (processIncomingData()) {
      updateDisplay(666);
      displayTimeModifier = 10;
    }
  }
  /*--- check dwell timer ---*/
  if( dwellActive && (currentTime > dwellEndTime) ){
    Serial.println(" end of dwell");
    dwellActive = false;
    firstPassPrinting = true;
    processingIndex++;
  }
  /*--- Update display ---*/
  if ((currentTime - pastPositionReadTime) > displayTimeModifier * UPDATE_DISPLAY_TIME) {
    updateDisplay(0);
    pastPositionReadTime += (displayTimeModifier * UPDATE_DISPLAY_TIME);
    displayTimeModifier = 1;
  }
  /*--- Update Machine ---*/
  switch (machineMode) {
    case JOG_MODE: 
      isMachineActive = true;
      if (currentTime - pastEncoderReadTime > ENCODER_SAMPLE_TIME) {
        pastEncoderReadTime += ENCODER_SAMPLE_TIME;
        positionSetPoint = positionSetPoint + 0.01 * float(encoderCount);
        encoderCount = 0;
      }
    break;
    case PROGRAM_MODE: 
      switch (programState) {
        case RUNNING:
          if (isProgramLoaded) {
            if (processingIndex == commandsIndex) {// check if at end of program
              Serial.println("** Program Complete **");
              Serial.println("** No Program Loaded **");
              shiftReg.sendData(ILLUM_NO_PROGRAM);
              processingIndex = 0;
              isProgramLoaded = false;
              isMachineActive = false;
              programState = WAITING;
            } else {
              positionSetPoint = userDesiredPosition[processingIndex];
              switch (userActiveAxis[processingIndex]) {
                case 'X':
                  activeAxis = X_AXIS_ACTIVE;
                break;
                case 'Y':
                  activeAxis = Y_AXIS_ACTIVE;
                break;
                case 'D':
                  if( !dwellActive ){ // check if already holding
                    dwellActive = true;
                    positionSetPoint *= 1000.0;
                    dwellEndTime = (unsigned long)positionSetPoint;
                    dwellEndTime += currentTime;
                  }
                break;
              }
              if( dwellActive ){
                isMachineActive = false;
              } else {
                isMachineActive = true;
              }
              if (firstPassPrinting) {
                firstPassPrinting = false;
                Serial.print("* Running line ");
                Serial.print(processingIndex + 1);
                Serial.print(" of ");
                Serial.print(commandsIndex);
                if( dwellActive ){
                  Serial.print("-> Dwell ");
                } else{ 
                  Serial.print("-> going to: ");
                }
                Serial.print(positionSetPoint);
                Serial.print("...");
              }
            }
          } else {
            Serial.println("No Program Loaded");
            shiftReg.sendData(ILLUM_NO_PROGRAM);
            updateDisplay(2);
            displayTimeModifier = 10;
            programState = WAITING;
          }
        break;
        case WAITING:
          isMachineActive = false;
          firstPassPrinting = true;
          if (!isProgramLoaded) {
            processingIndex = 0;
          }
        break;
      }
    break;
  }
  /*--- Run machine if appropriate ---*/
  if (isMachineActive) {
    switch (activeAxis) {
      case X_AXIS_ACTIVE:
        feedbackPot = &xPot;
        activeStepper = &xMotor;
      break;
      case Y_AXIS_ACTIVE:
        feedbackPot = &yPot;
        activeStepper = &yMotor;
      break;
    }
    currentPosition = feedbackPot->getPosition();
    /*--- Check position against machine limits ---*/
    if( currentPosition <= 0.010 || currentPosition >= feedbackPot->getLimit() ) {
      activeStepper->release();
      debugln("*** OUT OF BOUNDS ***");
      updateDisplay(1);
      displayTimeModifier = 10;
      positionSetPoint = currentPosition;
      if (isProgramLoaded) {
        processingIndex++;
        firstPassPrinting = true;
      }
      encoderCount = 0;
    }
    /*--- Check position against setpoint ---*/
    if( abs(positionSetPoint - currentPosition) > 0.0099 ) {
      if (!activeStepper->isActive()) {
        activeStepper->activate();
        startMoveTime = currentTime;
        checkMoveTimer = true;
      }
      if( positionSetPoint < currentPosition ) {
        activeStepper->step(FORWARD);
      } else {
        activeStepper->step(BACKWARD);
      }
    } else { // AT SET-POINT
      if (activeStepper->isActive()) { activeStepper->release(); }
      checkMoveTimer = false;
      if (isProgramLoaded) {
        Serial.println(" arrived (I think)");
        processingIndex++;
        firstPassPrinting = true;
      }
    }
    /*--- Check motor timeout ---*/
    if( checkMoveTimer ) {
      if (currentTime - startMoveTime > MOTOR_TIMEOUT) {
        activeStepper->release();
        Serial.println("***  MOTOR POSITION TIMEOUT  ***");
        updateDisplay(3);
        displayTimeModifier = 10;
        positionSetPoint = currentPosition;
        checkMoveTimer = false;
        encoderCount = 0;
        if( isProgramLoaded ) {
          processingIndex++;
          firstPassPrinting = true;
        }
      }
    }
  }
}  // END MAIN LOOP
