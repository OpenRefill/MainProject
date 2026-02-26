// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * Azure_IoT_PnP_Template.cpp implements the IoT Plug and Play template
 * specific for the Espressif ESP32 Azure IoT Kit board.
 */
 
#ifndef AZURE_IOT_PNP_TEMPLATE_H
#define AZURE_IOT_PNP_TEMPLATE_H

#include <string>
#include "AzureIoT.h"

/*
 * @brief     Initializes internal components of this module.
 * @remark    It must be called once by the application, before any other function
 *            call related to Azure IoT.
 */
void azure_pnp_init();

/*
 * @brief     Returns the model id of the IoT Plug and Play template implemented by this device.
 * @remark    Every IoT Plug and Play template has a model id that must be informed by the 
 *            device during Azure IoT provisioning and connection with the Azure IoT Hub.
 * @return    az_span       An `az_span` containing the model id implemented by this module.
 */
const az_span azure_pnp_get_model_id();

/*
 * @brief     Sends the device description to Azure IoT Central.
 * @remark    Azure IoT Central expects the application to send a description of device and its capabilities.
 *            This function generates a description of the Espressif ESP32 Azure IoT Kit and sends it to
 *            Azure IoT Central.
 *
 * @param[in]    azure_iot     A pointer the azure_iot_t instance with the state of the Azure IoT client.
 * @param[in]    request_id    An unique identification number to correlate the response with when
 *                             `on_properties_update_completed` (set in azure_iot_config_t) is invoked.
 * @return       int           0 if the function succeeds, non-zero if any error occurs.
 */
int azure_pnp_send_device_info(azure_iot_t* azure_iot, uint32_t request_id);

/*
 * @brief     Sets with which minimum frequency this module should send telemetry to Azure IoT Central.
 * @remark    `azure_pnp_send_telemetry` is used to send telemetry, but it will not send anything
 *            unless enough time has passed since the last telemetry has been published.
 *            This delay is defined internally by `telemetry_frequency_in_seconds`,
 *            set initially to once every 10 seconds.
 *
 * @param[in]    frequency_in_seconds    Period of time, in seconds, to wait between two consecutive
 *                                       telemetry payloads are sent to Azure IoT Central.
 */
void azure_pnp_set_telemetry_frequency(size_t frequency_in_seconds);

/*
 * @brief     Sends machine state change, i.e., not a stock-take of dispense
 * @remark    The IoT Plug and Play template implemented by this device is specific to the
 *            Espressif ESP32 Azure IoT Kit board, which contains several sensors.
 *            The template defines telemetry data points for temperature, humidity, 
 *            pressure, altitude, luminosity, magnetic field, rolling and pitch angles, 
 *            as well as acceleration. All of these data are read from the board sensors and sent to 
 *            Azure IoT Central when `azure_pnp_send_telemetry` is called.
 *            This function must be called frequently enough, no slower than the frequency set
 *            with `azure_pnp_set_telemetry_frequency` (or the default frequency of 10 seconds).
 *            
 * @param[in]    azure_iot    A pointer to a azure_iot_t instance, previously initialized 
 *                            with `azure_iot_init`.
 *                            
 * return        int          0 on success, non-zero if any failure occurs.
 */
int azure_pnp_send_status_change(azure_iot_t* azure_iot,  const char* event, const char* product, int enteredAmount, int dispensedAmount, int price, const char* date);

/*
 * @brief     Sends STOCKTAKE telemetry implemented by this IoT Plug and Play application to Azure IoT Central.
 * @remark    This is only sent on an stockake "event", not based on prev "telemetry" frequency.
 *            
 * @param[in]    azure_iot      A pointer to a azure_iot_t instance, previously initialized 
 *                              with `azure_iot_init`.
 * 
 * @param[in]    p1_mass               current hopper mass for product 1- 4
 *                              
 * return        int          0 on success, non-zero if any failure occurs.
 */
int azure_pnp_send_stocktake(azure_iot_t* azure_iot,  float p1_mass, float p2_mass, float p3_mass, float p4_mass);


/*
 * @brief     Sends DISPENSE telemetry implemented by this IoT Plug and Play application to Azure IoT Central.
 * @remark    This is only sent on an "event", not based on prev "stock-take" frequency.
 *            
 * @param[in]    azure_iot      A pointer to a azure_iot_t instance, previously initialized 
 *                              with `azure_iot_init`.
 * 
 * @param[in]    product                Product name
 * @param[in]    selected_amount        selected amount
 * @param[in]    dx_amount              dipsensed amount
 * @param[in]    price                  price due/charged
 * @param[in]    date                   date-time stamp
 * @param[in]    dx_number              dispense number (or counter)
 *                              
 * return        int          0 on success, non-zero if any failure occurs.
 */
int azure_pnp_send_dispense(azure_iot_t* azure_iot, const char* product,int selected_amount,int dx_amount, int price,const char* date,uint32_t dx_number);


// resend
// int azure_pnp_resend_dispense(azure_iot_t* azure_iot);

/*
 * @brief     Handles a command when it is received from Azure IoT Central.
 * @remark    This function will perform the task requested by the command received 
 *            (if the command matches the expected name) and sends back a response to 
 *            Azure IoT Central.
 *
 * @param[in]    azure_iot          A pointer to a azure_iot_t instance, previously initialized 
 *                                  with `azure_iot_init`.
 * @param[in]    command_request    The `command_request_t` instance containing the details of the 
 *                                  device command.
 *
 * return        int                0 on success, non-zero if any failure occurs.
 */
int azure_pnp_handle_command_request(azure_iot_t* azure_iot, command_request_t command_request);

/*
 * @brief     Handles a payload with writable properties received from Azure IoT Central.
 * @remark    This function will consume the writable properties update received 
 *            and send back a response to Azure IoT Central.
 * 
 * @param[in]    azure_iot     A pointer to a azure_iot_t instance, previously initialized 
 *                             with `azure_iot_init`.
 * @param[in]    properties    Raw properties writable-properties payload received from Azure.
 * @param[in]    request_id    The request ID of the response that is sent to the Azure IoT Central.
 *                             In Azure IoT Plug and Play, a response to a writable-property update is
 *                             itself a reported-property (device-side property) update, so it gets a 
 *                             a response from Azure with the same request ID provided here as argument.
 *
 * return        int           0 on success, non-zero if any failure occurs.
 */
int azure_pnp_handle_properties_update(azure_iot_t* azure_iot, az_span properties, uint32_t request_id);


bool isEventStockTake(char* event);
bool isEventDispense(char* event);

#endif // AZURE_IOT_PNP_TEMPLATE_H
