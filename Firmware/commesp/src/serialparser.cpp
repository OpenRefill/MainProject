/*



*/
#include <Arduino.h>

#include "serialparser.h"


int8_t SerialParser::doStartup(){
    int8_t error_code = 0;
    Serial.begin(SERIAL_BAUD_RATE);
    // uint32_t clean_read_cnt = 0;
    // uint32_t max_read_cnt = 2000;

    return error_code;
}


ComCmds SerialParser::getCurrentCmd()
{
    //https://stackoverflow.com/a/11452972
    return static_cast<ComCmds>(_rx_cmd);
}

float *SerialParser::getRxDataPointer()
{
 return _rx_vals;
}

void SerialParser::clear(int8_t cmd_error)
{
    // if (!_found) Serial.println(COM_SP_ERROR COM_KEY_MSG  "not found");
    
    _new_data = false;
    if (cmd_error != 0){
        _s0.printf(COM_SP_ERROR COM_KEY_MSG "cmd-error,e:%d\n",cmd_error);
        _showParsedData();
    }
    else if (_echo)
    {
        _showParsedData();
    }

    //clear or reset
    _clearRx();
}

// bool SerialParser::hasFoundPort()
// {
//     return _found;
// }

void SerialParser::setEcho(bool active)
{
    _echo = active;
}

// void SerialParser::parseStringCallback(const String &payload)
// {
//     if (!_new_data){
//         _string_parse_busy = true;
//         for (int i = 0; i < payload.length(); i++) {
//                 _recvEndMarker(payload[i]); 
//             }
//         if (_new_data) _string_parse_busy = false;
//     }
//     else{
//         //could not parse
//     }
// }

bool SerialParser::checkSerial()
{
    while (_s0.available() > 0 && !_new_data) {
        _recvEndMarker(_s0.read());
    }
    return _new_data; //has new data that needs to be parsed
}

void SerialParser::_recvEndMarker(char rc)
{
    static byte ndx = 0;
    static byte value_ix = 0;
    _parse_busy = true;
    //end of current value, indicated by e.g., "<x>,<y>", cvt chars <x> to value
    if (rc == SERIAL_RX_DELIM_CHAR) _parseDelimValues(value_ix,ndx);
    //add chars to current value in char array
    else if (rc != SERIAL_RX_END_CHAR) {
        _received_chars[ndx] = rc;
        ndx++; //point to next index
        if (ndx >= SERIAL_NUM_RX_CHARS) ndx = SERIAL_NUM_RX_CHARS - 1; //safety    
    }
    //cmd end marker reached, e.g: <x>,<Y>\n, i.e rx == SERIAL_RX_END_CHAR
    else {
        _parseDelimValues(value_ix,ndx); //cvt current/last char string to value
        _received_chars[ndx] = '\0'; // terminate the string
        ndx = 0;
        value_ix = 0;
        //_rx_index = 0; use clear
        _new_data = true;
        _parse_busy = false; //but new data must first be cleared
    }
    
}

void SerialParser::_parseDelimValues(byte &value_ix, byte &ndx)
{
    _received_chars[ndx] = ' '; //space delim for atoi/atof
    switch (value_ix)
        {
        //convert to int, the cmd number
        case 0:
            if (ndx > 4) _rx_cmd = 0; //limit to small values
            else {
                _rx_cmd = atoi(_received_chars);
                _rx_cmd = constrain(_rx_cmd,0,254);
            }
           
            if (_rx_cmd == 0){ //could be string, instead of "0" cmd
                // if (strncmp(_received_chars, COM_SPECIAL_SEARCH, strlen(COM_SPECIAL_SEARCH)) == 0) 
                //     Serial.println(COM_SPECIAL_FOUND " was found");
            } 
            break;

        //convert to float, parameters
        case 1 ...ARGS_CMDS_SIZE:   //first value start at value_ix 1;
            //_rx_val1 = atof(_received_chars);
            if (_rx_index >= ARGS_CMDS_SIZE) _rx_index = ARGS_CMDS_SIZE-1; //o/w... safety
            //special string value commands
            if (_isStrCmd(_rx_cmd))
            {
                _received_chars[ndx] = '\0'; 
                //limit num of strings, overwrite last
                if (_rx_index >= ARGS_NUM_STRINGS) _rx_index = ARGS_NUM_STRINGS - 1;
                //dest <-- source, stop and null
                strcpy(_string_vals[_rx_index],_received_chars);
            }
            //else assume numeric, convert to float, results in 0 on failed conversion
            else{
                _rx_vals[_rx_index] = atof(_received_chars); 
            }
            _rx_index++;
            break;


        default:
            Serial.printf(COM_SP_ERROR "value_ix:%d\n",value_ix);
            break;
    }
    value_ix += 1;
    if (value_ix > ARGS_CMDS_SIZE+1) value_ix= ARGS_CMDS_SIZE;
    ndx = 0; //to start again
}

void SerialParser::_clearRx()
{
    _rx_cmd = 0;
    _rx_index = 0;
    for (uint8_t i = 0; i < ARGS_CMDS_SIZE; i++)
    {
        _rx_vals[i] = 0;
    }
    
    for (uint8_t i = 0; i < ARGS_NUM_STRINGS; i++)
    {
        _string_vals[i][0] = '\0';
    } 
}



void SerialParser::_showParsedData()
{
    Serial.printf(COM_SP_RX "cmd:%d",_rx_cmd);
    for (uint8_t i = 0; i < _rx_index; i++) Serial.printf(",V%d:%.1f",i,_rx_vals[i]);
    Serial.print("\n");
    
}

bool SerialParser::_isStrCmd(uint8_t cmd)
{
    bool is_str_cmd = false;
    for (size_t i = 0; i < _NUM_STR_CMDS; i++)
    {
        is_str_cmd = cmd == _str_cmds[i];
        if (is_str_cmd) break;
    }
    
    return is_str_cmd;
}