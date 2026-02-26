#ifndef DX_MAIN_H_FILE
#define DX_MAIN_H_FILE

#include <Arduino.h>

#define I2C_DEV_ADDR 0x55   //coms esp i2c address
#define DISPLAY_ADD 0x56    //display esp i2c address

#define BAUD_RATE 115200

// #define OPT_PRINT_LOGO

#define VERSION_BUTTONFUNCTIONPIO "3.4.0"


const uint32_t T_RAW_PERIOD = 30;       //period to send raw values of pins
const size_t NUM_PINS = 5;              //GPIO-buttons x4 and limits x1
const uint8_t NUM_BTNS = 4;             // used to specify the number of buttons per dispenser
const uint8_t NUM_DX = 4;               //number of dispensers
const uint8_t NUM_AMOUNTS = 3;          //number of amounts (preset volumes)

enum DxStates{
    STATE_IDLE = 0,
    STATE_ACTIVE,               // busy with selection of amount on specific dispenser
    STATE_WAIT_BTN_GREEN,       // wait for green ok
    STATE_WAIT_BEFORE_START,    // wait for period before going to wait for trigger
    STATE_WAIT_TRIGGER,         // wait for trigger before starting dispense
    STATE_DISPENSING,           // busy dispensing
    STATE_PRINTING,             // busy printing
    STATE_PAUSED,               // paused in dispense
    STATE_CANCEL,               // cancel dispense before dispense start
    STATE_CANCEL_IN_DX,         // cancel dispense after dispense start
    STATE_RAW_STREAM,           // print raw adc values for btn config
    STATE_MOTOR_TEST,           // run motors by cmd

    STATE_LOCKED,               // device locked, needs unlock 

};

// UI Button Order
enum BtnNumEnum{
    BTN_UP =   0,
    BTN_RED,
    BTN_DOWN,
    BTN_GREEN,
    BTN_NO_BTN = 99
};

//0: none, 1-4: DX, 5
enum MBtnEnum{
    BTN_EVENT_NA = 0,
    BTN_EVENT_DX1,
    BTN_EVENT_DX2,
    BTN_EVENT_DX3,
    BTN_EVENT_DX4,
    BTN_EVENT_LIM,
};

/*
=====================================================================
COMMANDS
=====================================================================
*/
enum ComCmds{
    //DISPENSER COMMANDS
    CMD_DX_ID = 0,                  // 0. print fw version number
    CMD_DX_SET_FEATURES,            // 1. set features; (bool has_printer, bool buzzer)

    //SET CONFIG
    CMD_CFG_FLOW_RATE = 20,         // 20. set ml/s; (flowrate_1,flowrate_2,flowrate_3,flowrate_4)
    CMD_CFG_RANGE_BTN_X,            // 21. set buttons voltages for btn X; (dx_num,low, high)
    CMD_CFG_RANGE_LIM_X,            // 22. set buttons voltages for lim X; (dx_num, low, high)
    CMD_CFG_PRODUCT_TABLE,          // 23. set product table for product; (dx_num, vol_1 (ml), price_1 (tk), vol_2 (ml), price_2 (tk), vol_3 (ml), price_3 (tk)) 

    CMD_CFG_GET_ALL,                // 24. get all cfg

    //do calibrate
    CMD_CAL_PRINT_RAW = 60,      // 66. enters/exits into raw stream mode (active: 1 / inactive: 0)
    // CMD_CAL_PRINT_DX = 60,  //   print dispenser voltage (dispenser number, number of readings to print)
    // CMD_CAL_PRINT_LIMS,     //   print limits  (number of readings to print)
    // CMD_CAL_DX,             //   auto cal btn voltage (dispenser number, number of readings)
    // CMD_CAL_LIM,            //   auto cal lims voltage ( number of readings)
    // CMD_CAL_PRINT_CURRENT,  //   print the current voltage ranges
    // CMD_CAL_PRINT_STREAM,   //   print stream data for plotting (dispenser number: 1-4, 5 limits, num points, sample period delay in ms)
    

    //printer interface
    CMD_PRINT_SIM_DX = 80,  // 80. simulate a dispense completed (int dispenserIndex, int amountDispensedTK, int enteredAmountTK)
    CMD_PRINT_SCAN_BT,      // 81. scan BT devices    
    CMD_PRINT_CONNECT,      // 82. try to connect to printer again  

