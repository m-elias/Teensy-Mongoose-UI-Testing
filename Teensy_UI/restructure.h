/*
#include "Arduino.h"

  MG_INFO(("rebooting"));
  SCB_AIRCR = 0x05FA0004; // Teensy reboot

  if(s_inputs.workThres > 10) s_inputs.workThres -= 1;
  glue_update_state();

  if(s_inputs.workThres < 90) s_inputs.workThres += 1;
  glue_update_state();

  //s_inputs.workThres = min(max(s_inputs.workInput, 10), 90);
  s_inputs.workThres = constrain(s_inputs.workInput, 10, 90);
  glue_update_state();

  s_inputs.workThres = 50;
  memcpy(s_inputs.workHystStr, "18", 2);
  s_inputs.workInvert = true;
  glue_update_state();



// Dropdown item lists
const char* pwmFreqList = "[\"122 hz - Baraki Valve\",\"490 hz\",\"1000 hz - Danfoss\",\"3921 hz\",\"9210 hz\",\"39210 hz - Motor\"]";
const char* kickoutModeList = "[\"1 - AOG Setting (default)\",\"2 - Quadrature Encoder\",\"3 - JD Variable Duty Encoder\",\"4 - WAS-PWM Ratio\",\"5 - Encoder Speed\"]";
const char* ko_help[6] = {
    "Unknown option/error.",
    "Set AOG to Count/Pressure/Current Sensor according to your setup",
    "Set AOG to \"Count Sensor\". Connect both signals, one to each Kickout Input (Analog & Digital)",
    "Set AOG to \"Pressure Sensor\". Connect to Kickout Analog (Fastest response time)",
    "Set AOG to \"Pressure Sensor\". *Need more instructions*",
    "Set AOG to \"Pressure Sensor\". *Need more instructions*"
};

// added these forward declarations so that btn functions can access API endpoint structs
static struct comms s_comms;
static struct inputs s_inputs;
static struct outputs s_outputs;
static struct misc s_misc;

static void ws_timer_50(void *arg) {    //  || s_misc.update
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mg_connection *c;
  //uint64_t now = mg_millis();

  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_websocket) {
      if (c->send.len > 2048) continue;  // Prevent stale connections to grow infinitely

      s_misc.update = false;
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("steerState"), s_inputs.steerState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workState"), s_inputs.workState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workInput"), s_inputs.workInput);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workThres"), s_inputs.workThres);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutState"), s_inputs.kickoutState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutStateHist"), s_inputs.kickoutStateHist);
    }
  }
}

// Update help text in Kickout panel according to option selected in dropdown
static void ws_timer_200(void *arg) {    // || oldKickoutMode != s_inputs.kickoutModeStr[0]
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mg_connection *c;
  //uint64_t now = mg_millis();
  //static uint8_t oldKickoutMode = 0;

  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_websocket) {
      if (c->send.len > 2048) continue;  // Prevent stale connections to grow infinitely

      // check first char of dropdown's selected option to match displayed help text
      uint8_t koSelection = s_inputs.kickoutModeStr[0] - '0';
      const char* ko_help_selected = ko_help[koSelection];

      //MG_INFO((s_inputs.kickoutModeStr));
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("kickout_dropdown_help"), MG_ESC((ko_help_selected)));
      //oldKickoutMode = s_inputs.kickoutModeStr[0];
    }
  }
}

void glue_init(void) {
  MG_INFO(("Starting websocket timers"));

  mg_timer_add(&g_mgr, 50,        // Call every 50 milliseconds
               MG_TIMER_REPEAT,   // Periodically
               ws_timer_50,   // This function
               &g_mgr             // And pass this parameter to the function
  );

  mg_timer_add(&g_mgr, 200,      // Call every 2000 milliseconds
               MG_TIMER_REPEAT,   // Periodically
               ws_timer_200,       // This function
               &g_mgr             // And pass this parameter to the function
  );

  s_comms.gps2State = 3;    // set as 3 - "Undetected" at boot, UI Wizard defaults to '4' to show all UI elements while designing layout
  //s_comms.ntripState = 3;
  MG_DEBUG(("Custom init done"));
}
*/