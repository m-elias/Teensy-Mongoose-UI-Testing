#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>
#include "mongoose_init.h"

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_input_settings(&s_input_global); before making changes
//  - call glue_set_input_settings(&s_input_global); after making changes
struct input_settings s_input_global = {
  false,  // bool steer_state
  false,  // bool work_state
  0,      // int work_input
  50,     // int work_thres
  "18",   // int work_hyst
  true,   // bool work_invert
  false,  // bool kickout_state
  false,  // bool update
  "AiO GUI v5.old"  // char fversion[40]
};

const uint16_t EE_IDENT = 2412;
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
  EEPROM.get(eeAddr + 0, eeIdentRead);

  if (eeIdentRead != EE_IDENT) {  // if value in eeprom does not match, overwrite with defaults
    EEPROM.put(eeAddr + 0, EE_IDENT);           // EEPROM.put() uses EEPROM.update() which only writes to EEPROM when the value has changed
    EEPROM.put(eeAddr + 2, s_input_global);
    Serial.print("\r\n- ** EEPROM version mismatch, reset to defaults! **\r\n");
  } else {
    EEPROM.get(eeAddr + 2, s_input_global);  // read the Settings saved in EEPROM to the global Settings struct
    Serial.print("\r\n- loaded settings from EEPROM\r\n");
  }
  glue_set_input_settings(&s_input_global);

  long num = random();
  Serial.println((num));
  Serial.println(u_int(num));
  Serial.println(int(num));
  //Serial.println(byte(num));
  //Serial.println(u_long(0xFFFFFFFF));
  Serial.println(float(num) / 0xFFFFFFFF, 3);

  /*glue_get_input_settings(&s_input_global);
  memcpy(s_input_global.fversion, "Hello World!", sizeof("Hello World!"));
  glue_set_input_settings(&s_input_global);*/

}

void loop() {
  mongoose_poll();

  static elapsedMillis workTimer = 0;
  if (workTimer > 99) { // 10hz
    //uint32_t t1 = micros();
    workTimer -= 100;   // try to maintain steady 10hz
    static bool workState = 0;
    float read = float(analogRead(WAS_A_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)

    glue_get_input_settings(&s_input_global); // pull from UI
    
    uint8_t work_hyst = atoi(s_input_global.work_hyst);
    if (read < max(s_input_global.work_thres - work_hyst, 10))  // limit to >10% (0.33V)
      workState = LOW;
    if (read > min(s_input_global.work_thres + work_hyst, 90))  // limit to <90% (2.97V)
      workState = HIGH;

    s_input_global.work_state = (!workState != !s_input_global.work_invert);  // XOR the work state logic with invert setting
    s_input_global.work_input = min(round(read), 100);

    s_input_global.steer_state = !digitalRead(STEER_PIN);
    s_input_global.kickout_state = !digitalRead(KICKOUT_D_PIN);
    //s_input_global.update = true;

    glue_set_input_settings(&s_input_global); // push to UI
    glue_update_state();  // triggers UI update, max 1hz regardless of Wizard setting?


    //uint32_t t2 = micros();
    //Serial << "work update: " << t2-t1 << "uS";

    //t1 = micros();
    //EEPROM.put(eeAddr + 2, s_input_global);
    //t2 = micros();
    //Serial << "  ee update: " << t2-t1 << "uS\r\n";
  }


  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }

}
