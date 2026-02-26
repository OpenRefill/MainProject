#include <Arduino.h>
// Device configs
#include "commespcfg.h"
#include "pinmap.h"
// C99 libraries
// #include <cstdarg>
// #include <cstdlib>
// #include <string.h>
// #include <time.h>

// Libraries for MQTT client and WiFi connection
// #include <WiFi.h>
// #include <WiFiClientSecure.h>

// I2C
#include <Wire.h>
// Azure IoT SDK for C includes
// #include <az_core.h>
// #include <az_iot.h>


// Additional azure headers
// #include "AzureIoT.h"
// #include "Azure_IoT_PnP_Template.h"
// #include "iot_configs.h"

// serial RX
#include "serialparser.h"
// log util
#include "dxlogutil.h"
// wifi, mqtt and time
#include "dxclient.h"

SerialParser dxsp;
void _doCheckSerial();
int8_t _doParseCmd(ComCmds cmd, float *args);


// Timers



/* --- Settings from iot_configs.h --- */
// static const char* wifi_ssid = IOT_CONFIG_WIFI_SSID; 
// static const char* wifi_password = IOT_CONFIG_WIFI_PASSWORD;


// --- Function Declarations ---
void dxI2ConRequest();
void dxI2ConReceive();

// I2C
bool i2c_is_rdy = false;

void dxI2ConRequest() // Anwsers to Master's "requestFrom"
{
  // LogInfo("Request received. Sending buffered data."); //The sending happens on the background
}


// Anwsers to Master's "transmissions"
void dxI2ConReceive(int len) 
{
  // String response = "";  // request reply msg
  String rx_msg_str = "";   // to save Master's message
  // char rx_msg_chr[len] = "";
  // int msg_index = 0;

  // If there are bytes through I2C
  while (Wire.available())   rx_msg_str.concat((char)Wire.read()); // make a string out of the bytes

  // rx_msg_str.toCharArray(rx_msg_chr, len);  
  LogInfo("received message[%d]: %s", len,rx_msg_str.c_str());
  // Filter I2C Master messages
  // Ready: tell MainEsp if comms is ready, note this is not called by Main currently
  if (rx_msg_str == "Ready") 
  {
    String response_state = "Yes";
    // if (!send_device_info) product1String = "Yes";
    // else product1String = "No";
    
    int str_len = response_state.length() + 1;
    char char_array[str_len];
    response_state.toCharArray(char_array, str_len);
    Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  }
  // Main ESP request date, for printing on receipt
  else if(rx_msg_str=="Date")
  {
    String DT = getDateTime();
    int str_len = DT.length() + 1;
    char char_array[str_len];
    DT.toCharArray(char_array, str_len);
    Wire.slaveWrite((uint8_t *)char_array, str_len); 
  }
  // product messages, not used
  // must be dispense message
  else if(rx_msg_str[0]!='R' && rx_msg_str[0]!='P') 
  {
    sendMsgFormI2C(rx_msg_str); 
  }
}

/*
================================================================================================
MAIN SETUP AND LOOP
================================================================================================
*/
void setup()
{
  // Set serial monitor debugging window baud rate to 115200
  // Serial.begin(SERIAL_BAUD_RATE);
  dxsp.doStartup();
  set_logging_function(logging_function);

  // Start I2C communication
  Wire.onReceive(dxI2ConReceive);
  Wire.onRequest(dxI2ConRequest);
  Wire.begin((uint8_t)I2C_DEV_ADDR); // Starting Wire as Slave
  
  // start internet connection
  dxClientStartup();


  
}

void loop()
{
  _doCheckSerial();
  // dxWiFiProcessState();
  
  dxClientProcess();

}

void _doCheckSerial()
{
  //CHECK SERIAL
  if (dxsp.checkSerial())
  { 
      ComCmds cmd_num = dxsp.getCurrentCmd();
      float *data_ptr = dxsp.getRxDataPointer();
      int8_t cmd_error = _doParseCmd(cmd_num,data_ptr);
      dxsp.clear(cmd_error);

  }
}

int8_t _doParseCmd(ComCmds cmd, float *args)
{
  int8_t error_code = 0;
  float value_1 = args[0];
  // float value_2 = args[1];
  // float value_3 = args[2];
  // float value_4 = args[3];
  char (*cfg_strings)[SERIAL_NUM_RX_CHARS] = dxsp.getStringValues();

  switch (cmd){
    case CMD_DX_ID:
      LogInfo(COM_DX_MSG "version:%s",VERSION_COMMESP);
      break;

    //==================================
  
    //==================================
    //ESP CMDS
    case CMD_ESP_RESET:
      LogInfo("DO ESP RESTART...");
      delay(500);
      esp_restart();
      break;

    //==================================
    case CMD_MQTT_GET_STATE:
      dxClientPrintState();
      break;

    case CMD_MQTT_GET_WIFI_CRED:
      dxWifiPrintCred();
      break;
    
    case CMD_MQTT_GET_DEV_ID:
      dxClientGetDevNum();
      break;

    case CMD_MQTT_GET_DATE:
      LogInfo(COM_DX_MSG "date:%s",getDateTime().c_str());
      break;

    case CMD_MQTT_SET_WIFI_CRED:
      dxWifiSetCred(cfg_strings[0],cfg_strings[1]);
      break;

    case CMD_MQTT_SET_DEV_ID:
      dxClientSetDevNum((uint32_t) value_1);
      break;

    case CMD_MQTT_SIM_DX:
      if (value_1 == 1) dxClientSimDx();
      break;

    case CMD_MQTT_FORCE_RESTART:
    if (value_1 == 1) dxClientRestart();
    break;

    default:
      error_code = -1;
      break;
  };

  return error_code;
}












