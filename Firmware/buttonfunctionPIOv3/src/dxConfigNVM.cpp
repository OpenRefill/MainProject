#include <Preferences.h>
#include "dxConfigMain.h"
#include "dxConfigNVM.h"
#include "dxcfgid.h"

/*
Preferences Type
https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html

Data Type -- Size (bytes)

Bool    bool          1
Char    int8_t        1
UChar   uint8_t       1
Short   int16_t       2
UShort  uint16_t      2
Int     int32_t       4
UInt    uint32_t      4
Long    int32_t       4
ULong   uint32_t      4
Long64  int64_t       8
ULong64 uint64_t      8
Float   float_t       8
Double  double_t      8
String  const char*   variable
String  String        variable
Bytes   uint8_t       variable

getX Methods Default
  Preferences-Type    Default-Return-Value
  Char,               0
  UChar,
  Short,
  UShort,
  Int, 
  UInt,
  Long, 
  ULong,
  Long64, 
  ULong64

  Bool              false

  Float,            NAN
  Double

  String (String)   “”

  String (* buf)    \0

preferences.getX("myKey", myDefault)

*/
//storage of vals
Preferences cfg;

const char* txt_saved = "saved";
const char* txt_loaded = "loaded";

const char* unlock_pin = "17935";

//=======================================================================================================
//DECLARE
template<size_t N>
void _putUint16Array2D(const char* key, uint16_t (*data)[N], size_t length);

void _printUint16Array1D(const char* operation, const char* key_name, const uint16_t* arr, size_t length);

template<size_t N>
void _printUint16Array2D(const char* operation, const char* key_name, const uint16_t (*arr)[N], size_t length);

template<typename Value>
void _processX(DxCfgDirection dir_flag, const char* key, Value &value);

template<size_t N>
void _proccessUint16Array2D(DxCfgDirection dir_flag, const char* key, uint16_t (*data)[N], size_t length);

//PRINT
template<size_t J, size_t K>
void _printUint16Array3D(const char* operation, const char* key_name, uint16_t data[][J][K], size_t length);
void _onNameNotFound(const char* name);

//HELPER
bool openNameSpace(const char* name_space);
//=======================================================================================================
//DEFINE
//WRITE -- PUT VALUE
bool _putX(const char* key, float &value)     {return cfg.putFloat(key,value) != 0;}
bool _putX(const char* key, int32_t &value)   {return cfg.putLong(key,value) == 4;}
bool _putX(const char* key, uint32_t &value)  {return cfg.putULong(key,value) == 4;}
bool _putX(const char* key, uint8_t &value)   {return cfg.putUChar(key,value) != 0;}
bool _putX(const char* key, bool &value)      {return cfg.putBool(key,value) == 1;}
bool _putX(const char* key, const char *value){return cfg.putString(key,value) == strlen(value);}

//READ -- GET VALUE
void _getX(const char *key, float &value)     {value = cfg.getFloat(key,value);}
void _getX(const char *key, int32_t &value)   {value = cfg.getLong(key,value);}
void _getX(const char *key, uint32_t &value)  {value = cfg.getULong(key,value);}
void _getX(const char *key, uint8_t &value)   {value = cfg.getUChar(key,value);}
void _getX(const char *key, bool &value)      {value = cfg.getBool(key,value);}
void _getX(const char *key, char* value)      {cfg.getString(key,value).toCharArray(value,SERIAL_NUM_RX_CHARS);}

//=========================================================
//WRITE -- SPECIAL
// Store a uint16_t arrayT
void _putUint16Array1D(const char* key, const uint16_t* data, size_t length) {
        // Convert uint16_t array to a byte array
        uint8_t* byteData = reinterpret_cast<uint8_t*>(const_cast<uint16_t*>(data));
        cfg.putBytes(key, byteData, length * sizeof(uint16_t));
}

// Store a uint16_t [][2] arrayT
template<size_t N>
void _putUint16Array2D(const char* key, uint16_t (*data)[N], size_t length) {
    // Flatten the 2D array into a 1D array
    uint16_t* flatData = new uint16_t[length * N];
    for (size_t i = 0; i < length; ++i) {
        for (size_t j = 0; j < N; ++j) {
            flatData[i * N + j] = data[i][j];
        }
    }
    // Convert uint16_t array to a byte array
    uint8_t* byteData = reinterpret_cast<uint8_t*>(flatData);
    cfg.putBytes(key, byteData, length * N * sizeof(uint16_t));
    delete[] flatData;
}

