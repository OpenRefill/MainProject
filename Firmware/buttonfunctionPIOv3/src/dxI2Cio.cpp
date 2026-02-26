#include "dxI2Cio.h"

String DxI2Cio::requestMsg(int slave_address, int message_len)
{
    // Request data from slave device
    String dataReceived = "";
    uint8_t bytesReceived = Wire.requestFrom(slave_address, message_len); // it makes the request here

    // If received more than zero bytes
    if ((bool)bytesReceived)
    { 
        dataReceived = Wire.readStringUntil('}');
        return dataReceived;
    }

    return "failed request";
}

void DxI2Cio::sendtoComs(String message)
{
    Wire.beginTransmission(I2C_DEV_ADDR);       // Address the slave
    Wire.print(message);                        // Add data to buffer
    uint8_t error = Wire.endTransmission(true); // Send buffered data
    //if (error != 0) Serial.printf("endTransmission error: %u\n", error); // Prints if there's an actual error
}

void DxI2Cio::comsOnFinished(String p_name, uint16_t sel_amount_ml, uint16_t dx_amount_ml, uint16_t price)
{
    sendtoComs("Finished:" + p_name + ":" + String(sel_amount_ml) + ":" + String(dx_amount_ml) + ":" + String(price) + ":");
    // sendtoSlave("Finished:" + choice + ":" + String(volEnteredML) + ":" + String(volDispensedML) + ":" + String(price) + ":");
}

void DxI2Cio::comsOnTimeOut(String p_name, uint16_t sel_amount_tk, uint16_t dx_amount_ml)
{
    sendtoComs("Time Out:" + String(p_name) + ":" + String(sel_amount_tk) + ":" + String(dx_amount_ml) + ":");

}

void DxI2Cio::comsOnStarted(String p_name)
{
    sendtoComs("Started:" + String(p_name) + ":999:999:999:"); 
}



void DxI2Cio::comsOnAmountSelected(String p_name, uint16_t amount_ml, uint16_t amount_tk)
{
    sendtoComs("Amount Chosen:" + p_name + ":" + String(amount_ml) + ":999:" + String(amount_tk) + ":");
}


void DxI2Cio::comsOnCancelInit(String p_name, uint16_t amount_tk)
{
    sendtoComs("Cancel Init:" + p_name + ":" + String(amount_tk) + ":999:999:");
}

void DxI2Cio::comsGetReady()
{
    sendtoComs("Ready");
    delay(1000);
    String results = requestMsg(I2C_DEV_ADDR, 16); // Receive data from comms

    _coms_is_rdy = false;
    if (results == "Yes")
    {
        _coms_is_rdy = true;
        Serial.printf(COM_I2C_MSG "req-res:%s\n",results);
    }
}

String DxI2Cio::comsGetDate()
{
    String date = "";  
    sendtoComs("Date");
    delay(1000);
    uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_ADDR, 10); // it makes the request here

    while (Wire.available())
    { // If received more than zero bytes
        uint8_t temp[bytesReceived];
        date = Wire.readStringUntil('\n');
    }
    return date;
}



void DxI2Cio::sendtoDisplay(String message)
{
    Wire.beginTransmission(DISPLAY_ADD);        // Address the slave
    Wire.print(message);                        // Add data to buffer
    uint8_t error = Wire.endTransmission(true); // Send buffered data
    //if (error != 0) Serial.printf("endTransmission error: %u\n", error); // Prints if there's an actual error
}

void DxI2Cio::dispOnDispensing(uint16_t current_amount, uint16_t selected_amount_tk)
{
    String DisMsg = "D:";
      
    if (current_amount < 10) DisMsg += "0";
    DisMsg += String(current_amount) + ":";
    
    if (selected_amount_tk < 10) DisMsg += "0";
    DisMsg += String(selected_amount_tk) + ":";
    
    sendtoDisplay(DisMsg);
}

