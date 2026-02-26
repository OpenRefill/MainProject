#include "dxproduct.h"

uint8_t DxProduct::_getArrayIndex(uint8_t dx_index)
{
    uint8_t array_index = 0;
    if (dx_index > 0 && dx_index <= NUM_DX){
        array_index = dx_index - 1;
    }        
    return array_index;
}

uint16_t DxProduct::_getSelectedX(uint8_t dx_index, uint8_t amount_index, uint8_t value_index)
{
    amount_index = constrain(amount_index,0,NUM_AMOUNTS-1);
    value_index = constrain(value_index,0,1);
    uint8_t array_index = _getArrayIndex(dx_index);
    return _product_table[array_index][amount_index][value_index];
}

void DxProduct::_calcPriceRatios()
{
    for (size_t dx_i = 0; dx_i < NUM_DX; dx_i++)
    {
        Serial.printf(COM_PROD_MSG COM_KEY_MSG "price tk/ml, dx:%u,ratios:",dx_i+1);
        for (size_t amount_i = 0; amount_i < NUM_AMOUNTS; amount_i++)
        {
            uint16_t ml = _product_table[dx_i][amount_i][0];
            uint16_t tk = _product_table[dx_i][amount_i][1];
            if (ml != 0) _tk_p_ml[dx_i][amount_i] = (float) tk/ml;
            Serial.printf("%.3f ",_tk_p_ml[dx_i][amount_i]);
        }
        Serial.println();
    }
}

uint16_t DxProduct::getPriceOfVolumeDx(uint8_t dx_index, uint16_t volume_ml)
{
    uint8_t array_index = _getArrayIndex(dx_index);
    float ratio = 1.0; //in tk/ml
    uint16_t price = 0; //in TK
    for (size_t i = 0; i < NUM_AMOUNTS; i++)
    {
        //get (volume, price) pair
        uint16_t fixed_vol_ml = _product_table[array_index][i][0];
        uint16_t fixed_price  = _product_table[array_index][i][1];
        //get ratio (price in TK/ml)
        ratio = _tk_p_ml[array_index][i];
        //calculate price = ratio*volume dispensed
        price = (uint16_t) ceil(ratio*volume_ml);
        // Serial.printf(COM_PROD_MSG "get price; ix:%u,dx_ml:%u,fix_ml:%u,fix_tk:%u,dx_tk:%u\n",i,volume_ml,fixed_vol_ml,fixed_price,price);
        //volume dispensed < table volume, use current ratio, price = TK/ml * ml 
        if (volume_ml < fixed_vol_ml) break;
        //exact valume, use exact price
        else if (fixed_vol_ml == volume_ml)
        {
            price = fixed_price;
            break;
        }
        //else, go to next bracket or use max ratio
    }
    
    return price;
}

void DxProduct::getCfg()
{
    for (size_t i = 0; i < NUM_DX; i++)
    {
        for (size_t j = 0; j < NUM_AMOUNTS; j++)
        {
            Serial.printf(COM_PROD_CFG "dx:%u,ax:%u,vol:%u,price:%u\n",i+1,j+1,_product_table[i][j][0],_product_table[i][j][1]);
        }
    }
    Serial.printf(COM_PROD_CFG "flowrate:");
    for (size_t i = 0; i < NUM_DX; i++) Serial.printf(" %.3f",_ml_per_s[i]);
    Serial.print("\n");
}

void DxProduct::getAmounts(uint8_t dx_index, uint8_t amount_index, uint16_t &amount_tk, uint16_t &amount_ml)
{
    amount_tk = getSelectedVolumeTK(dx_index,amount_index);
    amount_ml = getSelectedVolumeML(dx_index,amount_index);
}

void DxProduct::cfgIOflowRate(DxCfgDirection dir_flag)
{
    cfgFlowRate(dir_flag,_ml_per_s);
}

void DxProduct::cfgIOTable(DxCfgDirection dir_flag)
{
  cfgProductTable(dir_flag,_product_table);
  if (dir_flag == DX_CFG_READ) _calcPriceRatios();
 
}

void DxProduct::setFlowRate(float flowrate_1, float flowrate_2, float flowrate_3, float flowrate_4)
{
    _ml_per_s[0] = flowrate_1;
    _ml_per_s[1] = flowrate_2;
    _ml_per_s[2] = flowrate_3;
    _ml_per_s[3] = flowrate_4;
    cfgIOflowRate(DX_CFG_WRITE);
}

void DxProduct::setTableXY(uint8_t dx_index, uint8_t amount_index, uint16_t ml, uint16_t tk)
{
    //this only sets table amount XY, it does not save it to NVM
    uint8_t array_index = _getArrayIndex(dx_index);
    amount_index = constrain(amount_index,0,NUM_AMOUNTS-1);
    _product_table[array_index][amount_index][0] = ml;
    _product_table[array_index][amount_index][1] = tk;
    Serial.printf(COM_PROD_MSG COM_KEY_MSG "setXY,dx:%u,amount:%u,ml:%u,tk:%u\n",dx_index,amount_index,ml,tk);
}
