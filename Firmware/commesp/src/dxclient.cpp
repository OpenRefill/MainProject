
// For hmac SHA256 encryption
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
// load/save NVM
#include <Preferences.h>
// Wi-Fi
#include <WiFi.h>
// Azure
#include <azure_ca.h>
// azure interface
#include "Azure_IoT_PnP_Template.h"
#include "AzureIoT.h"

// local
#include "dxclient.h"
#include "dxlogutil.h"

WiFiManager wm;

// bool DX_MQTT_CONNECTED = false;
// bool DX_MQTT_CONNECT_EVENT = false;
bool DX_WIFI_CONNECTED = false;
bool DX_WIFI_FIRST_CONNECT = false; // connected to Wifi atleast once
bool DX_NTP_SYNCED = false;

bool DX_DO_STOCK_TAKE = false;
bool DX_SEND_INIT_INFO = false;

bool DX_LATCH_ERROR_FLAG = false; // only print error msg once
bool DX_LATCH_DISCONNECTION = false;
bool DX_LATCH_STOREOUTBOX = false;

hw_timer_t *DX_TIMER = NULL;

azure_iot_config_t azure_iot_config;
azure_iot_t azure_iot;
esp_mqtt_client_handle_t mqtt_client;
uint8_t az_iot_data_buffer[AZ_IOT_DATA_BUFFER_SIZE];

char mqtt_broker_uri[128];

uint32_t properties_request_id = 0;
bool get_device_props = true;

Preferences pref_dev_id;
uint32_t dev_num = 0;
uint32_t dx_cnt = 0;


/*
================================================================================================
local functions
================================================================================================
*/
void cfgLoad();
void cfgUpdateDxCnt();
void cfgSaveDevNum();
bool cfgCheckDevNum(uint32_t check_num);

void mqttOnEventError(esp_mqtt_event_handle_t event);

/*
================================================================================================
start up
================================================================================================
*/

void dxClientStartup()
{
  //dev number
  cfgLoad();
  // outbox
  dxOutbox.open();
  // wifi
  dxWifiStartup();
}

void dxClientPrintState()
{
  LogInfo("wifi state,wifi_ok:%d,status:%d,rssi:%d,ntp_synced:%d", WiFi.isConnected(), WiFi.status(),WiFi.RSSI(),DX_NTP_SYNCED);
  uint8_t state_num = (uint8_t) azure_iot_get_status(&azure_iot);
  if (state_num > 3) state_num = 3;
  LogInfo("iot-mqtt,state:%s,internal state:%d",AZURE_IOT_STATUS_LU[state_num],azure_iot_get_iot_state(&azure_iot));
}



