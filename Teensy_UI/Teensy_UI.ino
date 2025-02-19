#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>
#include "mongoose_init.h"

const char* inoVersion = "AiO v5.0d Web GUI - " __DATE__ " " __TIME__;

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_inputs(&input_vars); before making changes
//  - call glue_set_inputs(&input_vars); after making changes
struct inputs input_vars;/* = {
  false,  // bool steer_state
  false,  // bool work_state
  0,      // int work_input
  50,     // int work_thres
  "18",   // char work_hyst[3]
  18,     // int work_hyst_int
  true,   // bool work_invert
  false,  // bool kickout_state
  "1 - AOG Setting" // char kickout_mode[30]
};*/

struct misc misc_vars;/* = {
  false,            // bool update
  "AiO GUI v5.old"  // char fversion[40]
};*/

const uint16_t EE_IDENT = 2415; // change to force EE update
const uint16_t eeAddr = 0;

#define WAS_A_PIN       A15     // WAS input
#define STEER_PIN         2     // STEER input
#define WORK_A_PIN      A17     // WORK input
#define KICKOUT_D_PIN     3     // REMOTE input
#define CURRENT_PIN     A13     // CURRENT sense input from on board DRV8701
#define KICKOUT_A_PIN   A12     // PRESSURE input

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WORK_A_PIN, INPUT_DISABLE);
  pinMode(STEER_PIN, INPUT_PULLUP);
  pinMode(KICKOUT_D_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  //while (!Serial) delay(50);
  Serial.print("\r\n\n\n*********************\r\nStarting setup...\r\n");

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
    Serial.print("\r\n- ** EEPROM reset to new defaults **\r\n");
  } else {
    EEPROM.get(eeAddr + 4, input_vars);  // read the Settings saved in EEPROM to the global Settings struct
    Serial.print("\r\n- loaded settings from EEPROM\r\n");
    glue_set_inputs(&input_vars);
  }

  glue_get_misc(&misc_vars);
  strcpy(misc_vars.fversion, inoVersion);
  glue_set_misc(&misc_vars);
}

elapsedMillis kickoutTransitionTimer = 0;

void loop() {
  mongoose_poll();

  static elapsedMillis workTimer = 0;
  if (workTimer > 99) { // 10hz
    //uint32_t t1 = micros();
    workTimer -= 100;   // try to maintain steady 10hz
    static bool workState = 0;
    float read = float(analogRead(WAS_A_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)

    glue_get_inputs(&input_vars); // pull-sync from UI

    if (input_vars.workInvert) read = 100 - read;  // option to invert input polarity

    input_vars.workHystVal = atoi(input_vars.workHystStr);
    if (read < max(input_vars.workThres - input_vars.workHystVal, 10)) {  // limit to >10% (0.33V)
      workState = LOW;
    }
    if (read > min(input_vars.workThres + input_vars.workHystVal, 90)) { // limit to <90% (2.97V)
      workState = HIGH;
    }

    input_vars.workState = workState;
    //input_vars.work_state = (!workState != !input_vars.work_invert);  // 2nd option to invert, XOR the work state logic with invert setting
    
    input_vars.workInput = max(min(round(read), 100), 0);  // limit betwwen 0 - 100

    input_vars.steerState = !digitalRead(STEER_PIN);
    
    if (input_vars.kickoutState == digitalRead(KICKOUT_D_PIN)){
      kickoutTransitionTimer = 0;
      memcpy(input_vars.kickoutStateColor, "#FFA500", 7);
      input_vars.kickoutState = !input_vars.kickoutState;
    }

    if (kickoutTransitionTimer > 2000) {
      if (input_vars.kickoutState == HIGH) {
        memcpy(input_vars.kickoutStateColor, "#22c55e", 7);
      } else {
        memcpy(input_vars.kickoutStateColor, "#ef4444", 7);
      }
    }


    


    glue_set_inputs(&input_vars); // push-sync to UI
    glue_update_state();  // triggers UI update, max 1hz regardless of Wizard setting?

    //uint32_t t2 = micros();
    //Serial << "work update: " << t2-t1 << "uS";

    //t1 = micros();
    //EEPROM.put(eeAddr + 2, input_vars);
    //t2 = micros();
    //Serial << "  ee update: " << t2-t1 << "uS\r\n";

  }


  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }

  if (Serial.available()) {
    uint8_t sRead = Serial.read();
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
