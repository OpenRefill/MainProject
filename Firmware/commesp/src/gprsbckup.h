#ifndef GPRS_BACKUP_H_FILE
#define GPRS_BACKUP_H_FILE

/*
could not get GPRS modem to work/connect with AZURE nicely

code stored here for future ref.

============================================================================================================
CONFIGS:
  // enable debug prints of AT commands to console, if needed
  // #define DUMP_AT_COMMANDS

  enum PscStates{
    PSC_STATE_NOT_CONNECTED = 0,  // not connected
    PSC_STATE_CONNECTED,          // connection made
    PSC_STATE_PUBLISHED,          // published, assume publish is a blocking call
    PSC_STATE_SUBSCRIBED,         // subscribed, assume is blocking call
  };
============================================================================================================
PLATFORM-IO: platformio.ini

	knolleary/PubSubClient@^2.8
	digitaldragon/SSLClient@^1.2.0
============================================================================================================
MAIN FUNCTION DECLARE   
    #include <PubSubClient.h>
    #include <SSLClient.h>

    void startModem();
    void restartSimCard();
    bool setPowerBoostKeepOn(int en);
    void pscCallback(const char *topic, byte* payload, unsigned int length);

    // Set serial for AT commands (to SIM800 module)
    #define SerialAT Serial1
    #define TINY_GSM_MODEM_SIM800   // Modem is SIM800
    #define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
    #include <TinyGsmClient.h>

    // enable debug prints of AT commands to console, if needed
    #ifdef DUMP_AT_COMMANDS
      #include <StreamDebugger.h>
      StreamDebugger debugger(SerialAT, SerialMon);
      TinyGsm modem(debugger);
    #else
      TinyGsm modem(SerialAT);
    #endif

    // GSM SIM Card Credentials 
    // Your GPRS credentials (leave empty, if not needed)
    // APN is required, but username, password and SIM pin may not be needed.
    
    const char apn[]      = "internet";//"lte.vodacom.za"; // YOUR CELLULAR APN HERE
    const char gprsUser[] = ""; // GPRS User
    const char gprsPass[] = ""; // GPRS Password
    const char simPIN[]   = ""; // SIM card PIN (leave empty, if not defined)


    //Layers stack
    //TinyGsm sim_modem(SerialAT);
    TinyGsmClient client(modem); //gsm_transpor_layer
    SSLClient secure_presentation_layer(&client);
    PubSubClient psc_mqtt(secure_presentation_layer);

    //psc mqtt state to simulate connection/disconnection event

    PscStates psc_mqtt_state;// = PSC_STATE_NOT_CONNECTED;
    int psc_msg_id = 0;
    // psc_mqtt_state = PSC_STATE_NOT_CONNECTED;
    
============================================================================================================
MAIN FUNCTION DEFINITONS

static int mqtt_client_init_function(mqtt_client_config_t *mqtt_client_config, mqtt_client_handle_t *mqtt_client_handle)
{
    // use GPRS
    secure_presentation_layer.setCACert((const char *)ca_pem);
    psc_mqtt_state = PSC_STATE_NOT_CONNECTED;
    psc_mqtt.setKeepAlive(30);
    psc_mqtt.setServer("global.azure-devices-provisioning.net",mqtt_client_config->port);
    psc_mqtt.setCallback(pscCallback);
    const char* client_id = (const char *)az_span_ptr(mqtt_client_config->client_id);
    const char* username = (const char *)az_span_ptr(mqtt_client_config->username);
    const char* password = (const char *)az_span_ptr(mqtt_client_config->password);

    LogInfo("psc_mqtt init, broker: %s, port: %d", mqtt_broker_uri,mqtt_client_config->port);
    LogInfo("psc_mqtt connect, client_id:%s",client_id);
    LogInfo("psc_mqtt connect, username:%s",username);
    LogInfo("psc_mqtt connect, password:%s",password);
    bool connect_ok = false;
    int32_t connect_cnt = 0;
    while (!connect_ok){
    connect_ok = psc_mqtt.connect(client_id,username,password);
    LogInfo("psc_mqt, state: %d, connect_ok:%d, connect_cnt:%d",psc_mqtt.state(),connect_ok,connect_cnt);
    if (connect_ok) break;
    if (connect_cnt++ > 5) break;
    delay(2000);
    }
    //psc_mqtt_state
    //change state in loop, to trigger event
    if (!connect_ok)  LogError("psc mqtt failed");
    else
    {
    LogInfo("psc mqtt connected");
    *mqtt_client_handle = &psc_mqtt;
    result = 0;
    }
}

static int mqtt_client_deinit_function(mqtt_client_handle_t mqtt_client_handle)
{
    psc_mqtt.disconnect();
}

static int mqtt_client_subscribe_function(mqtt_client_handle_t mqtt_client_handle, az_span topic, mqtt_qos_t qos)
{
    // USE GPRS
    // qos > 1 not supported
    bool ok = psc_mqtt.subscribe( (const char *)az_span_ptr(topic), 
                                    (uint8_t)qos);
    if (ok)
    {
        psc_mqtt_state = PSC_STATE_SUBSCRIBED;
        LogInfo("psc_mqtt.subscribe ok, topic:%s,qos:%d",(const char *)az_span_ptr(topic),(uint8_t)qos);
    }
    else
    {
        LogError("psc_mqtt.subscribe failed, topic:%s,qos:%d",(const char *)az_span_ptr(topic),(uint8_t)qos);
        packet_id = -1;
    }
}

static int mqtt_client_publish_function(mqtt_client_handle_t mqtt_client_handle, mqtt_message_t *mqtt_message)
{
    // USE GPRS 
    //QOS not supported
    bool ok = psc_mqtt.publish((const char *)az_span_ptr(mqtt_message->topic),
                                (const char *)az_span_ptr(mqtt_message->payload),
                                az_span_size(mqtt_message->payload));
    if (ok) {
        psc_mqtt_state = PSC_STATE_PUBLISHED;
        // psc_msg_id++;
        // if (psc_msg_id > 50000) psc_msg_id = 0;

    }
    if (ok) return RESULT_OK;
    else    return RESULT_ERROR;
}

void setup()
{
    .....
    if (!setPowerBoostKeepOn(1))
    {
      LogError("Setting board power management error");
    }
    else LogInfo("Setting board power management ok");
    ...
    startModem();
}

void loop()
{
    ....
     // USE GPRS
    connection_ok = modem.gprsConnect(apn, gprsUser, gprsPass);
    if (!connection_ok)
    {
      restartSimCard();
      azure_iot_start(&azure_iot);
    }
    else // modem is connected
    {
      bool psc_mqtt_connected = psc_mqtt.connected();
      switch (psc_mqtt_state)
      {
        case PSC_STATE_NOT_CONNECTED:
          // connection made
          if (psc_mqtt_connected){
            mqtt_on_event_connected(0);
            psc_mqtt_state = PSC_STATE_CONNECTED;
          }
          // else reconnect?
          break;

        case PSC_STATE_CONNECTED:
          // connection lost
          if (!psc_mqtt_connected){
            mqtt_on_event_disconnected();
            psc_mqtt_state = PSC_STATE_NOT_CONNECTED;
          }
          // else connection ok
          break;

        case PSC_STATE_PUBLISHED:
          if (!psc_mqtt_connected){
            mqtt_on_event_disconnected();
            psc_mqtt_state = PSC_STATE_NOT_CONNECTED;
          }
          else
          {
            mqtt_on_event_pusblished(psc_msg_id);
            psc_mqtt_state = PSC_STATE_CONNECTED;
          }
          break;

        case PSC_STATE_SUBSCRIBED:
          if (!psc_mqtt_connected){
            mqtt_on_event_disconnected();
            psc_mqtt_state = PSC_STATE_NOT_CONNECTED;
          }
          else
          {
            mqtt_on_event_subscribed(psc_msg_id);
            psc_mqtt_state = PSC_STATE_CONNECTED;
          }
          break;
      }
    }
    ....
}


================================================================================================
MODEM FUNCTIONS
================================================================================================

  void startModem()
  {
    LogInfo("Modem Set Pins, wait 6 s");
    // Set modem reset, enable, power pins for SIM800L
    pinMode(PIN_MODEM_PWKEY, OUTPUT);
    pinMode(PIN_MODEM_RST, OUTPUT);
    pinMode(PIN_MODEM_POWER_ON, OUTPUT);

    digitalWrite(PIN_MODEM_PWKEY, LOW);
    digitalWrite(PIN_MODEM_RST, HIGH);
    digitalWrite(PIN_MODEM_POWER_ON, HIGH);

    delay(6000);

    // Set SIM800L module baud rate and UART pins 
    SerialAT.begin(115200, SERIAL_8N1, PIN_MODEM_RX, PIN_MODEM_TX); 

    // Restart SIM800 module, it takes quite some time, to skip, call init() instead of restart()
    LogInfo("Initializing modem...");
    modem.init(simPIN);
    delay(3000);

    // Unlock your SIM card with a PIN if needed
    // if (strlen(simPIN) && modem.getSimStatus() != 3)
    // {
    //   LogInfo("sim unlock:%d",modem.simUnlock(simPIN));
    // }

    //SET CLIENT authentication
    //If need server authentication,        please set AT+SSLOPT=0,0
    //If do not need server authentication, please set AT+SSLOPT=0,1
    //If need client authentication,        please set AT+SSLOPT=1,1
    //If do not need client authentication, please set AT+SSLOPT=1,0
    modem.sendAT("+SSLOPT=1,1");
    if (modem.waitResponse() != 1)
    {
      LogInfo("modem set client authentication failed");
    }
    //print modem info
    LogInfo("Modem Name: %s",modem.getModemName().c_str());
    LogInfo("Modem Info: %s",modem.getModemInfo().c_str());
    //wait NW
    LogInfo("Waiting for network... ");
    modem.waitForNetwork(600000L);
    if (modem.isNetworkConnected()) LogInfo("Network connected!");
    //else?

    // use modem.init() if you don't need the complete restart
    modem.gprsConnect(apn, gprsUser, gprsPass);
    
    // GPRS connection parameters are usually set after network registration
    int connectAttempts = 0;
    while (!modem.gprsConnect(apn, gprsUser, gprsPass))
    {
      LogInfo("Connecting to apn: %s",apn);
      delay(10000);
      
      connectAttempts++;
      if (connectAttempts > 3) 
      {
        LogInfo("Unable to connect apn, restarting");
        delay(750);
        ESP.restart();
      }
    }
    LogInfo("GPRS status connected: %d", modem.isGprsConnected());
    LogInfo("Modem CCID: %s",modem.getSimCCID().c_str());
    LogInfo("Modem IMEI: %s",modem.getIMEI().c_str());
    LogInfo("Modem IMSI: %s",modem.getIMSI().c_str());
    LogInfo("Modem Operator: %s",modem.getOperator().c_str());
    LogInfo("Modem localIP: %s",modem.localIP().toString().c_str());
    LogInfo("Modem signal: %d",modem.getSignalQuality());
    
    
    if (modem.isGprsConnected())
    { 
      sync_device_clock_with_ntp_server();
      azure_pnp_init();
      dx_azure_init();
    } 

  }

  void restartSimCard()
  {
    modem.restart(); // use modem.init() if you don't need the complete restart
    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN) && modem.getSimStatus() != 3)
    {
      modem.simUnlock(simPIN);
    }
  }

  // I2C for SIM800 (to keep it running when powered from battery)
  bool setPowerBoostKeepOn(int en){
    bool ok = false;
    Wire.begin(PIN_I2C_SDA,PIN_I2C_SCL);
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en) {
      Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    } else {
      Wire.write(0x35); // 0x37 is default reg value
    }
    ok = Wire.endTransmission() == 0;
    Wire.end();
    return ok;
  }

static void sync_device_clock_with_ntp_server()
{
    // use modem for Sync
    //https://github.com/vshymanskyy/TinyGSM/issues/357
    //https://github.com/vshymanskyy/TinyGSM/issues/620
    //https://github.com/mathieucarbou
    //https://en.cppreference.com/w/c/chrono/tm
    //https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/system/system_time.html
    // int ntp_year, ntp_month, ntp_day, ntp_hour, ntp_min, ntp_sec;
    struct tm m_tm = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    float modem_time_zone = 0;

    int sync_res = modem.NTPServerSync();
    LogInfo("sync_res:%d:str:%s",sync_res,modem.ShowNTPError(sync_res).c_str());
    if (sync_res == 1 && modem.getNetworkTime(&m_tm.tm_year, &m_tm.tm_mon, &m_tm.tm_mday, &m_tm.tm_hour, &m_tm.tm_min, &m_tm.tm_sec, &modem_time_zone))
    {
      m_tm.tm_year -= 1900;
      m_tm.tm_mon -= 1;
      //m_tm.tm_isdst = 0; //daylight saving, 0: none, -1, unkown
      LogInfo("year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d,tz:%f",m_tm.tm_year, m_tm.tm_mon, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec, modem_time_zone);
      // unix time
      struct timeval now = {mktime(&m_tm),0};// - static_cast<int>(modem_time_zone * 3600.0), 0}; //need to minus tz if operator reports time with it included
      LogInfo("now.tv_sec: %d", now.tv_sec);

      // install the time
      setenv("TZ", "UTC0", 1); // assume time is now UTC+0
      tzset();
      settimeofday(&now, nullptr);
      LogInfo("settimeofday done");
      // TODO get time zone string using modem_time_zone instead of define, could be issue as operators report it differently..
      // change timezone
      setenv("TZ", MODEM_GMT_TIME_ZONE, 1);
      tzset();
      LogInfo("tzset done");
    } // res ok and got values
    // use modem for sync
}

void pscCallback(const char *topic, byte* payload, unsigned int length)
  {
    esp_mqtt_event_handle_t data_event;
    char* topic_str = const_cast<char*>(topic);
    char* payload_str = new char[length + 1];
    memcpy(payload_str, payload, length);
    payload_str[length] = '\0';
    LogInfo("on_msg,topic:%s,strval:%s",topic_str,payload_str);
      
    // data_event->event_id = MQTT_EVENT_DATA;
    // data_event->topic = topic_str;
    // data_event->topic_len = strlen(topic);
    // data_event->data = payload_str;
    // data_event->data_len = length;

    mqtt_on_event_data(topic_str, strlen(topic_str), payload_str, length);
    delete[] payload_str;

  }
*/
#endif