void dxClientProcess()
{
  static uint32_t last_az_disconnect = 0;
  static uint32_t last_wifi_reconnect = 0;
  static azure_iot_status_t prev_state = azure_iot_disconnected;
  azure_iot_status_t state = azure_iot_get_status(&azure_iot);
  azure_iot_client_state_t internal_state = azure_iot_get_iot_state(&azure_iot);
  
  if (prev_state != state)
  {
    LogInfo("IOT-state change, was:%s,new:%s,internal:%d",AZURE_IOT_STATUS_LU[prev_state],AZURE_IOT_STATUS_LU[state],internal_state);
    prev_state = state;
  }
  // wifi tries to auto reconnect
  // if wifi is not conencted do not process MQTT
  // on wifi disconnected event, store any records to NVM
  if (!WiFi.isConnected())
  {
    switch (state)
    {
      case azure_iot_disconnected:
        break;

      case azure_iot_connected:
        azure_iot_do_work(&azure_iot);
        break;

      case azure_iot_error:
        break;
      
      default:
        break;
    }
  
    if (millis() - last_wifi_reconnect > 5000)
    {
      if (!DX_LATCH_STOREOUTBOX)
      {
        DX_LATCH_STOREOUTBOX = true;
        dxOutbox.enqueueBoxToStore();
      }
      LogInfo("Force WiFi recconnect,dt:%u",millis() - last_wifi_reconnect);
      last_wifi_reconnect = millis();
      WiFi.reconnect();
      
    }
    return;
  }
  else
  {
    // else if (!DX_NTP_SYNCED) dxSynWithNtpServer();
    // else if (!DX_WIFI_FIRST_CONNECT) dxWifiOnFirstConnect();
    // if WiFi is conencted and MQTT connected, try to resend any in NVM
    // else if (DX_MQTT_CONNECTED && dxOutbox.isAnyUnsent() && !dxOutbox.isEnqueueBoxFull())
    // {
    //   // dxOutboxResendNext(millis());  
    //   azure_iot_resend_telemetry(&azure_iot);
    // }

    // azure state is handled in detail in azure_iot_do_work, here only process specific ones
    /*
        0  azure_iot_state_not_initialized = 0,
        1  azure_iot_state_initialized,
        2  azure_iot_state_started,
        3  azure_iot_state_connecting_to_dps,
        4  azure_iot_state_connected_to_dps,
        5  azure_iot_state_subscribing_to_dps,
        6  azure_iot_state_subscribed_to_dps,
        7  azure_iot_state_provisioning_querying,
        8  azure_iot_state_provisioning_waiting,
        9  azure_iot_state_provisioned,
        10 azure_iot_state_connecting_to_hub,
        11 azure_iot_state_connected_to_hub,
        12 azure_iot_state_subscribing_to_pnp_cmds,
        13 azure_iot_state_subscribed_to_pnp_cmds,
        14 azure_iot_state_subscribing_to_pnp_props,
        15 azure_iot_state_subscribed_to_pnp_props,
        16 azure_iot_state_subscribing_to_pnp_writable_props,
        17 azure_iot_state_ready,
        18 azure_iot_state_refreshing_sas,
        19 azure_iot_state_error

    All the possible statuses returned by `azure_iot_get_status`.
    0 . azure_iot_disconnected
          The Azure IoT client is completely disconnected.

    1 . azure_iot_connecting
          The client is in an intermediate state between disconnected and connected.
            When using SAS-based authentication (default for Azure IoT Central), the client
            will automatically perform a reconnection with a new SAS token after the previous 
            one expires, going back into `azure_iot_connecting` state briefly.

    2 . azure_iot_connected
          In this state the Azure IoT client is ready to be used for messaging.

    3 . azure_iot_error 
          The Azure IoT client encountered some internal error and is no longer active.
            This can be caused by:
              - Memory issues (not enough memory provided in `azure_iot_config_t`'s `data_buffer`);
              - Any failures reported by the application's MQTT client
                (through the abstracted interface) return values;
              - Any protocol errors returned by Azure IoT services.
            If not enough memory has been provided in `azure_iot_config_t`'s `data_buffer,
            please expand the size of that memory space.

            Once the possible mitigations are applied stop the Azure IoT client 
            by calling `azure_iot_stop` (which resets the client state) and restart it
            using `azure-iot-start`.
    */
    
    switch (state)
    {
      case azure_iot_disconnected:
        // wifi is connected
        if (millis() - last_az_disconnect > 5000)
        {
          if (internal_state == azure_iot_state_not_initialized)
          {
          }
          else
          {
            LogInfo("try iot reconnect");
            azure_iot_stop(&azure_iot);
            if (internal_state  == azure_iot_state_initialized)  azure_iot_start(&azure_iot); // change state if possible
          }
          last_az_disconnect = millis();
        }
        break;

      case azure_iot_connecting:
        break;

      case azure_iot_connected:
        if (DX_SEND_INIT_INFO)
        {
          DX_SEND_INIT_INFO = false; // Only need to send once.
          LogInfo("I2C Ready!");
        }
        else
        {
        dxClientStockProcess(); 
        }
        if (DX_LATCH_ERROR_FLAG)
        {
          DX_LATCH_ERROR_FLAG = false;
          LogInfo("unset error flag latch");
        }
        if (dxOutbox.isAnyUnsent() && !dxOutbox.isEnqueueBoxFull())
        {
          azure_iot_resend_telemetry(&azure_iot);
        }
        break;
        
      case azure_iot_error:
        LogError("Azure IoT client is in error state.");
        delay(1000);
        azure_iot_stop(&azure_iot);
        // ESP.restart();
        break;
      
      default:
        break;
    }
    azure_iot_do_work(&azure_iot);
  }
}

void dxClientStockProcess()
{
  static bool timer_en = false;
  static bool first_start = false;

  if (!timer_en)
  {
    timerRestart(DX_TIMER);
    timerStart(DX_TIMER);
    timer_en=true;
  }
  float time_taken = timerReadSeconds(DX_TIMER);

  if (time_taken > TELEMETRY_FREQUENCY_IN_SECONDS)
  {
      DX_DO_STOCK_TAKE=true;
  }
  if (DX_DO_STOCK_TAKE || first_start)
  {
      first_start = false;
      DX_DO_STOCK_TAKE=false;
      timer_en=false; 
      sendStockTake();
  }
}

void dxClientSimDx()
{
  String sim_msg = "Finished:Sunsilk Pink:75:75:25:";
  LogInfo("SIMULATE DISPENSE I2C EVENT, msg:%s",sim_msg.c_str());
  sendMsgFormI2C(sim_msg);    
  // sendtoComs("Finished:"+ p_name + ":" + String(sel_amount_ml) + ":" + String(dx_amount_ml) + ":" + String(price) + ":");
}

