#ifndef DX_PRODUCT_H_FILE
#define DX_PRODUCT_H_FILE

#include <Arduino.h>
#include "commespcfg.h"

/*
JSON product config from Arduino verion:
-- currently not supported to set product info via IOTcentral properties update
-- future can set over I2C using serial commands (parse I2C using serial parser)

Code pasted here for future reference, the class DxProduct below is from Main ESP

=======================================================================================
platformio.ini:
    bblanchon/ArduinoJson@^7.1.0

=======================================================================================
MAIN
// json product cfgs
#include <ArduinoJson.h>

// --- Product JSON Files ---
JsonDocument Products;
JsonDocument Product1;
JsonDocument Product2;
JsonDocument Product3;
JsonDocument Product4;

//PRODUCT
// String readString;
// char Product1Value[50];
// char Product2Value[50];
// char Product3Value[50];
// char Product4Value[50];
// String Product1Name;
// int Product1Price;
// int Product1Time;


static void exract_val(String product)
{
  int str_len = product.length() + 1; // convert string to char
  char char_arr[str_len];
  product.toCharArray(char_arr, str_len);
  char *tokenName = strtok(char_arr, ":");
  char *productVal = strtok(NULL, ":");
  Products[tokenName] = productVal;
}

//
// See the documentation of `properties_update_completed_t` in AzureIoT.h for details.
//
static void on_properties_update_completed(uint32_t request_id, az_iot_status status_code, az_span properties)
{
  // LogInfo("%.*s", az_span_size(properties), az_span_ptr(properties));
  // 999 id for get desired props
  if (request_id == 999)
  {
    char props;
    LogInfo("Desired properties");

    // This is defenitly not the best way of doing it but its done
    char s[1024];
    snprintf(s, sizeof(s), "%.*s", az_span_size(properties), az_span_ptr(properties));
    LogInfo(s);
    char *desiredProps = strtok(s, "$");
    const char *P1Name = "Product1Name";
    char *Product1Val;

    Product1Val = strstr(desiredProps, P1Name);
    String test = Product1Val;
    test.replace('"', ' ');
    test.replace(" ", "");

    int str_len = test.length() + 1; // convert string to char
    char char_arr[str_len];
    test.toCharArray(char_arr, str_len);
    LogInfo(char_arr);
    String values[15];
    int indexVal = 0;
    char *token = strtok(char_arr, ",");

    while (token != NULL)
    {
      values[indexVal] = token;
      indexVal++;
      token = strtok(NULL, ",");
    }

    for (int x = 0; x < indexVal; x++)
    {
      exract_val(values[x]);
    }


    Product1["name"] = Products["Product1Name"];
    Product2["name"] = Products["Product2Name"];
    Product3["name"] = Products["Product3Name"];
    Product4["name"] = Products["Product4Name"];

    Product1["price"] = Products["Product1Price"];
    Product2["price"] = Products["Product2Price"];
    Product3["price"] = Products["Product3Price"];
    Product4["price"] = Products["Product4Price"];

    Product1["time"] = Products["Product1Time"];
    Product2["time"] = Products["Product2Time"];
    Product3["time"] = Products["Product3Time"];
    Product4["time"] = Products["Product4Time"];
    
    serializeJson(Product1, Serial);
    serializeJson(Product2, Serial);
    serializeJson(Product3, Serial);
    serializeJson(Product4, Serial);
  
  }
  LogInfo("Properties update request completed (id=%d, status=%d)", request_id, status_code);
}


void onReceive(int len)
{
...
 // else if (masterMessage == "Product1Name") // Filter Master messages
  // {
  //   String product1String = Product1["name"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }
  // else if (masterMessage == "Product1Time") // Filter Master messages
  // {
    
  //   String product1String = Product1["time"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }
  // else if (masterMessage == "Product1Price") // Filter Master messages
  // {
  //   String product1String = Product1["price"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product2Name") // Filter Master messages
  // {
  //   String product1String = Product2["name"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product2Time") // Filter Master messages
  // {
  //   String product1String = Product2["time"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product2Price") // Filter Master messages
  // {
  //   String product1String = Product2["price"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product3Name") // Filter Master messages
  // {
  //   String product1String = Product3["name"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product3Time") // Filter Master messages
  // {
  //   String product1String = Product3["time"];
  //   LogInfo(product1String.c_str()); 
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product3Price") // Filter Master messages
  // {
  //   String product1String = Product3["price"];
  //   LogInfo(product1String.c_str());  
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product4Name") // Filter Master messages
  // {
  //   String product1String = Product4["name"];
  //   LogInfo(product1String.c_str());   
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product4Time") // Filter Master messages
  // {
  //   String product1String = Product4["time"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }

  // else if (masterMessage == "Product4Price") // Filter Master messages
  // {
  //   String product1String = Product4["price"];
  //   LogInfo(product1String.c_str());
  //   int str_len = product1String.length() + 1;
  //   char char_array[str_len];
  //   product1String.toCharArray(char_array, str_len);
  //   Wire.slaveWrite((uint8_t *)char_array, str_len); // Adds the string to Slave buffer, sent on Request
  // }
  ...
}

void setup()
{
    ...
  //Product
  Product1["name"] = "Sunsilk Pink";
  Product2["name"] = "Sunsilk Black";
  Product3["name"] = "Dove Hairfall Rescue";
  Product4["name"] = "Lifebuoy Handwash";

  Product1["price"] = 1.3;
  Product2["price"] = 1.3;
  Product3["price"] = 1.3;
  Product4["price"] = 1.3;

  Product1["time"] = 1.2;
  Product2["time"] = 1.2;
  Product3["time"] = 1.2;
  Product4["time"] = 1.2;
  ...
  }

*/

