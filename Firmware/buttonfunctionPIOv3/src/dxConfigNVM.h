#ifndef DX_CONFIG_NVM_H
#define DX_CONFIG_NVM_H
// #include "dxConfig.h"
// #include "serialparser.h"



//==============
#define PREF_NAMESPACE_FLOWRATE   "flowrate" //flow rate ml/s
#define PREF_KEY_FLOWRATE_P1            "p1"
#define PREF_KEY_FLOWRATE_P2            "p2"
#define PREF_KEY_FLOWRATE_P3            "p3"
#define PREF_KEY_FLOWRATE_P4            "p4"

//==============
#define PREF_NAMESPACE_VRANGE       "vrange"
#define PREF_KEY_VRANGE_BTNS            "btns"
#define PREF_KEY_VRANGE_LIMITS           "lims"

//==============
#define PREF_NAMESPACE_PRODUCT       "product"
#define PREF_KEY_PRODUCT_TABLE            "table" //product:[[ml,tl]x3]x4

//==============
#define PREF_NAMESPACE_DX       "dx"
#define PREF_KEY_DX_PRINTER         "printer"   //printer is enabled
#define PREF_KEY_DX_BUZZER          "buzzer"    //buzzer is enabled


//==============
#define PREF_NAMESPACE_LOCK       "lock" //lock
#define PREF_KEY_LOCK_ID            "lckid"      // lock id


enum DxCfgDirection{
    DX_CFG_READ = 0,
    DX_CFG_WRITE,
};
//func decl.

/// @brief load/save config for flow rates
/// @param dir_flag write or read
/// @param product1 ml/s for Sunsilk Pink
/// @param product2 ml/s for Sunsilk Black
/// @param product3 ml/s for Dove Hairfall Rescue
/// @param product4 ml/s for Lifebuoy Handwash
void cfgFlowRate(DxCfgDirection dir_flag,float flow_rates[NUM_DX]);

/// @brief load/save config for voltage ranges
/// @param dir_flag 
/// @param  vrange_btns for buttons
/// @param  vrange_lims for limit switches
/// @param num_dx 
void cfgVoltageRanges(DxCfgDirection dir_flag,uint16_t vrange_btns[][2], uint16_t vrange_lims[][2], size_t num_dx);

/// @brief load save product amounts table
/// @param dir_flag 
/// @param product_table array for all dispensers, each with [ml,tk] pair per amount
void cfgProductTable(DxCfgDirection dir_flag, uint16_t product_table[][NUM_AMOUNTS][2]);


/// @brief load save product amounts table
/// @param has_printer 
/// @param has_buzzer 
void cfgStateMachine(DxCfgDirection dir_flag, bool &has_printer, bool &has_buzzer);


/// @brief check if device is locked
/// @return true if locked
bool cfgIsLocked();

/// @brief unlock device
/// @param pwd password
/// @return true on success
bool cfgUnlock(const char *pwd);

/// @brief re-lock device for testing
void cfgLock();

#endif