void dxClientRestart()
{
  LogInfo("do restart, stopping...");
  azure_iot_stop(&azure_iot);
  for (size_t i = 0; i < 10; i++)
  {
    azure_iot_do_work(&azure_iot);
    delay(50);
  }
  LogInfo("do restart, start...");
  if (azure_iot_get_iot_state(&azure_iot) != azure_iot_state_not_initialized) azure_iot_start(&azure_iot);
  
  
}

void dxClientGetDevNum()
{
  LogInfo("dev_num:%d",dev_num);
}
/*
================================================================================================
config io
================================================================================================
*/

/* 
* set current machine number and restart
*/
void dxClientSetDevNum(uint32_t new_dev_num)
{
  if (!cfgCheckDevNum(new_dev_num)) return;
  dev_num = new_dev_num-1; // user input is 1-index: [1...x]
  cfgSaveDevNum();
  LogInfo("change dev num, will restart now...");
  delay(1000);
  esp_restart();

}

/* 
* save dev number
*/
void cfgSaveDevNum()
{
  size_t res = pref_dev_id.putULong(PREF_KEY_DEV_ID_MACHINE_NUM, dev_num);
  LogInfo("saved dev_num,dev_num:%u,res:%d",dev_num, res);
}

/* 
* update and save dx count
*/
void cfgUpdateDxCnt()
{
  dx_cnt++;
  size_t res = pref_dev_id.putULong(PREF_KEY_DEV_DX_CNT, dx_cnt);
  LogInfo("saved dx_cnt,dx_cnt:%u,res:%d",dx_cnt, res);
}
/* 
* load machine number/dev number for NVM
*/
void cfgLoad()
{
  bool ok = pref_dev_id.begin(PREF_NAMESPACE_DEVID, false);
  if (ok)
  {
    if (!pref_dev_id.isKey(PREF_KEY_DEV_ID_MACHINE_NUM))  cfgSaveDevNum();
    else
    {
      uint32_t loaded_zero_index = pref_dev_id.getULong(PREF_KEY_DEV_ID_MACHINE_NUM,dev_num);
      uint32_t loaded_one_index = loaded_zero_index + 1; // dev num is zero index (array index), but user input is from 1..10
      
      if (cfgCheckDevNum(loaded_zero_index)) // checks 1...x
      {
        dev_num = loaded_zero_index;
      }

      dx_cnt = pref_dev_id.getULong(PREF_KEY_DEV_DX_CNT, dx_cnt);
      
      LogInfo("load namepsace,opened:%s,free:%d,dev_num:%u,dx_cnt:%u",PREF_NAMESPACE_DEVID,pref_dev_id.freeEntries(),dev_num,dx_cnt);
    }
      
  }
  else
  {
    LogError("could not open space,name:%s,dev_num:%u",PREF_NAMESPACE_DEVID,dev_num);
  }
}

/* 
* checks dev 1...x, 1 indexed
*/
bool cfgCheckDevNum(uint32_t check_num)
{
  bool ok = (check_num > 0) && (check_num < NUM_BD_MACHINES);
  if (!ok)
  {
    LogError("device number out of bounds:%d",check_num);
  }
  return ok;
}


/*
================================================================================================
 TIME
================================================================================================
*/
void dxSynWithNtpServer()
{
  if (DX_NTP_SYNCED) return;
  LogInfo("DO NTP SYNC...");
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  uint32_t try_cnt = 0;
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(1000);
    Serial.print(".");
    now = time(NULL);
    if (try_cnt++ > 20)
    {
      
      break;
    }
  }
  Serial.println();
  DX_NTP_SYNCED = now > UNIX_TIME_NOV_13_2017;
  if (!DX_NTP_SYNCED)  LogError("Time Sync Failed!");
  else                 LogInfo("Time initialized!");

}

/*
================================================================================================
MQTT
================================================================================================
*/
/* --- MQTT Interface Functions --- */
/*
 * These functions are used by Azure IoT to interact with whatever MQTT client used by the sample
 * (in this case, Espressif's ESP MQTT). Please see the documentation in AzureIoT.h for more details.
 */
/*
 * See the documentation of `mqtt_client_init_function_t` in AzureIoT.h for details.
 https://stackoverflow.com/questions/76895422/do-my-pico-w-needs-a-ca-certificate-if-i-am-using-a-sas-token
 https://stackoverflow.com/questions/70665647/sending-messages-to-azure-iotcentral-via-mqtt
 https://learn.microsoft.com/en-us/azure/iot/iot-mqtt-connect-to-iot-hub#using-the-mqtt-protocol-directly-as-a-device
 https://github.com/govorox/SSLClient/blob/master/examples/Esp32-Arduino-IDE/mqtt_gsm_SIM800L_Azure_x509_Device_Twin/mqtt_gsm_SIM800L_Azure_x509_Device_Twin.ino

 */