class DxProduct
{
private:
    /// @brief dx_index is counted from 1, but the array are from 0, so this check if index is ok, -1
    /// @param dx_index 
    /// @return 0 if not ok, else index - 1
    uint8_t _getArrayIndex(uint8_t dx_index);

    // uint16_t (*_getDxTable(uint8_t dx_index))[2] {return _all_tables[_getArrayIndex(dx_index)];}

    uint16_t _getSelectedX(uint8_t dx_index,uint8_t amount_index, uint8_t value_index);

    /// @brief Caclulate price ratios in TK/ml for each product for each amount
    void _calcPriceRatios();
    /* Fluid Calibration Values
    Update the following values after calculating the motor on-time per ml 
    dispensed of each product. The calibration process is documented here:
    https://docs.google.com/spreadsheets/d/1RFrTEQL8gR5JihkoM7z2qM6ESyBOF3Ku0LrqBt9l5YI/edit#gid=556128175 
    CALIBRATE: time in seconds to dispense 1ml of Sunsilk Pink, Sunsilk Black,Dove Hairfall Rescue, Lifebuoy Handwash
    */
    //flow rate in ml/second, inverse of _time_per_ml
    //set cmd for flowrates: 20,2.941,2.941,2.941,2.941
    float _ml_per_s[NUM_DX] = {2.941,2.941,2.941,2.941};


    //Volumne (ml) and amount price (tk) table per product
    // product1 - Sunsilk Pink
    // product2 - Sunsilk Black
    // product3 - Dove Hairfall Rescue   
    // product4 - Lifebuoy Handwash
    //set cmd for dx1: 23,1,75,25,100,35,125,40
    uint16_t _product_table[NUM_DX][NUM_AMOUNTS][2] = {
                                                        {{75,25}, {100,35}, {125,40}},
                                                        {{60,30}, {80,40}, {100,50}},
                                                        {{60,30}, {80,40}, {100,50}},
                                                        {{50,40}, {75,60}, {100,80}},
                                                        };
    //price in tk/ml ratio look-up, so that we dont need to calculate on each "getPrice"
    //actuals cacluated on product table load
    float _tk_p_ml[NUM_DX][NUM_AMOUNTS] =   {
                                            {0.333,0.35,0.32},
                                            {0.333,0.35,0.32},
                                            {0.333,0.35,0.32}
                                            };

public:
    /// @brief Get flow rate of product in ml/s
    /// @param dx_index dispenser index from 1 to 4
    /// @return flow rate in ml/s
    float getFlowRate(uint8_t dx_index)   {return _ml_per_s[_getArrayIndex(dx_index)];}

