#ifndef DX_CLIENT_H_FILE
#define DX_CLIENT_H_FILE

#include <Arduino.h>
#include <mqtt_client.h>
#include <WiFiManager.h>

#include "iot_configs.h"
#include "commespcfg.h"

// main start, call in setup
void dxClientStartup();
void dxClientPrintState();
void dxClientProcess();
void dxClientStockProcess();
void dxClientSimDx();
void dxClientRestart();
void dxClientGetDevNum();

void dxClientSetDevNum(uint32_t new_dev_num);

// time
void dxSynWithNtpServer();

// mqtt
esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event);
void mqtt_on_event_connected(int session_present);
void mqtt_on_event_disconnected();
void mqtt_on_event_subscribed(int msg_id);
void mqtt_on_event_pusblished(int msg_id);
void mqtt_on_event_data(char *topic, int topic_len, char *data,int data_len);

void dxWifiStartup();
void dxWifiOnFirstConnect();
void dxWifiPrintCred();
void dxWifiSetCred(const char *ssid, const char *pwd);
void dxWifiOnEvent(WiFiEvent_t event);
void dxWifiOnDisconnect();
// void getWifiSSID(char* buffer);
// int dxWifiSignal();

void dxAzureInit();


void sendMsgFormI2C(String message);
void sendStockTake();

#endif