// Store a uint16_t [I][J][K] array, I = length
template<size_t J,size_t K>
void _putUint16Array3D(const char* key, uint16_t (*data)[J][K], size_t I) {
    // Flatten the 3D array into a 1D array
    size_t num_elements = I*J*K;
    uint16_t* flatData = new uint16_t[num_elements];
    // Serial.printf(COM_DX_MSG "put163d;N:%u,I:%u,J:%u,K:%u\n",num_elements,I,J,K);
    for (size_t i = 0; i < I; ++i) 
      for (size_t j = 0; j < J; ++j) 
        for (size_t k = 0; k < K; ++k) 
        {
          // Serial.printf(COM_DX_MSG "%u:[%u][%u][%u]:%u:\n",i*J*K + j*K + k,i,j,k,data[i][j][k]);
          flatData[i*J*K + j*K + k] = data[i][j][k];
        }
    // Convert uint16_t array to a byte array
    uint8_t* byteData = reinterpret_cast<uint8_t*>(flatData);
    cfg.putBytes(key, byteData, num_elements * sizeof(uint16_t));
    delete[] flatData;
}

//=========================================================
//READ -- SPECIAL
// Retrieve a uint16_t array
size_t _getUint16Array(const char* key, uint16_t* retrievedData, size_t maxSize) {
    // Retrieve the byte array
    size_t byteLength = cfg.getBytes(key, reinterpret_cast<uint8_t*>(retrievedData), maxSize * sizeof(uint16_t));
    // Convert byte array size back to uint16_t array size
    return byteLength / sizeof(uint16_t);
}

// Retrieve a uint16_t [][2] array
template<size_t N>
size_t _getUint16Array2D(const char* key, uint16_t (*retrievedData)[N], size_t maxSize) {
    // Retrieve the byte array into a flat array
    uint16_t* flatData = new uint16_t[maxSize * N];
    size_t byteLength = cfg.getBytes(key, reinterpret_cast<uint8_t*>(flatData), maxSize * N * sizeof(uint16_t));
    // Convert flat array back to 2D array
    for (size_t i = 0; i < byteLength / (N * sizeof(uint16_t)); ++i) {
        for (size_t j = 0; j < N; ++j) {
            retrievedData[i][j] = flatData[i * N + j];
        }
    }
    delete[] flatData;
    // Convert byte array size back to uint16_t array size
    return byteLength / (N * sizeof(uint16_t));
}

template<size_t I, size_t J>
size_t _getUint16Array3D(const char* key, uint16_t (*retrievedData)[I][J], size_t maxSize) {
    // Calculate the total number of elements in the 3D array
    size_t num_elements = maxSize * I * J;
    // Retrieve the byte array into a flat array
    uint16_t* flatData = new uint16_t[num_elements];
    size_t byteLength = cfg.getBytes(key, reinterpret_cast<uint8_t*>(flatData), num_elements * sizeof(uint16_t));
    // Serial.printf(COM_DX_MSG "get163d;N:%u,L:%u,I:%u,J:%u,K:%u\n",num_elements,byteLength,maxSize,I,J);
    // Convert flat array back to 3D array
    for (size_t k = 0; k < byteLength / (I * J * sizeof(uint16_t)); ++k) {
        for (size_t i = 0; i < I; ++i) {
            for (size_t j = 0; j < J; ++j) {
                // Serial.printf(COM_DX_MSG "[%u][%u][%u]:%u:\n",k,i,j,flatData[k * I * J + i * J + j]);
                retrievedData[k][i][j] = flatData[k * I * J + i * J + j];
            }
        }
    }
    
    delete[] flatData;
    // Convert byte array size back to uint16_t array size
    return byteLength / (I * J * sizeof(uint16_t));
}

template<size_t N>
void _proccessUint16Array2D(DxCfgDirection dir_flag, const char* key, uint16_t (*data)[N], size_t length){
  //check key
  if (!cfg.isKey(key)){
    _onNameNotFound(key);
    dir_flag = DX_CFG_WRITE;
  }
  //READ RANGES
  if (dir_flag == DX_CFG_READ){
    _getUint16Array2D(key,data,length);
    _printUint16Array2D(txt_loaded,key,data,length);  
  }    
  //WRITE RANGES
  else{
    _putUint16Array2D(key,data,length);
    _printUint16Array2D(txt_saved,key,data,length);  
  }
}

