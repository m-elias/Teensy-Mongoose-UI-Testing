// SPDX-FileCopyrightText: 2024 Cesanta Software Limited
// SPDX-License-Identifier: GPL-2.0-only or commercial
// Generated by Mongoose Wizard, https://mongoose.ws/wizard/

// Include your device-specific headers, and edit functions below
// #include "hal.h"

#include "mongoose_glue.h"

void glue_init(void) {
  MG_DEBUG(("Custom init done"));
}

// This function is called automatically every WIZARD_WEBSOCKET_TIMER_MS millis
void glue_websocket_on_timer(struct mg_connection *c) {
  uint64_t *timer_voltage = (uint64_t *) &c->data[0];  // Beware: c->data size is MG_DATA_SIZE
  uint64_t *timer_pressure = (uint64_t *) &c->data[sizeof(uint64_t)];
  uint64_t now = mg_millis();

  // Send updates to "websocket.voltage" value every 200 milliseconds
  if (mg_timer_expired(timer_voltage, 200, now)) {
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %llu, %m: %s}", MG_ESC("voltage"),
                 now % 1000, MG_ESC("led"), now & 1 ? "true" : "false");
  }

  // Send updates to "websocket.pressure" value every 5000 milliseconds
  if (mg_timer_expired(timer_pressure, 5000, now)) {
    mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %llu}", MG_ESC("pressure"), now);
  }
}


// reboot
static uint64_t s_action_timeout_reboot;  // Time when reboot ends
bool glue_check_reboot(void) {
  return s_action_timeout_reboot > mg_now(); // Return true if reboot is in progress
}
void glue_start_reboot(void) {
  s_action_timeout_reboot = mg_now() + 1000; // Start reboot, finish after 1 second
}

// dec_work_thres
static uint64_t s_action_timeout_dec_work_thres;  // Time when dec_work_thres ends
bool glue_check_dec_work_thres(void) {
  return s_action_timeout_dec_work_thres > mg_now(); // Return true if dec_work_thres is in progress
}
void glue_start_dec_work_thres(void) {
  s_action_timeout_dec_work_thres = mg_now() + 1000; // Start dec_work_thres, finish after 1 second
}

// set_work_thres
static uint64_t s_action_timeout_set_work_thres;  // Time when set_work_thres ends
bool glue_check_set_work_thres(void) {
  return s_action_timeout_set_work_thres > mg_now(); // Return true if set_work_thres is in progress
}
void glue_start_set_work_thres(void) {
  s_action_timeout_set_work_thres = mg_now() + 1000; // Start set_work_thres, finish after 1 second
}

// inc_work_thres
static uint64_t s_action_timeout_inc_work_thres;  // Time when inc_work_thres ends
bool glue_check_inc_work_thres(void) {
  return s_action_timeout_inc_work_thres > mg_now(); // Return true if inc_work_thres is in progress
}
void glue_start_inc_work_thres(void) {
  s_action_timeout_inc_work_thres = mg_now() + 1000; // Start inc_work_thres, finish after 1 second
}

// set_work_digital
static uint64_t s_action_timeout_set_work_digital;  // Time when set_work_digital ends
bool glue_check_set_work_digital(void) {
  return s_action_timeout_set_work_digital > mg_now(); // Return true if set_work_digital is in progress
}
void glue_start_set_work_digital(void) {
  s_action_timeout_set_work_digital = mg_now() + 1000; // Start set_work_digital, finish after 1 second
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

static struct comms s_comms = {"60ms - F9P", false};
void glue_get_comms(struct comms *data) {
  *data = s_comms;  // Sync with your device
}
void glue_set_comms(struct comms *data) {
  s_comms = *data; // Sync with your device
}

static struct inputs s_inputs = {2, false, 2, false, 50, true, 50, "18", 18, 2, false, 0, "1 - AOG Setting (default)"};
void glue_get_inputs(struct inputs *data) {
  *data = s_inputs;  // Sync with your device
}
void glue_set_inputs(struct inputs *data) {
  s_inputs = *data; // Sync with your device
}

static struct misc s_misc = {75, false, "AiO GUI v5.old"};
void glue_get_misc(struct misc *data) {
  *data = s_misc;  // Sync with your device
}
void glue_set_misc(struct misc *data) {
  s_misc = *data; // Sync with your device
}