    //motors
    CMD_MOTORS_RUNTIME = 100,   // 100. run motors for set time; (uint8 dx_num, uint16_t speed, bool fwd, uint32_t period) 
    CMD_MOTORS_STOP,            // 101. stop motors; ()

    //ESP or device 
    CMD_ESP_RESET = 120,    // 120. reset ESP
    CMD_ESP_UNLOCK,         // 121. unlock device; (char pwd)
    CMD_ESP_LOCK,           // 122. relock device; ()

  
};

/*
================================================================
serial com protocol
================================================================
*/

const char SERIAL_RX_END_CHAR = '\n'; //on what is received from host
const char SERIAL_RX_DELIM_CHAR = ','; //on what is receieved from host

static const size_t ARGS_CMDS_SIZE = 10; //max number of parameters for a command: <CMD_NUM>, param1, param2,... param_N
static const size_t ARGS_NUM_STRINGS = 3; //max number of strings, each with max length of SERIAL_NUM_RX_CHARS, <CMD_NUM>, string1..

//max length of a parameters/argument received, length of RX buffer per argument, also max len of string received by serial, max 2 strings
static const size_t SERIAL_NUM_RX_CHARS = 100; 


#define COM_PROTOCOL_DELIM "|" //separate com groups, <GROUP>|<NAME>|<msg>

//predefined keys
#define COM_KEY_MSG "MSG:"

//GROUPS
#define COM_GROUP_SP            "SP"        //serial parser
#define COM_GROUP_DX            "DX"        //dispense messages
#define COM_GROUP_BTNS          "BTN"       //buttons
#define COM_GROUP_I2C           "I2C"       //I2C esp interface
#define COM_GROUP_TP            "TP"        //thermal printer
#define COM_GROUP_MOTORS        "MOT"       //motors
#define COM_GROUP_PRODUCT       "PRD"       //product

//VALUE NAMES
#define COM_NAME_ERROR  COM_PROTOCOL_DELIM "e" COM_PROTOCOL_DELIM
#define COM_NAME_WARN   COM_PROTOCOL_DELIM "w" COM_PROTOCOL_DELIM 
#define COM_NAME_MSG    COM_PROTOCOL_DELIM "m" COM_PROTOCOL_DELIM
#define COM_NAME_VALUE  COM_PROTOCOL_DELIM "v" COM_PROTOCOL_DELIM
#define COM_NAME_CFG    COM_PROTOCOL_DELIM "c" COM_PROTOCOL_DELIM
#define COM_NAME_STATE  COM_PROTOCOL_DELIM "s" COM_PROTOCOL_DELIM 

//MESSAGES

//PER GROUP
//SERIAL PARSER: 
#define COM_SP_ERROR        COM_GROUP_SP COM_NAME_ERROR 
#define COM_SP_RX           COM_GROUP_SP COM_PROTOCOL_DELIM "rx" COM_PROTOCOL_DELIM 

//DISPENSE
#define COM_DX_MSG          COM_GROUP_DX COM_NAME_MSG
#define COM_DX_ERROR        COM_GROUP_DX COM_NAME_ERROR
#define COM_DX_STATE        COM_GROUP_DX COM_NAME_STATE
#define COM_DX_CFG          COM_GROUP_DX COM_NAME_CFG

//BUTTONS
#define COM_BTNS_MSG          COM_GROUP_BTNS COM_NAME_MSG
#define COM_BTNS_ERROR        COM_GROUP_BTNS COM_NAME_ERROR
#define COM_BTNS_CFG          COM_DX_CFG COM_GROUP_BTNS COM_PROTOCOL_DELIM  

//I2C
#define COM_I2C_MSG          COM_GROUP_I2C COM_NAME_MSG

//BT Printer
#define COM_TP_MSG              COM_GROUP_TP COM_NAME_MSG
#define COM_TP_ERROR            COM_GROUP_TP COM_NAME_ERROR

//MOTORS
#define COM_MOTORS_MSG          COM_GROUP_MOTORS COM_NAME_MSG

//PRODUCT
#define COM_PROD_MSG            COM_GROUP_PRODUCT COM_NAME_MSG
#define COM_PROD_CFG            COM_DX_CFG COM_GROUP_PRODUCT COM_PROTOCOL_DELIM 

#endif