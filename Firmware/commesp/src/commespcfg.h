#ifndef COMMESPCFG_H_FILE
#define COMMESPCFG_H_FILE

#define VERSION_COMMESP "0.3.2"



//Time and NTP Settings/
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

#define MODEM_GMT_TIME_ZONE "UTC-2"

#define UNIX_TIME_NOV_13_2017 1510592825
#define UNIX_EPOCH_START_YEAR 1900


//Function Returns
#define RESULT_OK 0
#define RESULT_ERROR __LINE__

//Conversion factor for microseconds to seconds
#define uS_TO_S_FACTOR 1000000UL 

//I2C Addresses
#define I2C_DEV_ADDR        0x55 // I2C address of Motor/IO ESP32 Module
#define IP5306_ADDR         0x75 // I2C address of IP5306 battery power management SoC
#define IP5306_REG_SYS_CTL0 0x00 // Address of SYS_CTL0 register in the IP5306 SoC

// azure buffer
#define AZ_IOT_DATA_BUFFER_SIZE 1500

// mqtt
#define MQTT_PROTOCOL_PREFIX "mqtts://"
#define MQTT_DO_NOT_RETAIN_MSG 0

// Publish 1 message every 3600 seconds (1 hour).
const uint32_t TELEMETRY_FREQUENCY_IN_SECONDS = 3600;

// dispenser and product
const uint8_t NUM_DX = 4;               //number of dispensers
const uint8_t NUM_AMOUNTS = 3;          //number of amounts (preset volumes)

/*
=====================================================================
COMMANDS
=====================================================================
*/
enum ComCmds{
    //DISPENSER COMMANDS
    CMD_DX_ID = 0,                     // 0. print fw version number ()
    // CMD_DX_SET_FEATURES,            // 1. set features ()

    //SET CONFIG
    // CMD_CFG_FLOW_RATE = 20,         // 20. set ml/s; (flowrate_1,flowrate_2,flowrate_3,flowrate_4)
    // CMD_CFG_RANGE_BTN_X,            // 21. set buttons voltages for btn X; (dx_num,low, high)
    // CMD_CFG_RANGE_LIM_X,            // 22. set buttons voltages for lim X; (dx_num, low, high)
    // CMD_CFG_PRODUCT_TABLE,          // 23. set product table for product; (dx_num, vol_1 (ml), price_1 (tk), vol_2 (ml), price_2 (tk), vol_3 (ml), price_3 (tk)) 

    // CMD_CFG_GET_ALL,                // 24. get all cfg

    // do calibrate
    // 40

    // printer interface
    // CMD_PRINT_SIM_DX = 80,  // 80. simulate a dispense completed (int dispenserIndex, int amountDispensedTK, int enteredAmountTK)
    // CMD_PRINT_SCAN_BT,      // 81. scan BT devices    ()
    // CMD_PRINT_CONNECT,      // 82. try to connect to printer again  ()

    // motors
    // CMD_MOTORS_RUNTIME = 100,   // 100. run motors for set time; (uint8 dx_num, uint16_t speed, bool fwd, uint32_t period) 
 
    // ESP or device 
    CMD_ESP_RESET = 120,    // 120. reset ESP ()
    CMD_ESP_UNLOCK,         // 121. unlock device; (char pwd)
    CMD_ESP_LOCK,           // 122. relock device; ()

    // WiFi / MQTT
    CMD_MQTT_SET_WIFI_CRED = 140,   // 140. set SSID and PWD (char SSID, char PWD)
    CMD_MQTT_SET_DEV_ID,            // 141. set machine number -- note machine 1 == 1 (uint number[1..11])

    CMD_MQTT_GET_WIFI_CRED,         // 142. print/get current SSID and PWD ()
    CMD_MQTT_GET_STATE,             // 143. print/get current connection state ()
    CMD_MQTT_GET_DATE,              // 144. print/get current date ()
    CMD_MQTT_GET_DEV_ID,            // 145. print.get machine number ()

    CMD_MQTT_SIM_DX,                // 146. simulate dispense I2C event -- confirm needs to 1 (bool confirm)
    CMD_MQTT_FORCE_RESTART,         // 147. force restart IOT-MQTT -- confirm needs to 1 (bool confirm)


};

/*
================================================================
serial com protocol
================================================================
*/
#define SERIAL_BAUD_RATE 115200

const char SERIAL_RX_END_CHAR = '\n'; //on what is received from host
const char SERIAL_RX_DELIM_CHAR = ','; //on what is receieved from host

static const size_t ARGS_CMDS_SIZE = 10; //max number of parameters for a command: <CMD_NUM>, param1, param2,... param_N
static const size_t ARGS_NUM_STRINGS = 3; //max number of strings, each with max length of SERIAL_NUM_RX_CHARS, <CMD_NUM>, string1..

//max length of a parameters/argument received, length of RX buffer per argument, also max len of string received by serial, max 2 strings
static const size_t SERIAL_NUM_RX_CHARS = 250; 

#define COM_PROTOCOL_DELIM "|" //separate com groups, <GROUP>|<NAME>|<msg>

//predefined keys
#define COM_KEY_MSG "MSG:"

//GROUPS
#define COM_GROUP_SP            "SP"        //serial parser
#define COM_GROUP_DX            "DX"        //dispense messages

//VALUE NAMES
#define COM_NAME_ERROR  COM_PROTOCOL_DELIM "e" COM_PROTOCOL_DELIM
#define COM_NAME_WARN   COM_PROTOCOL_DELIM "w" COM_PROTOCOL_DELIM 
#define COM_NAME_MSG    COM_PROTOCOL_DELIM "m" COM_PROTOCOL_DELIM
#define COM_NAME_VALUE  COM_PROTOCOL_DELIM "v" COM_PROTOCOL_DELIM
#define COM_NAME_CFG    COM_PROTOCOL_DELIM "c" COM_PROTOCOL_DELIM
#define COM_NAME_STATE  COM_PROTOCOL_DELIM "s" COM_PROTOCOL_DELIM 

//PER GROUP
//SERIAL PARSER: 
#define COM_SP_ERROR        COM_GROUP_SP COM_NAME_ERROR 
#define COM_SP_RX           COM_GROUP_SP COM_PROTOCOL_DELIM "rx" COM_PROTOCOL_DELIM

//DISPENSER
#define COM_DX_MSG          COM_GROUP_DX COM_NAME_MSG
#define COM_DX_ERROR        COM_GROUP_DX COM_NAME_ERROR
#define COM_DX_STATE        COM_GROUP_DX COM_NAME_STATE
#define COM_DX_CFG          COM_GROUP_DX COM_NAME_CFG

/*
================================================================
//PREFERENCES
================================================================
*/


// MQTTT NVM OUTBOX
#define PREF_NAMESPACE_OUTBOX "dxmsgbox"
#define PREF_KEY_OUTBOX_LU     "LU"    // look-up



// CLIENT DEVICE ID
#define PREF_NAMESPACE_DEVID "devid"
#define PREF_KEY_DEV_ID_MACHINE_NUM     "devnum" // machine number, e.g. 0-10 (user inputs 1-10, stored as array index though)
#define PREF_KEY_DEV_DX_CNT             "dxcnt"  // count of total number of dispenses, will roll-over at 2^23

#endif