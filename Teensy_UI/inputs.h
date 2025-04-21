#include "pins_arduino.h"
#include "core_pins.h"
#define WAS_A_PIN       A15     // WAS input
#define STEER_PIN         2     // STEER input
#define WORK_A_PIN      A17     // WORK input
#define KICKOUT_D_PIN     3     // REMOTE input
#define CURRENT_PIN     A13     // CURRENT sense input from on board DRV8701
#define KICKOUT_A_PIN   A12     // PRESSURE input

void initializeInputs(){
  pinMode(WAS_A_PIN, INPUT_DISABLE);
  pinMode(WORK_A_PIN, INPUT_DISABLE);
  pinMode(STEER_PIN, INPUT_PULLUP);
  pinMode(KICKOUT_D_PIN, INPUT_PULLUP);

  struct inputs input_temp;
  glue_get_inputs(&input_temp); // pull-sync from UI

  if (analogRead(WAS_A_PIN) < 20) { // means no WAS connected, also checked in loop() inputsTimer
    input_temp.steerState = 0;
  } else {
    input_temp.steerState = 1;
  }
  Serial.printf("Steer Input %s\r\n", (input_temp.steerState ? "Enabled" : "*Disabled!* - check your wiring & settings"));

  if (1>2) {  // replace with conditional expr to disable work input UI elements appropriately
    input_temp.workState = 3;
  } else {
    input_temp.workState = 0;
  }
  //Serial.printf("Work Input %s\r\n", (input_temp.workState ? "Enabled" : "*Disabled!* - check your wiring & settings"));

  if (1>2) {  // replace with conditional expr to disable kickout input UI elements appropriately
    input_temp.kickoutState = 3;
  } else {
    input_temp.kickoutState = 0;
  }
  Serial.printf("Kickout Input %s\r\n", (input_temp.kickoutState ? "Enabled" : "*Disabled!* - check your wiring & settings"));
  glue_set_inputs(&input_temp); // push-sync to UI
  glue_update_state();
}

void checkInputsTimer()
{
  static elapsedMillis inputsTimer = 0;
  if (inputsTimer > 99) { // 10hz
    inputsTimer -= 100;   // try to maintain steady 10hz
    LEDs.queueBlueFlash(LED_ID::GPS);
    struct inputs input_temp;
    glue_get_inputs(&input_temp); // pull-sync from UI

    // **** read analog WORK input ****
    float read = float(analogRead(WAS_A_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)
    
    if (read < 3.0 || read > 97.0) {  // detect disconnected sensor
      input_temp.workState = 3;
    } else {
      input_temp.workState = 0;
    }
    //Serial.printf("Steer Enabled: %i\r\n", input_temp.steerState);

    //if (input_temp.workInvert) read = 100 - read;  // option to invert input polarity

    input_temp.workHystVal = atoi(input_temp.workHystStr);
    static bool workState = 0;
    if (read < max(input_temp.workThres - input_temp.workHystVal, 10)) {  // limit to >10% (0.33V)
      workState = LOW;
    }
    if (read > min(input_temp.workThres + input_temp.workHystVal, 90)) { // limit to <90% (2.97V)
      workState = HIGH;
    }

    //input_temp.workState = workState;
    input_temp.workState = (!workState != !input_temp.workInvert);  // 2nd option to invert, XOR the work state logic with invert setting
    
    input_temp.workInput = max(min(round(read), 100), 0);  // limit betwwen 0 - 100

    // **** read digital STEER input ****
    input_temp.steerState = !digitalRead(STEER_PIN);
    

    // **** read digital KICKOUT input ****
    static elapsedMillis kickoutTransitionTimer = 0;
    if (input_temp.kickoutState == digitalRead(KICKOUT_D_PIN)){ // detect change (input state is inverted from variable)
      kickoutTransitionTimer = 0;
      input_temp.kickoutStateHist = 2;  // set temp color to show there was a recent change in state
      input_temp.kickoutState = !input_temp.kickoutState;
    }

    if (kickoutTransitionTimer > 2000) {
      input_temp.kickoutStateHist = input_temp.kickoutState;
    }

    glue_set_inputs(&input_temp); // push-sync to UI
    glue_update_state();  // triggers UI update, max 1hz regardless of Wizard setting?
  } // end inputs timer
}