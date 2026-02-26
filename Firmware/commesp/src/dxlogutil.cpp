#include "dxlogutil.h"


// Function to zero-pad a number (e.g., 3 -> "03"), assumes 100 > number >= 0
String zeroPad(int value) 
{
    if (value < 10) return "0" + String(value);
    else            return String(value);
}

String getDateTime()
{
  struct tm *ptm;
  time_t now = time(NULL);
  ptm = gmtime(&now);

  // localtime_r(&now, &timeinfo); // Convert to struct tm
  // Format the time string using String
  int ptm_year = ptm->tm_year + UNIX_EPOCH_START_YEAR;
  String date_time_str = String(ptm_year)         + "/" +
                        zeroPad(ptm->tm_mon + 1)  + "/" +
                        zeroPad(ptm->tm_mday)     + " " +
                        zeroPad(ptm->tm_hour)     + ":" +
                        zeroPad(ptm->tm_min)      + ":" +
                        zeroPad(ptm->tm_sec);

  return date_time_str;
}

/*
================================================================================================
DEBUG
================================================================================================
*/
void logging_function(log_level_t log_level, char const *const format, ...)
{
  Serial.printf("%s",getDateTime().c_str());
  if (log_level == log_level_info)  Serial.print(" [INFO] ");
  else                              Serial.print(" [ERROR] ");

  size_t log_msg_len = 256;
  char message[log_msg_len];
  va_list ap;
  va_start(ap, format);
  int message_length = vsnprintf(message, log_msg_len, format, ap);
  va_end(ap);

  if (message_length < 0) Serial.println("Failed encoding log message (!)");
  else                    Serial.println(message);

}