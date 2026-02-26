#ifndef DX_LOG_UTIL_H_FILE
#define DX_LOG_UTIL_H_FILE
#include <Arduino.h>

#include "AzureIoT.h"
#include "commespcfg.h"

//zero pad value
String zeroPad(int value);
// return UTC/GM date-time as string
String getDateTime();
// This is a logging function used by Azure IoT client.
void logging_function(log_level_t log_level, char const *const format, ...);

#endif