static int mqtt_client_init_function(mqtt_client_config_t *mqtt_client_config, mqtt_client_handle_t *mqtt_client_handle)
{
  LogInfo("DO MQTT CLIENT INIT");
  int result = 1;
  az_span mqtt_broker_uri_span = AZ_SPAN_FROM_BUFFER(mqtt_broker_uri);
  mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, AZ_SPAN_FROM_STR(MQTT_PROTOCOL_PREFIX));
  mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, mqtt_client_config->address);
  az_span_copy_u8(mqtt_broker_uri_span, null_terminator);

 
  // USE WIFI
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));

  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_client_config->port;
  mqtt_config.client_id = (const char *)az_span_ptr(mqtt_client_config->client_id);
  mqtt_config.username = (const char *)az_span_ptr(mqtt_client_config->username);

  #ifdef IOT_CONFIG_USE_X509_CERT
    LogInfo("MQTT client using X509 Certificate authentication");
    mqtt_config.client_cert_pem = ;
    mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
  #else // Using SAS key
    mqtt_config.password = (const char *)az_span_ptr(mqtt_client_config->password);
  #endif

  mqtt_config.keepalive = 500;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = esp_mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char *)ca_pem;

  LogInfo("MQTT client target uri set to '%s'", mqtt_broker_uri);

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    LogError("esp_mqtt_client_init failed.");
    result = 1;
  }
  else
  {
    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK)
    {
      LogError("esp_mqtt_client_start failed (error code: 0x%08x).", start_result);
      result = 1;
    }
    else
    {
      *mqtt_client_handle = mqtt_client;
      result = 0;
    }
  }
  return result;
}

/*
 * See the documentation of `mqtt_client_deinit_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_deinit_function(mqtt_client_handle_t mqtt_client_handle)
{
  int result = 0;
  LogInfo("MQTT client being disconnected.");
  // USE WIFI
  esp_mqtt_client_handle_t esp_mqtt_client_handle = (esp_mqtt_client_handle_t)mqtt_client_handle;

  if (esp_mqtt_client_stop(esp_mqtt_client_handle) != ESP_OK)     LogError("Failed stopping MQTT client.");

  if (esp_mqtt_client_destroy(esp_mqtt_client_handle) != ESP_OK)  LogError("Failed destroying MQTT client.");
    

  if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)   LogError("Failed updating azure iot client of MQTT disconnection.");

  return 0;
}

/*
 * See the documentation of `mqtt_client_subscribe_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_subscribe_function(mqtt_client_handle_t mqtt_client_handle, az_span topic, mqtt_qos_t qos)
{
  LogInfo("MQTT client subscribing to '%.*s'", az_span_size(topic), az_span_ptr(topic));
  int packet_id = 0;
  // As per documentation, `topic` always ends with a null-terminator.
  // USE WIFI
  // esp_mqtt_client_subscribe returns the packet id or negative on error already, so no conversion is needed.
  packet_id = esp_mqtt_client_subscribe((esp_mqtt_client_handle_t)mqtt_client_handle, (const char *)az_span_ptr(topic), (int)qos);

  return packet_id;
}

/*
 * See the documentation of `mqtt_client_publish_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_publish_function(mqtt_client_handle_t mqtt_client_handle, mqtt_message_t *mqtt_message)
{
  LogInfo("MQTT PUBLISH: topic: '%s',QoS:%d, payload:'%.*s'",
          az_span_ptr(mqtt_message->topic),mqtt_message->qos,az_span_size(mqtt_message->payload), az_span_ptr(mqtt_message->payload));

  // USE WIFI
  int msg_id = esp_mqtt_client_publish(
      (esp_mqtt_client_handle_t)mqtt_client_handle,
      (const char *)az_span_ptr(mqtt_message->topic), // topic is always null-terminated.
      (const char *)az_span_ptr(mqtt_message->payload),
      az_span_size(mqtt_message->payload),
      (int)mqtt_message->qos,
      MQTT_DO_NOT_RETAIN_MSG
    );

  return msg_id;
}
/* --- Other Interface functions required by Azure IoT --- */
/*
 * See the documentation of `hmac_sha256_encryption_function_t` in AzureIoT.h for details.
 */
static int mbedtls_hmac_sha256(const uint8_t *key, size_t key_length, const uint8_t *payload, size_t payload_length, uint8_t *signed_payload, size_t signed_payload_size)
{
  (void)signed_payload_size;
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, key_length);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload, payload_length);
  mbedtls_md_hmac_finish(&ctx, (byte *)signed_payload);
  mbedtls_md_free(&ctx);
  return 0;
}

/*
 * See the documentation of `base64_decode_function_t` in AzureIoT.h for details.
 */
static int base64_decode(uint8_t *data, size_t data_length, uint8_t *decoded, size_t decoded_size, size_t *decoded_length)
{
  return mbedtls_base64_decode(decoded, decoded_size, decoded_length, data, data_length);
}

