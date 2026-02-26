#ifndef DX_I2C_IO_H_FILE
#define DX_I2C_IO_H_FILE

#include <Arduino.h>
#include <Wire.h>

#include "dxConfigMain.h"
/*
Message Formats
 * 
 * Slave Messages, i.e. sendToComms() go to the Comms module. 
 * Format:
 * <eventType>:<productChoice>:<enteredAmount>:<dispensedAmount>:<price>
 * The comms module ignores the value 999 for enteredAmount, dispensedAmount and price
 *
 * Display Messages, i.e. sendToDisplay() go to the Display module.
 * Formats (X is placeholder for numerical digits):
 * 1. X (send just the index of the active dispenser)
 * 2. S:XX (send the savings value for the savings screen)
 * 3. N:XX:XXX: (number changes on UI menu, selecting amount. Order is N:<price>:<volume>:)
 * 4. D:XX:XX: (dispensing progress updates. Order is D:<currentAmount>:<totalAmount>:)
 * 5. MSG (send just a string message. Options: "Press Green", "Push Bottle", "Cancel", "Printing", "Finished")
 
The main ESP (button/motor controller) sends message to the CommsESP using I2C messages. 
(Most of these are forwarded to IOT central using MQTT)

Here are the messages which are sent on dispense completed or early stop:
    dxi2c.comsOnFinished(_selected_product,_selected_amount_ml,_dx_dispensed_ml,_dx_dispensed_price_tk);
    
    this is sent on completed dispense, if the dispensed volume (ml) > 0,  events which send this command:
        dispense completed (volume dispensed >= selected volume OR price >= selected price)
        "dispense cancel" (red button after pause)
        timeout in pause or in state "cancel dispense" (waiting for red button)
    
    the dispensed volume is calculated with: time * flow-rate
    the dispensed price is calculated with: price_lookup_funciton (dispensed_volume)

    the I2C message is structured like this:
        sendtoComs("Finished:" + p_name + ":" + String(sel_amount_ml) + ":" + String(dx_amount_ml) + ":" + String(price) + ":");
    Note: the Event name is "Finished"

The CommsEsp receives this message, and then unpacks it into the telemetry message... (a really cumbersome approach)

Other I2C Events/messages:

Time Out: comsOnTimeOut:  sendtoComs("Time Out:" + String(p_name) + ":" + String(sel_amount_tk) + ":" + String(dx_amount_ml) + ":");
    Only called in "state paused" (paused during dispense) after time period passed (no continue or cancel dispense event)
    Right after sending this event, it sends Finished event
    Time Out event does not contain dispensed price info, only selected price and current dispensed amount

Started : comsOnStarted: sendtoComs("Started:" + String(p_name) + ":999:999:999:");
    Only called on "dispenser activation", when the user presses an initial button on a dispenser (e.g. to select amount)

Amount Chosen : comsOnAmountSelected: sendtoComs("Amount Chosen:" + p_name + ":" + String(amount_ml) + ":999:" + String(amount_tk) + ":");
    Only called when user selected amount (on go to state: wait for "green button", second press)

Cancel Init : comsOnCancelInit: sendtoComs("Cancel Init:" + p_name + ":" + String(amount_tk) + ":999:999:")
    Called on Red Button after "activating" dispenser (during amount selection)
    Called on Red Button in state "waiting for Green button" (second press)

Other I2C Messages: Not sent to Telemetry, but request something from CommsESP:
    Ready : notify the CommsESP of that MainESP is ready (establish I2C comms handshake) but I don't think this used
    Date : request date from CommsESP, used when printing the Receipt with the printer

on dispense finished (either early stop or after selected amount dispensed) the Finished event is sent
*/

class DxI2Cio
{
private:
    bool _coms_is_rdy = true;

public:
    void doInit()                                     {Wire.begin();} // Starting Wire as Master
    String requestMsg(int slave_address, int message_len);
    //COMS INTERFACE
    void sendtoComs(String message);
    void comsOnFinished(String p_name, uint16_t sel_amount_ml, uint16_t dx_amount_ml, uint16_t price);
    void comsOnTimeOut(String p_name,uint16_t sel_amount_tk, uint16_t dx_amount_ml); 
    void comsOnStarted(String p_name); 
    void comsOnAmountSelected(String p_name,uint16_t amount_ml,uint16_t amount_tk);
    void comsOnCancelInit(String p_name,uint16_t amount_tk);
    
    void comsGetReady();
    String comsGetDate();


    //DISPLAY INTERFACE
    void sendtoDisplay(String message);
    void dispOnDispensing(uint16_t current_amount, uint16_t selected_amount_tk);
    void dispOnSavings(uint16_t saved_tk)                           {sendtoDisplay("S:"+saved_tk);}
    void dispOnFinished()                                           {sendtoDisplay("Finished");}
    void dispOnselection(uint16_t amount_tk,uint16_t amount_ml)     {sendtoDisplay("N:" + String(amount_tk) + ":" + String(amount_ml) + ":");}
    void dispOnDxActive(uint8_t dx_index)                           {sendtoDisplay(String(dx_index));}
    void dispPushBottle()                                           {sendtoDisplay("Push Bottle");}
    void dispPrinting()                                             {sendtoDisplay("Printing");}
    void dispCancel()                                               {sendtoDisplay("Cancel");}
    void dispPressGreen()                                           {sendtoDisplay("Press Green");}

    bool isCommsRdy()   {return _coms_is_rdy;}

    
};



#endif