template<size_t J, size_t K>
void _proccessUint16Array3D(DxCfgDirection dir_flag, const char* key, uint16_t (*data)[J][K], size_t length){

  //check key
  if (!cfg.isKey(key)){
    _onNameNotFound(key);
    dir_flag = DX_CFG_WRITE;
  }
  //READ RANGES
  if (dir_flag == DX_CFG_READ){
    _getUint16Array3D(key,data,length);
    _printUint16Array3D(txt_loaded,key,data,length);  
  }    
  //WRITE RANGES
  else{
    _putUint16Array3D(key,data,length);
    _printUint16Array3D(txt_saved,key,data,length);  
  }
}
//=========================================================
//check type, compare type of key value with the value to be written or read
bool _checkType(const char *key, float &value)     {return cfg.getType(key) == PT_BLOB;}
bool _checkType(const char *key, int32_t &value)   {return cfg.getType(key) == PT_I32;}
bool _checkType(const char *key, uint32_t &value)  {return cfg.getType(key) == PT_U32;}
bool _checkType(const char *key, uint8_t &value)   {return cfg.getType(key) == PT_U8;}
bool _checkType(const char *key, bool &value)      {return cfg.getType(key) == PT_U8;}
bool _checkType(const char *key, const char *value){return cfg.getType(key) == PT_STR;}

//========================================================================
//PRINT RESULT HELPERS
//========================================================================
void _onCouldNotOpenName(const char* name){Serial.printf(COM_DX_ERROR COM_KEY_MSG "could not open,name:%s\n",name);}
void _onOpenedName(const char* name)      {Serial.printf(COM_DX_MSG "opened:%s,free:%d\n",name,cfg.freeEntries());}
void _onNameNotFound(const char* name)    {Serial.printf(COM_DX_MSG COM_KEY_MSG "key not found, save: %s\n",name);}