/*
 * See the documentation of `base64_encode_function_t` in AzureIoT.h for details.
 */
static int base64_encode(uint8_t *data, size_t data_length, uint8_t *encoded, size_t encoded_size, size_t *encoded_length)
{
  return mbedtls_base64_encode(encoded, encoded_size, encoded_length, data, data_length);
}


/*
 * See the documentation of `properties_update_completed_t` in AzureIoT.h for details.
 */
static void on_properties_update_completed(uint32_t request_id, az_iot_status status_code, az_span properties)
{
  
  LogInfo("Properties update request completed (id=%d, status=%d)", request_id, status_code);
}

/*
 * See the documentation of `properties_received_t` in AzureIoT.h for details.
 */
void on_properties_received(az_span properties)
{
  LogInfo("Properties update received: %.*s", az_span_size(properties), az_span_ptr(properties));
  LogInfo("Property Recieved");
  if (azure_pnp_handle_properties_update(&azure_iot, properties, properties_request_id++) != 0)
  {
    LogError("Failed handling properties update.");
  }
}

/*
 * See the documentation of `command_request_received_t` in AzureIoT.h for details.
 */
static void on_command_request_received(command_request_t command)
{
  az_span component_name = az_span_size(command.component_name) == 0 ? AZ_SPAN_FROM_STR("") : command.component_name;

  LogInfo("Command request received (id=%.*s, component=%.*s, name=%.*s)",
          az_span_size(command.request_id), az_span_ptr(command.request_id),
          az_span_size(component_name), az_span_ptr(component_name),
          az_span_size(command.command_name), az_span_ptr(command.command_name));


    if(az_span_is_content_equal(command.command_name, AZ_SPAN_FROM_STR("TakeStock"))){
      DX_DO_STOCK_TAKE=true;
    }


  // Here the request is being processed within the callback that delivers the command request.
  // However, for production application the recommendation is to save `command` and process it outside
  // this callback, usually inside the main thread/task/loop.
  (void)azure_pnp_handle_command_request(&azure_iot, command);
}

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static az_iot_hub_client hub_client;
static void get_device_twin_document(void)
{
  int rc;
  LogInfo("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  rc = az_iot_hub_client_twin_document_get_publish_topic(
      &azure_iot.iot_hub_client,
      twin_document_topic_request_id,
      twin_document_topic_buffer,
      sizeof(twin_document_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    LogError("Failed to get the Twin Document topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the twin document request.
  rc = esp_mqtt_client_publish(mqtt_client, twin_document_topic_buffer, 0, 0, 1, 0);
  // int propertyRes = esp_mqtt_client_publish((esp_mqtt_client_handle_t)mqtt_client,twin_document_topic_buffer, 0, NULL, 1, 0);

  LogInfo("Client requesting device twin document from service.");
}


esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  // void esp_mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  // esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

  switch (event->event_id)
  {
    int i, r;

  case MQTT_EVENT_ERROR:
    mqttOnEventError(event);
    break;

  case MQTT_EVENT_CONNECTED:
    mqtt_on_event_connected(event->session_present);
    // LogInfo("MQTT client connected (session_present=%d).", event->session_present);

    // if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
    // {
    //   LogError("azure_iot_mqtt_client_connected failed.");
    // }

    break;
  case MQTT_EVENT_DISCONNECTED:
    mqtt_on_event_disconnected();
    // LogInfo("MQTT client disconnected.");

    // if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
    // {
    //   LogError("azure_iot_mqtt_client_disconnected failed.");
    // }

    break;
  case MQTT_EVENT_SUBSCRIBED:
    mqtt_on_event_subscribed(event->msg_id);
    // LogInfo("MQTT topic subscribed (message id=%d).", event->msg_id);

    // if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, event->msg_id) != 0)
    // {
    //   LogError("azure_iot_mqtt_client_subscribe_completed failed.");
    // }

    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    LogInfo("MQTT EVENT -- topic unsubscribed.");
    break;

  case MQTT_EVENT_PUBLISHED:
    mqtt_on_event_pusblished(event->msg_id);
    // LogInfo("MQTT event MQTT_EVENT_PUBLISHED");

    // if (azure_iot_mqtt_client_publish_completed(&azure_iot, event->msg_id) != 0)
    // {
    //   LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", event->msg_id);
    // }

    break;
  case MQTT_EVENT_DATA:
    mqtt_on_event_data(event->topic, event->topic_len,event->data, event->data_len);
    // LogInfo("MQTT message received.");

    // mqtt_message_t mqtt_message;
    // mqtt_message.topic = az_span_create((uint8_t *)event->topic, event->topic_len);
    // mqtt_message.payload = az_span_create((uint8_t *)event->data, event->data_len);
    // mqtt_message.qos = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.

    // if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
    // {
    //   // %.*s
    //   // by specifying a length, we can get around printing (or sprintf) 'ing a string 
    //   // which has no null terminator, 
    //   // for example a string which is input from any stream or file based source. 
    //   LogError("azure_iot_mqtt_client_message_received failed (topic=%.*s).", event->topic_len, event->topic);
    // }

    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    LogInfo("MQTT EVENT -- before connect -- client connecting");
    break;
  default:
    LogError("MQTT EVENT -- UNKNOWN.");
    break;
  }

  return ESP_OK;
}

void mqtt_on_event_connected(int session_present)
{
  LogInfo("MQTT EVENT - CONNECTED, session_present:%d", session_present);
  DX_LATCH_DISCONNECTION = false;
  // if (!DX_MQTT_CONNECTED) DX_MQTT_CONNECT_EVENT = true;
  // DX_MQTT_CONNECTED = true;

  if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
  {
    LogError("azure_iot_mqtt_client_connected failed.");
  }
}

void mqtt_on_event_disconnected()
{
  if (!DX_LATCH_DISCONNECTION)
  {
    LogInfo("MQTT EVENT - DISCONNECT");
    DX_LATCH_DISCONNECTION = true;
  }
  // if (DX_MQTT_CONNECTED)
  // {
  //   DX_MQTT_CONNECT_EVENT = true;
  // }
  // DX_MQTT_CONNECTED = false;
  if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
  {
    LogError("azure_iot_mqtt_client_disconnected failed.");
  }
  // else
  // {
  
  // }
}

void mqtt_on_event_subscribed(int msg_id)
{
    LogInfo("MQTT EVENT-topic subscribed (message id=%d)", msg_id);
    if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, msg_id) != 0)
    {
      LogError("azure_iot_mqtt_client_subscribe_completed failed");
    }
}

