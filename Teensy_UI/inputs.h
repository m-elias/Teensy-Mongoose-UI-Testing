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