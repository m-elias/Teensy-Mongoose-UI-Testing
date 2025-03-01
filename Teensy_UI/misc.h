void checkMiscTimer()
{
  static elapsedMillis miscTimer = 0;
  if (miscTimer > 499) { // 2hz
    miscTimer -= 500;   // try to maintain steady 2hz
    struct misc misc_vars;
    glue_get_misc(&misc_vars); // pull-sync from UI

    LEDs.setBrightness(int(float(misc_vars.rgbBrightness) / 2.55));

    if (esp32ElapsedUpdateTime > 15000) {
      struct comms comms_vars;
      glue_get_comms(&comms_vars);
      if (comms_vars.esp32Detected == 1) comms_vars.esp32Detected = 2;  // set as "Lost", timed out
      glue_set_comms(&comms_vars);
    }
  }
}

void serialDataCheck()
{
  if (Serial.available()) {
    uint8_t sRead = Serial.read();
    struct inputs input_vars;

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