void mqtt_on_event_pusblished(int msg_id)
{
  LogInfo("MQTT EVENT-PUBLISHED,msg_id:%d",msg_id);
  dxOutbox.onPublished(msg_id);
  // calls unused function...
  if (azure_iot_mqtt_client_publish_completed(&azure_iot, msg_id) != 0)
  {
    LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", msg_id);
  }
  
}

void mqtt_on_event_data(char *topic, int topic_len, char *data,int data_len)
{
  LogInfo("MQTT EVENT - message received");

  mqtt_message_t mqtt_message;
  mqtt_message.topic = az_span_create((uint8_t *)topic, topic_len);
  mqtt_message.payload = az_span_create((uint8_t *)data, data_len);
  mqtt_message.qos = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.

  if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
  {
    // %.*s
    // by specifying a length, we can get around printing (or sprintf) 'ing a string 
    // which has no null terminator, 
    // for example a string which is input from any stream or file based source.
    LogError("azure_iot_mqtt_client_message_received failed (topic=%.*s).", topic_len, topic);
  }
}

void mqttOnEventError(esp_mqtt_event_handle_t event)
{
  if (DX_LATCH_ERROR_FLAG) return;
  DX_LATCH_ERROR_FLAG = true;
  dxOutbox.enqueueBoxToStore();  
  LogError("MQTT EVENT - ERROR, esp_tls_stack_err:%d, esp_tls_cert_verify_flags:%d, esp_transport_sock_errno:%d,error_type:%d,connect_return_code=%d",
      event->error_handle->esp_tls_stack_err,
      event->error_handle->esp_tls_cert_verify_flags,
      event->error_handle->esp_transport_sock_errno,
      event->error_handle->error_type,
      event->error_handle->connect_return_code);

    switch (event->error_handle->connect_return_code)
    {
      case MQTT_CONNECTION_ACCEPTED:
        LogError("connect_return_code=MQTT_CONNECTION_ACCEPTED");
        break;

      case MQTT_CONNECTION_REFUSE_PROTOCOL:
        LogError("connect_return_code=MQTT_CONNECTION_REFUSE_PROTOCOL");
        break;

      case MQTT_CONNECTION_REFUSE_ID_REJECTED:
        LogError("connect_return_code=MQTT_CONNECTION_REFUSE_ID_REJECTED");
        break;

      case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
        LogError("connect_return_code=MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE");
        break;

      case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
        LogError("connect_return_code=MQTT_CONNECTION_REFUSE_BAD_USERNAME");
        break;

      case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
        LogError("connect_return_code=MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED");
        break;

      default:
        LogError("connect_return_code=unknown (%d)", event->error_handle->connect_return_code);
        break;
    };

}
/*
================================================================================================
SEND MESSAGE to AZURE with MQTT
================================================================================================
*/
void sendStockTake()
{
  // float product1_mass = 0; //get_product1_mass();
  // float product2_mass = 0; //get_product2_mass();
  // float product3_mass = 0; //get_product3_mass();
  // float product4_mass = 0; //get_product4_mass();
  // LogInfo("takeStockTel,p1:%d,p2:%d,p3:%d,p4:%d",product1Mass,product2Mass,product3Mass,product4Mass);
  // char event[100] = "Stock Take";
  // char product[100] = "Stock Take";
  // char date[100] ="";
  azure_pnp_send_stocktake(&azure_iot, 0, 0, 0, 0);
  // azure_pnp_send_telemetry(&azure_iot, event, product, 999, 999, 999,date,product1Mass,product2Mass,product3Mass,product4Mass);
}