    /// @brief Get the volume dispesned give time passed in milli-seconds
    /// @param dx_index dispenser index from 1 to 4
    /// @param time_passed_ms time passed in ms
    /// @return volume in ml
    uint16_t getVolumeDispensed(uint8_t dx_index, uint32_t time_passed_ms) {return (uint16_t) ceil(getFlowRate(dx_index)*time_passed_ms/1000.0);}

    /// @brief Get price of given volume dispensed
    /// @param dx_index dispenser index from 1 to 4
    /// @param volume_ml volume dispensed ml
    /// @return price in TK
    uint16_t getPriceOfVolumeDx(uint8_t dx_index,uint16_t volume_ml);
    uint16_t getSelectedVolumeML(uint8_t dx_index,uint8_t amount_index) {return _getSelectedX(dx_index,amount_index,0);}
    uint16_t getSelectedVolumeTK(uint8_t dx_index,uint8_t amount_index) {return _getSelectedX(dx_index,amount_index,1);}
    void getCfg();

    // float getPrice(uint8_t dx_index, float amount_ml);
    // uint16_t getVolume(uint8_t array_index, float priceTK);
    void getAmounts(uint8_t dx_index, uint8_t amount_index,uint16_t &amount_tk, uint16_t &amount_ml);
    String *getName(uint8_t dx_index) {return product_name_lu[_getArrayIndex(dx_index)];}
    String *getCat(uint8_t dx_index) {return product_cat_lu[_getArrayIndex(dx_index)];}
    String *getIngredients(uint8_t dx_index) {return product_ing_lu[_getArrayIndex(dx_index)];}
    uint16_t getSavings(uint8_t dx_index,uint16_t price_tk) {return (uint16_t) ceil(price_tk * 0.1);}

    // void cfgIOflowRate(DxCfgDirection dir_flag);
    // void cfgIOTable(DxCfgDirection dir_flag);
    void cfgIOsaveTable(){}//                           {cfgIOTable(DX_CFG_WRITE);}

    void setFlowRate(float flowrate_1,float flowrate_2,float flowrate_3,float flowrate_4);
    void setTableXY(uint8_t dx_index,uint8_t amount_index, uint16_t ml,uint16_t tk);
  

    //PRODUCT1
    String  product1 = "Sunsilk Pink";
    String  product1Ingredients = "Aqua,SLES,CAPB,Sodium Chloride,Perfume,Carbomer,Preservative,Glycerin,Sodium Hydroxide,Disodium EDTA,Emotives,Color,BDS 1269";
    String  product1Cat = "Shampoo";

    //PRODUCT2
    String  product2 = "Sunsilk Black";
    String  product2Ingredients = "Aqua,SLES,CAPB,Sodium Chloride,Perfume,Carbomer,Preservative,Glycerin,Sodium Hydroxide,Disodium EDTA,Emotives,Color,BDS 1269";
    String  product2Cat = "Shampoo";

    //PRODUCT3
    String  product3 = "Dove Hairfall Rescue";
    String  product3Ingredients = "Aqua,SLES,CAPB,Sodium Chloride,Perfume,Carbomer,Preservative,Glycerin,Sodium Hydroxide,Disodium EDTA,Emotives,Color,BDS 1269";
    String  product3Cat = "Shampoo";

    //PRODUCT4
    String  product4 = "Lifebuoy Handwash";
    String  product4Ingredients = "Aqua,SLES,CMEA,Sodium Chloride,Perfume,Glycol distearate,Carbomer,Preservative,Glycerin,Hydroxy Stearic Acid,Multivitamins,Disodium EDTA,Color,BDS 1973";
    String  product4Cat = "Liquid Soap";

    //LOOK-UP
    String *product_name_lu[4] = {&product1,&product2,&product3,&product4};
    String *product_cat_lu[4] = {&product1Cat, &product2Cat, &product3Cat, &product4Cat};
    String *product_ing_lu[4] = {&product1Ingredients,&product2Ingredients,&product3Ingredients,&product4Ingredients};
  

};



#endif