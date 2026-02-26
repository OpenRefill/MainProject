/*
 * serial parser class
 * concept:
 *      https://stackoverflow.com/a/36132711
 *      https://forum.arduino.cc/t/easy-way-to-select-serial-serial1-serial2-etc/345865
 *      https://forum.arduino.cc/t/serial-input-basics-updated/382007/3
 *      https://www.forward.com.au/pfod/ArduinoProgramming/SerialReadingParsing/index.html

 * serial.begin:
 *      unsigned long baud, 
 *      uint32_t config, 
 *      int8_t rxPin, 
 *      int8_t txPin, 
 *      bool invert, 
 *      unsigned long timeout_ms, 
 *      uint8_t rxfifo_full_thrhd

*/
#ifndef SERIALPARSER_H
#define SERIALPARSER_H
#include <Arduino.h>
#include <HardwareSerial.h>

#include "commespcfg.h"


class SerialParser{
    public:
    bool checkSerial();

    int8_t doStartup();
    void clear(int8_t cmd_error);

    ComCmds getCurrentCmd();
    float *getRxDataPointer();
    // Function to get the array of strings
    char (*getStringValues())[SERIAL_NUM_RX_CHARS] {return _string_vals;}

    // bool hasFoundPort();
    void setEcho(bool active);

    // void parseStringCallback(const String &payload);
    
    private:
    HardwareSerial &_s0 = Serial;

    void _recvEndMarker(char rc);
    void _parseDelimValues(byte &value_ix, byte &ndx);

    void _clearRx();
    void _showParsedData();

    bool _isStrCmd(uint8_t cmd);
    
    // variables to hold the parsed data
    char _received_chars[SERIAL_NUM_RX_CHARS]; // temporary array for use when parsing
    int _rx_cmd = 0;
    float _rx_vals[ARGS_CMDS_SIZE];
    uint8_t _rx_index = 0;
    // char _rx_str[SERIAL_NUM_RX_CHARS];        //  recv str 
    bool _new_data = false;
    bool _parse_busy = false;
    bool _string_parse_busy = false;

    bool _echo = false;
    bool _print_once = false;

    char _string_vals[ARGS_NUM_STRINGS][SERIAL_NUM_RX_CHARS] = {}; 
    //{}: This is equivalent to manually setting each character in each string to '\0'.

    static const size_t _NUM_STR_CMDS = 1;  
    uint8_t _str_cmds[_NUM_STR_CMDS] = {
                            CMD_MQTT_SET_WIFI_CRED
                            };
};

#endif