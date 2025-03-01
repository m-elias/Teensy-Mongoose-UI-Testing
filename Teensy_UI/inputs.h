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

  struct inputs input_vars;
  glue_get_inputs(&input_vars); // pull-sync from UI

  if (analogRead(WAS_A_PIN) < 20) { // means no WAS connected, also checked in loop() inputsTimer
    input_vars.steerEnabled = 0;
  } else {
    input_vars.steerEnabled = 1;
  }
  Serial.printf("Steer Input %s\r\n", (input_vars.steerEnabled ? "Enabled" : "*Disabled!* - check your wiring & settings"));

  if (1>2) {  // replace with conditional expr to disable work input UI elements appropriately
    input_vars.workEnabled = 0;
  } else {
    input_vars.workEnabled = 1;
  }
  Serial.printf("Work Input %s\r\n", (input_vars.workEnabled ? "Enabled" : "*Disabled!* - check your wiring & settings"));

  if (1>2) {  // replace with conditional expr to disable kickout input UI elements appropriately
    input_vars.kickoutEnabled = 0;
  } else {
    input_vars.kickoutEnabled = 1;
  }
  Serial.printf("Kickout Input %s\r\n", (input_vars.kickoutEnabled ? "Enabled" : "*Disabled!* - check your wiring & settings"));
  glue_set_inputs(&input_vars); // push-sync to UI
}

void checkInputsTimer()
{
  static elapsedMillis inputsTimer = 0;
  if (inputsTimer > 99) { // 10hz
    inputsTimer -= 100;   // try to maintain steady 10hz
    struct inputs input_vars;
    glue_get_inputs(&input_vars); // pull-sync from UI

    // **** read analog WORK input ****
    float read = float(analogRead(WAS_A_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)
    
    if (read < 3.0 || read > 97.0) {  // detect disconnected sensor
      input_vars.workEnabled = 0;
    } else {
      input_vars.workEnabled = 1;
    }
    //Serial.printf("Steer Enabled: %i\r\n", input_vars.steerEnabled);

    if (input_vars.workInvert) read = 100 - read;  // option to invert input polarity

    input_vars.workHystVal = atoi(input_vars.workHystStr);
    static bool workState = 0;
    if (read < max(input_vars.workThres - input_vars.workHystVal, 10)) {  // limit to >10% (0.33V)
      workState = LOW;
    }
    if (read > min(input_vars.workThres + input_vars.workHystVal, 90)) { // limit to <90% (2.97V)
      workState = HIGH;
    }

    input_vars.workState = workState;
    //input_vars.work_state = (!workState != !input_vars.work_invert);  // 2nd option to invert, XOR the work state logic with invert setting
    
    input_vars.workInput = max(min(round(read), 100), 0);  // limit betwwen 0 - 100

    // **** read digital STEER input ****
    input_vars.steerState = !digitalRead(STEER_PIN);
    

    // **** read digital KICKOUT input ****
    static elapsedMillis kickoutTransitionTimer = 0;
    if (input_vars.kickoutState == digitalRead(KICKOUT_D_PIN)){ // detect change
      kickoutTransitionTimer = 0;
      //memcpy(input_vars.kickoutStateColor, "#FFA500", 7); // set temporary orange color
      input_vars.kickoutStateHist = 2;  // set temp orange color
      input_vars.kickoutState = !input_vars.kickoutState;
    }

    if (kickoutTransitionTimer > 2000) {
      input_vars.kickoutStateHist = input_vars.kickoutState;
      /*if (input_vars.kickoutState == HIGH) {
        memcpy(input_vars.kickoutStateColor, "#22c55e", 7);
      } else {
        memcpy(input_vars.kickoutStateColor, "#ef4444", 7);
      }*/
    }

    glue_set_inputs(&input_vars); // push-sync to UI
    glue_update_state();  // triggers UI update, max 1hz regardless of Wizard setting?
  } // end inputs timer
}