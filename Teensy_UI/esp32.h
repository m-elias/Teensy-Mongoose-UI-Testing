#define SerialESP32 Serial2
const int32_t baudESP32 = 460800;
uint8_t ESP32rxbuffer[256];   // don't know what size is needed
uint8_t ESP32txbuffer[256];   // don't know what size is needed
uint32_t esp32Runtime;
elapsedMillis esp32ElapsedUpdateTime;

void esp32DataCheck()
{

  struct comms comms_vars;
  glue_get_comms(&comms_vars); // pull-sync from UI
  if (SerialESP32.available())
  {
    if (comms_vars.esp32BridgeEnabled) {
      static uint8_t incomingBytes[50];
      static uint8_t incomingIndex;
      incomingBytes[incomingIndex] = SerialESP32.read();
      incomingIndex++;
      //Serial.print("\r\nindex: "); Serial.print(incomingIndex);
      //Serial.print(" ");
      //for (byte i = 0; i < incomingIndex; i++) {
        //Serial.print(incomingBytes[i]);
        //Serial.print(" ");
      //}

      if (incomingBytes[incomingIndex - 2] == 13 && incomingBytes[incomingIndex - 1] == 10)
      {
        if (incomingBytes[0] == 128 && incomingBytes[1] == 129)
        {
          /*for (byte i = 0; i < incomingIndex; i++) {
            Serial.print(incomingBytes[i]);
            Serial.print(" ");
          }
          Serial.println();*/

          // Process the special ESP32->Teensy PGNs
          if (incomingBytes[2] == 90 && incomingBytes[3] == 90) { // helloFromESP32 PGN
            union {   // both variables in the union share the same memory space
              byte array[4];
              uint32_t millis;
            } runtime;
            runtime.array[0] = incomingBytes[5];
            runtime.array[1] = incomingBytes[6];
            runtime.array[2] = incomingBytes[7];
            runtime.array[3] = incomingBytes[8];
            esp32Runtime = runtime.millis;
            uint8_t esp32NumClients = incomingBytes[9];
            //uint16_t esp32m2mPort = (incomingBytes[11] << 8) + incomingBytes[10];
            //Serial.printf("M2M Port: %i\r\n", esp32m2mPort);

            comms_vars.esp32State = 1;

            esp32Runtime /= 1000;
            
            uint8_t days = esp32Runtime / 86400;
            esp32Runtime %= 86400;
            
            uint8_t hrs = esp32Runtime / 3600;
            esp32Runtime %= 3600;
            
            uint8_t mins = esp32Runtime / 60;
            esp32Runtime %= 60;
            
            uint8_t secs = esp32Runtime;
            
            char esp32RuntimeStr[20];
            sprintf(esp32RuntimeStr, "%id %ih %im %is", days, hrs, mins, secs);
            //Serial.printf("ESP32 Runtime: %s\r\n", esp32RuntimeStr);
            strcpy(comms_vars.esp32Runtime, esp32RuntimeStr);
            
            comms_vars.esp32NumClients = esp32NumClients;
            glue_set_comms(&comms_vars);
            esp32ElapsedUpdateTime = 0;
            glue_update_state();
          }
          
          else if (incomingBytes[2] == 91 && incomingBytes[3] == 91) { // ESP32 WiFi Creds PGN
            //Serial.println("WiFi Creds PGN");

            char str[24];
            memset(str,'\0', sizeof(str));  //init to all 0, might not be necessary
            for (uint8_t i = 0; i < 24 && incomingBytes[i+5] != 0; i++) {
              str[i] = incomingBytes[i+5];  // only copy non 0 chars, 0 denotes end of char strings
            }
            //Serial.println(str);
            memcpy(comms_vars.esp32SSID, str, 24);    // copy to UI string

            memset(str,'\0', sizeof(str));  // zero out and repeat for password
            for (uint8_t i = 0; i < 24 && incomingBytes[i+5+24] != 0; i++) {
              str[i] = incomingBytes[i+5+24];
            }
            if (str[0] == 0) memcpy(str, "**blank**\0", 10);  // detect blank/empty password and indicate accordingly
            //Serial.println(str);
            memcpy(comms_vars.esp32PW, str, 24);

            glue_set_comms(&comms_vars);
            glue_update_state();

          // all other properly formed PGNs forwarded to AgIO
          } else {

            // Modules--Wifi:9999-->ESP32--serial-->Teensy--ethernet:9999-->AgIO
            //UDP.SendUdpByte(incomingBytes, incomingIndex - 2, UDP.broadcastIP, UDP.portAgIO_9999);

            //pass data to USB for debug
            //Serial.print("\r\nE32-s->T41-e:9999->AgIO ");
            //for (byte i = 0; i < incomingIndex - 2; i++) {
              //Serial.print(incomingBytes[i]);
              //Serial.print(" ");
            //}
            //Serial.print((String)" (" + SerialESP32->available() + ")");
          }

        // unformed/corrupt PGN detected
        } else {
          Serial.print("\r\n\nCR/LF detected but [0]/[1] bytes != 128/129\r\n");
          for (byte i = 0; i < incomingIndex; i++) {
            Serial.print(incomingBytes[i]);
            Serial.print(" ");
          }
          Serial.println();
        }
        incomingIndex = 0;
      }
    } else {  // if esp32 bridge is NOT enabled, just dump the data (empty the buffer)
      SerialESP32.read();
      esp32ElapsedUpdateTime = 0;
      comms_vars.esp32State = 4;
    }
  } // end if (SerialESP32.available())

} // end esp32dataCheck()