void _onXProcessed(const char* process,bool ok, const char* key_name,uint value)       {Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,ok:%d,%s:%d\n", process,ok,key_name,value);}
void _onXProcessed(const char* process,bool ok, const char* key_name,int value)        {Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,ok:%d,%s:%d\n", process,ok,key_name,value);}
void _onXProcessed(const char* process,bool ok, const char* key_name,float value)      {Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,ok:%d,%s:%f\n", process,ok,key_name,value);}
void _onXProcessed(const char* process,bool ok, const char* key_name,const char* value){Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,ok:%d,%s:%s\n", process,ok,key_name,value);}

//print u16 byte array
void _printUint16Array1D(const char* operation, const char* key_name, const uint16_t* arr, size_t length) {
    Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,key:%s,N:%d,VLIST:", operation, key_name, length);
    for (size_t i = 0; i < length; ++i)  Serial.printf("%u ",arr[i]);
    Serial.print("\n");
}

template<size_t N>
void _printUint16Array2D(const char* operation, const char* key_name, const uint16_t (*arr)[N], size_t length) {
    Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,key:%s,N:%d,VLIST:", operation, key_name, length);
    for (size_t i = 0; i < length; i++)  
      for (size_t j = 0; j < N; j++) Serial.printf("%u ", arr[i][j]);
    Serial.print("\n");
}

template<size_t J, size_t K>
void _printUint16Array3D(const char* operation, const char* key_name, uint16_t data[][J][K], size_t length) {
    Serial.printf(COM_DX_MSG COM_KEY_MSG "%s,key:%s,N:%d,VLIST:", operation, key_name, length);
    // size_t num_elements = length*J*K;
    // Serial.printf(COM_DX_MSG "print163d;N:%u,J:%u,K:%u\n",num_elements,J,K);
    for (size_t i = 0; i < length; i++)   
      for (size_t j = 0; j < J; j++)
      {
        for (size_t k = 0; k < K; k++) Serial.printf("%u ",data[i][j][k]);
        // Serial.println();
      }
    Serial.print("\n");
}


//========================================================================
//PROCESS VALUES, either read (get) or write (put) these assumes the prefrence namespace is open
//========================================================================
template <typename Value>
void _processX(DxCfgDirection dir_flag, const char *key, Value &value)
{
  bool ok = true;
  const char* process = "none";
  //check if key is not too long
  if (strlen(key) >= 14){
     Serial.printf(COM_DX_ERROR COM_KEY_MSG "key not valid length >= 14, key:{%s}\n",key);
     ok = false;
  }
  //check if key exists
  else if (!cfg.isKey(key)){
    _onNameNotFound(key);
    dir_flag = DX_CFG_WRITE;
  }
  //check if value type for key is correct
  else if (! _checkType(key,value)){
    Serial.printf(COM_DX_MSG COM_KEY_MSG "WARN-key value type mismatch,{%s}:{%d}\n",key,cfg.getType(key));
    //only warn
    //ok = false;
  }
  if (ok){
    switch (dir_flag)
    {
    case DX_CFG_READ:
        _getX(key,value);
        process = txt_loaded;
      break;
    
    case DX_CFG_WRITE:
        ok = _putX(key,value);
        process = txt_saved;
      break;

    default:
      break;
    }
  }
  _onXProcessed(process,ok,key,value);
}

// OPEN NAME SPACE
bool openNameSpace(const char* name_space)
{
  //namespace (max 15chr) and access: false: r/w access; true: read only
  bool open_ok = cfg.begin(name_space, false); //open name space, r&W
  if (open_ok)  _onOpenedName(name_space);
  else          _onCouldNotOpenName(name_space);
  return open_ok;
}
//========================================================================
//FLOW RATE
//========================================================================
void cfgFlowRate(DxCfgDirection dir_flag, float flow_rates[NUM_DX])
{
  float flowrate_1 = flow_rates[0];
  float flowrate_2 = flow_rates[1];
  float flowrate_3 = flow_rates[2];
  float flowrate_4 = flow_rates[3];
  if (openNameSpace(PREF_NAMESPACE_FLOWRATE)){
    _processX(dir_flag,PREF_KEY_FLOWRATE_P1,flowrate_1);
    _processX(dir_flag,PREF_KEY_FLOWRATE_P2,flowrate_2);
    _processX(dir_flag,PREF_KEY_FLOWRATE_P3,flowrate_3);
    _processX(dir_flag,PREF_KEY_FLOWRATE_P4,flowrate_4);
    cfg.end(); //close name space   

    if (dir_flag == DX_CFG_READ){
        flow_rates[0] = flowrate_1;
        flow_rates[1] = flowrate_2;
        flow_rates[2] = flowrate_3;
        flow_rates[3] = flowrate_4;
    }
  }
}


// void cfgVoltageRanges(DxCfgDirection dir_flag, uint16_t vrange_btns[][2], size_t num_buttons, uint16_t vrange_lims[][2], size_t num_limswitches)
void cfgVoltageRanges(DxCfgDirection dir_flag, uint16_t vrange_btns[][2], uint16_t vrange_lims[][2], size_t num_dx)
{
  if (openNameSpace(PREF_NAMESPACE_VRANGE)){
    //BUTTONS VOLTAGE RANGE
    _proccessUint16Array2D(dir_flag,PREF_KEY_VRANGE_BTNS,vrange_btns,num_dx);
    //LIM SWITCHES RANGES
    _proccessUint16Array2D(dir_flag,PREF_KEY_VRANGE_LIMITS,vrange_lims,num_dx);
    cfg.end(); //close name space   
  }
}

void cfgProductTable(DxCfgDirection dir_flag, uint16_t product_table[][NUM_AMOUNTS][2])
{
  if (openNameSpace(PREF_NAMESPACE_PRODUCT))
  { 
    _proccessUint16Array3D(dir_flag,PREF_KEY_PRODUCT_TABLE,product_table,NUM_DX);
    cfg.end(); //close name space   
  }
}

void cfgStateMachine(DxCfgDirection dir_flag, bool &has_printer, bool &has_buzzer)
{
  if (openNameSpace(PREF_NAMESPACE_DX))
  {
    _processX(dir_flag,PREF_KEY_DX_PRINTER,has_printer);
    _processX(dir_flag,PREF_KEY_DX_BUZZER,has_buzzer);
    cfg.end(); //close name space   
  }
}

bool cfgIsLocked()
{
  char lock_id[30] = "0";
  if (openNameSpace(PREF_NAMESPACE_LOCK))
  { 
    _processX(DX_CFG_READ,PREF_KEY_LOCK_ID,lock_id);
    cfg.end(); //close name space   
  }
  bool is_same = strncmp(lock_id, dx_dev_id, strlen(dx_dev_id)) == 0; //the same str == 0
  return !is_same;
}

bool cfgUnlock(const char *pwd)
{
  bool pin_ok = strncmp(pwd,unlock_pin, strlen(unlock_pin)) == 0; //the same str == 0
  if (pin_ok)
  {
    if (openNameSpace(PREF_NAMESPACE_LOCK))
    { 
      _processX(DX_CFG_WRITE,PREF_KEY_LOCK_ID,dx_dev_id);
      cfg.end(); //close name space   
    }

  }
  return pin_ok;
}

void cfgLock()
{
  char lock_id[30] = "0";
  if (openNameSpace(PREF_NAMESPACE_LOCK))
  {
    _processX(DX_CFG_WRITE,PREF_KEY_LOCK_ID,lock_id);
    cfg.end(); //close name space   
  }
}
