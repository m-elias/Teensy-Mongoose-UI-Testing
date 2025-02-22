#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>
#include "mongoose_init.h"

const char* inoVersion = "AiO v5.0d Web GUI - " __DATE__ " " __TIME__;

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_inputs(&input_vars); before making changes
//  - call glue_set_inputs(&input_vars); after making changes
struct inputs input_vars;
struct misc misc_vars;

const uint16_t EE_IDENT = 2417; // change to force EE update
const uint16_t eeAddr = 0;

#include "inputs.h"

#include "LEDS.h"
LEDS LEDs = LEDS(1000, 255, 64, 127);   // 1000ms RGB update, 255/64/127 RGB brightness balance levels for v5.0a


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  //while (!Serial) delay(50);
  Serial.print("\r\n\n\n*********************\r\nStarting setup...\r\n");
  Serial.println(inoVersion);

  //MG_ERROR(("Hello1"));
  //MG_INFO(("Hello2"));
  //MG_DEBUG(("Hello3"));   // doesn't print in setup() with default debug settings
  //MG_VERBOSE(("Hello4"));  // doesn't print in setup() with default debug settings

  ethernet_init();
  mongoose_init();
  //mg_set_ip();  // causes Teensy to hang

  uint16_t eeIdentRead;
  EEPROM.get(eeAddr + 0, eeIdentRead);  // get eeprom ident, for checking if eeprom needs resetting
  uint16_t varSize = 0;
  EEPROM.get(eeAddr + 2, varSize);      // get size of struct, if Mongoose wizard has changed struct, reset to new defaults

  // if EE Ident OR the struct size does not match then overwrite with new defaults
  if (eeIdentRead != EE_IDENT || varSize != sizeof(input_vars)) {
    glue_get_inputs(&input_vars);
    EEPROM.put(eeAddr + 0, EE_IDENT);   // EEPROM.put() uses EEPROM.update() which only writes to EEPROM if the value !=
    EEPROM.put(eeAddr + 2, sizeof(input_vars));
    EEPROM.put(eeAddr + 4, input_vars);
    Serial.print("\r\n** EEPROM reset to new defaults **\r\n");
  } else {
    EEPROM.get(eeAddr + 4, input_vars);  // read the Settings saved in EEPROM to the global Settings struct
    Serial.print("Loaded settings from EEPROM\r\n");
    glue_set_inputs(&input_vars);
  }

  glue_get_misc(&misc_vars);
  strcpy(misc_vars.fversion, inoVersion);
  glue_set_misc(&misc_vars);

  initializeInputs();
  LEDs.init();
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::PWR_ON);
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::ETH_READY);
}

elapsedMillis kickoutTransitionTimer = 0;

void loop() {
  mongoose_poll();

  static elapsedMillis miscTimer = 0;
  if (miscTimer > 499) { // 2hz
    miscTimer -= 500;   // try to maintain steady 2hz
    glue_get_misc(&misc_vars); // pull-sync from UI

    //LEDs.setBrightness(int(float(misc_vars.rgbBrightness) / 2.55));
  }


  static elapsedMillis inputsTimer = 0;
  if (inputsTimer > 99) { // 10hz
    inputsTimer -= 100;   // try to maintain steady 10hz
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
  }



  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }




  if (Serial.available()) {
    uint8_t sRead = Serial.read();

    if (sRead == 's') {   // toggle Steer input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.steerEnabled = !input_vars.steerEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'k') {   // toggle Kickout input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.kickoutEnabled = !input_vars.kickoutEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'w') {   // toggle Work input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.workEnabled = !input_vars.workEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'i') {
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.workInvert = !input_vars.workInvert;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'h') {
      glue_get_inputs(&input_vars); // pull-sync from UI
      memcpy(input_vars.workHystStr, "6", 2);
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'd') {
      glue_start_set_work_digital();
    }
  }
}