void sendMsgFormI2C(String message) 
{
    String event_str;
    String datetime = getDateTime();
    
    // FORMAT: <1:EVENT-NAME>:<2:product-name>:<3:selected-amount-ml/tk>:"4:dispensed amount-ml":"5:price":
    //      
    // sendtoComs("Finished:"       + p_name + ":" + String(sel_amount_ml) + ":" + String(dx_amount_ml) + ":" + String(price) + ":");
    // sendtoComs("Time Out:"       + p_name + ":" + String(sel_amount_tk) + ":" + String(dx_amount_ml) + ":");
    // sendtoComs("Amount Chosen:"  + p_name + ":" + String(amount_ml) + ":999:" + String(amount_tk) + ":");
    // sendtoComs("Cancel Init:"    + p_name + ":" + String(amount_tk) + ":999:999:")
    // sendtoComs("Started:"        + p_name + ":999:999:999:");

    //event name
    int index     = message.indexOf(':'); // Split the message by ':'
    event_str     = message.substring(0, index);        
    message       = message.substring(index + 1); 
    // product name
    index         = message.indexOf(':');
    String choice = message.substring(0, index);
    message       = message.substring(index + 1);
    // selected amount in TK or ml
    index         = message.indexOf(':');
    int sl_amount = message.substring(0, index).toInt();
    message       = message.substring(index + 1);
    // dispsensed amount in ml
    index         = message.indexOf(':');
    int dx_amount = message.substring(0, index).toInt();
    // price in TK
    int price     = message.substring(index + 1).toInt();
    
    
    // LogInfo("message:%s",message.c_str());
    // LogInfo("eventMsg:%s",eventMsg.c_str());
    // LogInfo("choice:%s",choice.c_str());
    // LogInfo("enamount:%d",enamount);
    // LogInfo("disamount:%d",disamount);
    // LogInfo("price:%d",price);
    // LogInfo("datetime:%s",datetime.c_str());

    // convert Strings to c-strings
    int str_len = event_str.length() + 1;
    char event_c_str[str_len];
    event_str.toCharArray(event_c_str, str_len);

    int str_len_choice = choice.length() + 1;
    char choice_array[str_len_choice];
    choice.toCharArray(choice_array, str_len_choice);

    int str_len_date = datetime.length() + 1;
    char datetime_c_str[str_len_date];
    datetime.toCharArray(datetime_c_str, str_len_date);

    bool is_dispense = isEventDispense(event_c_str);
    LogInfo("parsed I2C msg contents,event:%s, is dispense:%d",event_c_str, is_dispense);

    // send telemetry using specific function
    if (is_dispense)
    {
      DX_LATCH_STOREOUTBOX = false;
      cfgUpdateDxCnt();
      int res = azure_pnp_send_dispense(&azure_iot, choice_array, sl_amount, dx_amount,price,datetime_c_str,dx_cnt);
    }
    else
    {
      azure_pnp_send_status_change(&azure_iot, event_c_str, choice_array, sl_amount, dx_amount,price,datetime_c_str);
    }


}

/*
================================================================================================
AZURE SETUP
================================================================================================
*/
void dxAzureInit()
{
  /*
   * The configuration structure used by Azure IoT must remain unchanged (including data buffer)
   * throughout the lifetime of the sample. This variable must also not lose context so other
   * components do not overwrite any information within this structure.
  */
  LogInfo("start azure init: set azure-iot-config");
  azure_iot_config.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  azure_iot_config.model_id = azure_pnp_get_model_id();
  azure_iot_config.use_device_provisioning = true; // Required for Azure IoT Central.
  azure_iot_config.iot_hub_fqdn = AZ_SPAN_EMPTY;
  azure_iot_config.device_id = AZ_SPAN_EMPTY;

#ifdef IOT_CONFIG_USE_X509_CERT
  azure_iot_config.device_certificate = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT);
  azure_iot_config.device_certificate_private_key = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY);
  azure_iot_config.device_key = AZ_SPAN_EMPTY;
