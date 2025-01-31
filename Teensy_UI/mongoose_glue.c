#include "WProgram.h"
#include "core_pins.h"
// SPDX-FileCopyrightText: 2024 Cesanta Software Limited
// SPDX-License-Identifier: GPL-2.0-only or commercial
// Generated by Mongoose Wizard, https://mongoose.ws/wizard/

// Include your device-specific headers, and edit functions below
// #include "hal.h"

#include "mongoose_glue.h"

static struct settings s_settings = {192, 168, 5, 126, 1, false, false, false, 33, 50, 3, false, "AiO GUI v5.12"};
const uint8_t WORK_HYST_LOOKUP[6] = {1,2,3,5,8,12};

void glue_init(void) {
  MG_DEBUG(("Custom init done"));
}

// This function is called automatically every WIZARD_WEBSOCKET_TIMER_MS millis
void glue_websocket_on_timer(struct mg_connection *c) {
  //static uint64_t timer_voltage = 0;
  //static uint64_t timer_pressure = 0;
  uint64_t now = mg_millis();
  static uint64_t timer_work = 0;

  // Prevent stale connections to grow infinitely
  if (c->send.len > 1024) return;

  // Send updates to "websocket.voltage" value every 100 milliseconds
  /*if (mg_timer_expired(&timer_voltage, 100, now)) {
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %llu}", MG_ESC("voltage"),
                 now % 9999);
  }

  // Send updates to "websocket.pressure" value every 5000 milliseconds
  if (mg_timer_expired(&timer_pressure, 5000, now)) {
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %llu}", MG_ESC("pressure"), now);
  }*/

  // Send updates to "websocket.work_state" value every 200 milliseconds
  if (mg_timer_expired(&timer_work, 100, now)) {
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("work_state_websocket"), s_settings.work_state);

    char myInputText[101] = "---------|---------|---------|---------|---------|---------|---------|---------|---------|----------";
    myInputText[s_settings.work_input - 1] = '*';
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("work_input_websocket"), MG_ESC(myInputText));

    char myThresText[101] = "----------------------------------------------------------------------------------------------------";
    myThresText[s_settings.work_thres - WORK_HYST_LOOKUP[s_settings.work_hyst] - 1] = '^';
    myThresText[s_settings.work_thres + WORK_HYST_LOOKUP[s_settings.work_hyst] - 1] = '^';
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("work_thres_websocket"), MG_ESC(myThresText));
  }

}

// save_ip
static uint64_t s_action_timeout_save_ip;  // Time when save_ip ends
bool glue_check_save_ip(void) {
  return s_action_timeout_save_ip > mg_now(); // Return true if save_ip is in progress
}
void glue_start_save_ip(void) {
  s_action_timeout_save_ip = mg_now() + 250; // Start save_ip, finish after 250ms
  MG_INFO(("Save IP"));
}

// reboot
static uint64_t s_action_timeout_reboot;  // Time when reboot ends
bool glue_check_reboot(void) {
  return s_action_timeout_reboot > mg_now(); // Return true if reboot is in progress
}
void glue_start_reboot(void) {
  s_action_timeout_reboot = mg_now() + 1000; // Start reboot, finish after 1 second
  SCB_AIRCR = 0x05FA0004; // Teensy reboot
}

// dec_work_thres
static uint64_t s_action_timeout_dec_work_thres;  // Time when dec_work_thres ends
bool glue_check_dec_work_thres(void) {
  return s_action_timeout_dec_work_thres > mg_now(); // Return true if dec_work_thres is in progress
}
void glue_start_dec_work_thres(void) {
  s_action_timeout_dec_work_thres = mg_now() + 100; // Start dec_work_thres, finish after 1 second
  if(s_settings.work_thres > 0) s_settings.work_thres -= 1;
}

// set_work_thres
static uint64_t s_action_timeout_set_work_thres;  // Time when set_work_thres ends
bool glue_check_set_work_thres(void) {
  return s_action_timeout_set_work_thres > mg_now(); // Return true if set_work_thres is in progress
}
void glue_start_set_work_thres(void) {
  s_action_timeout_set_work_thres = mg_now() + 250; // Start set_work_thres, finish after 250ms
  s_settings.work_thres = s_settings.work_input;
}

// inc_work_thres
static uint64_t s_action_timeout_inc_work_thres;  // Time when inc_work_thres ends
bool glue_check_inc_work_thres(void) {
  return s_action_timeout_inc_work_thres > mg_now(); // Return true if inc_work_thres is in progress
}
void glue_start_inc_work_thres(void) {
  s_action_timeout_inc_work_thres = mg_now() + 100; // Start inc_work_thres, finish after 1 second
  if(s_settings.work_thres < 100) s_settings.work_thres += 1;
}

// set_work_digital
static uint64_t s_action_timeout_set_work_digital;  // Time when set_work_digital ends
bool glue_check_set_work_digital(void) {
  return s_action_timeout_set_work_digital > mg_now(); // Return true if set_work_digital is in progress
}
void glue_start_set_work_digital(void) {
  s_action_timeout_set_work_digital = mg_now() + 250; // Start set_work_digital, finish after 250ms
  s_settings.work_thres = 50;
  s_settings.work_hyst = 5;
  s_settings.work_invert = true;  
}

// firmware_update
void  *glue_ota_begin_firmware_update(char *file_name, size_t total_size) {
  bool ok = mg_ota_begin(total_size);
  MG_DEBUG(("%s size %lu, ok: %d", file_name, total_size, ok));
  return ok ? (void *) 1 : NULL;
}
bool  glue_ota_end_firmware_update(void *context) {
  bool ok = mg_ota_end();
  MG_DEBUG(("ctx: %p, success: %d", context, ok));
  return ok;
}
bool  glue_ota_write_firmware_update(void *context, void *buf, size_t len) {
  MG_DEBUG(("ctx: %p %p/%lu", context, buf, len));
  return mg_ota_write(buf, len);
}

// moved higher so that button functions can access struct
//static struct settings s_settings = {192, 168, 5, 126, 1, false, false, false, 33, 50, 3, false, "AiO GUI v5.12"};
void glue_get_settings(struct settings *data) {
  *data = s_settings;  // Sync with your device
}
void glue_set_settings(struct settings *data) {
  s_settings = *data; // Sync with your device
}