#else
  azure_iot_config.device_certificate = AZ_SPAN_EMPTY;
  azure_iot_config.device_certificate_private_key = AZ_SPAN_EMPTY;
  // azure_iot_config.device_key = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY);
  // LogInfo("dev_num:%d",dev_num);
  // LogInfo("defined-dev_key:%s",IOT_CONFIG_DEVICE_KEY);
  // LogInfo("table  -dev_key:%s",IOT_CONFIG_DEV_KEY_TABLE[dev_num]);
  azure_iot_config.device_key = az_span_create_from_str(const_cast<char*>(IOT_CONFIG_DEV_KEY_TABLE[dev_num]));
  
#endif // IOT_CONFIG_USE_X509_CERT

  azure_iot_config.dps_id_scope = AZ_SPAN_FROM_STR(DPS_ID_SCOPE);
  // azure_iot_config.dps_registration_id = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID); // Use Device ID for Azure IoT Central.
  azure_iot_config.dps_registration_id = az_span_create_from_str(const_cast<char*>(IOT_CONFIG_DEV_ID_TABLE[dev_num]));
  azure_iot_config.data_buffer = AZ_SPAN_FROM_BUFFER(az_iot_data_buffer);
  azure_iot_config.sas_token_lifetime_in_minutes = MQTT_PASSWORD_LIFETIME_IN_MINUTES;
  azure_iot_config.mqtt_client_interface.mqtt_client_init = mqtt_client_init_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_deinit = mqtt_client_deinit_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_subscribe = mqtt_client_subscribe_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_publish = mqtt_client_publish_function;
  azure_iot_config.data_manipulation_functions.hmac_sha256_encrypt = mbedtls_hmac_sha256;
  azure_iot_config.data_manipulation_functions.base64_decode = base64_decode;
  azure_iot_config.data_manipulation_functions.base64_encode = base64_encode;
  azure_iot_config.on_properties_update_completed = on_properties_update_completed;
  azure_iot_config.on_properties_received = on_properties_received;
  azure_iot_config.on_command_request_received = on_command_request_received;

  //make a timer
  DX_TIMER = timerBegin(0, 80, true); //!!!CHANGED
  timerStop(DX_TIMER);

  // check configs, set mem
  azure_iot_init(&azure_iot, &azure_iot_config);
  // change state to start
  azure_iot_start(&azure_iot);

  LogInfo("Azure IoT client initialized,state:%d", azure_iot.state);
}


/*
================================================================================================
PROCESS WIFI
================================================================================================
*/
void dxWifiStartup()
{
  LogInfo("START WIFI");
  wm.setDebugOutput(true);

  //setup WiFi
  WiFi.setAutoReconnect(true);
  WiFi.onEvent(dxWifiOnEvent);

  // if (wm.getWiFiIsSaved())
  // {
  //   LogInfo("wifi try connect with prev cred...");
  //   WiFi.begin(); // connect to prev credentials.
  // }
  // else
  // {
    WiFi.begin("SSID","password");  // connect to hard-coded credentials.
  // } 
}

void dxWifiOnFirstConnect()
{
  LogInfo("wifi-OnFirstTimeConnnect");
  DX_WIFI_FIRST_CONNECT = true;
  dxSynWithNtpServer();
  azure_pnp_init(); // just an empty function, does nothing
  /*
    populates the "azure_iot_config" object
    then 
      azure_iot_init -- set config
      azure-iot-start -- set state
  */
  dxAzureInit();

}


void dxWifiPrintCred()
{
  LogInfo("WiFi cred,ssid:%s,pwd:%s", wm.getWiFiSSID().c_str(),wm.getWiFiPass().c_str());
}

void dxWifiSetCred(const char *ssid, const char *pwd)
{
  LogInfo("set wifi cred, ssid:%s,pwd:%s",ssid, pwd);
  WiFi.begin(ssid, pwd);
}

void dxWifiOnEvent(WiFiEvent_t event)
{
  IPAddress ip;
  uint32_t time_stamp = millis();
  switch(event) 
  {
    case SYSTEM_EVENT_STA_GOT_IP:
      ip = WiFi.localIP();
      if (!DX_WIFI_CONNECTED) LogInfo("WiFi connected,ip:%d.%d.%d.%d,ts:%u", ip[0],ip[1],ip[2],ip[3],time_stamp);
      DX_WIFI_CONNECTED = true;
      if (!DX_WIFI_FIRST_CONNECT)
      {
        LogInfo("wifi connected for first time -- setup azure");
        dxWifiOnFirstConnect();
      }
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (DX_WIFI_CONNECTED)
      {
        LogError("WiFi lost connection,ts:%u",time_stamp);
        dxWifiOnDisconnect();
        DX_WIFI_CONNECTED = false;
      }
      break;

    default:
      // if (verbose) LogInfo("WiFi-event,n:%d,ts:%u", event,time_stamp);
      break;
  }
}

void dxWifiOnDisconnect()
{
  dxOutbox.enqueueBoxToStore();
}
