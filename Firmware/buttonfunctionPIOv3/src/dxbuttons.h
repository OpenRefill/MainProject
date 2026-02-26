#ifndef DX_BUTTONS_H_FILE
#define DX_BUTTONS_H_FILE

#include <Arduino.h>
#include "MultiButtons/MultiButtons.h"

#include "dxConfigMain.h"
#include "dxpinmap.h"
#include "dxConfigNVM.h"



class DxButtons
{
private:
    // static DxButtons* _self;  // pointer to instance
    
    /* Button Voltages 
    Button circuits are arranged such that each button is registered within a unique
    analog voltage range on the corresponding pin. 
    The Analog-to-digital (ADC) converter converts these voltages to values from 0 (0.0V) to 4096 (3.3V). 
    The ADC ranges are defined in the following format:
    {ADC_MIN, ADC_MAX}

    */
   
    // User Interface Buttons
    uint16_t _btnRanges[NUM_DX][2] = {
        {550,850},      // Button 0 - UP
        {1410,1710},    // Button 1 - RED
        {2265,2565},    // Button 2 - DOWN
        {3130,3430}     // Button 3 - GREEN
    }; 

    // Limit Switches
    uint16_t _limsRanges[NUM_DX][2] = {
        {400,800},    // Limit Switch 0 (Dispenser 1 - Sunsilk Pink)
        {1300,1550},  // Limit Switch 1 (Dispenser 2 - Sunsilk Black)
        {2100,2500},  // Limit Switch 2 (Dispenser 3 - Dove Hairfall Rescue )
        {2945,3545}   // Limit Switch 3 (Dispenser 4 - Lifebuoy Handwash)
    };


   
    void _printPressEvent(uint8_t dx_num, int btnIndex,MultiButtons *mb);
    MBtnEnum _getDxNumFromPin(uint8_t pin);

    //mb for each pin
    MultiButtons _dis1{DX_PIN_1, NUM_BTNS, _btnRanges};
    MultiButtons _dis2{DX_PIN_2, NUM_BTNS, _btnRanges};
    MultiButtons _dis3{DX_PIN_3, NUM_BTNS, _btnRanges};
    MultiButtons _dis4{DX_PIN_4, NUM_BTNS, _btnRanges};
    MultiButtons _lims{DX_PIN_LIMS, NUM_BTNS, _limsRanges,BTN_TRIGGER_EDGE_PRESS};

    static const size_t _SIZE_MB_LU = 5;
    MultiButtons *_mb_lu[_SIZE_MB_LU] = {&_dis1,&_dis2,&_dis3,&_dis4,&_lims};

    //callbacks for btn press events
    // static void _cbOnBtnFromDx1(MultiButtons *mb, int btnIndex);


    bool _dxNumOk(uint8_t dx_num) {return dx_num > 0 && dx_num <= NUM_DX;}

    

public:

    void doInit();
    void resetEvent(uint8_t dx_num);     
    BtnNumEnum getEventBtnNum(uint8_t dx_num, bool reset_after = true);


    void cfgIOVranges(DxCfgDirection dir_flag)          {cfgVoltageRanges(dir_flag,_btnRanges,_limsRanges,4);}

    void limsLoop()                                     {_lims.loop();}
    bool limsIsPressing(uint8_t dx_num);
    void limsPrintStats()                               {_lims.printStats();}
    void limsSetRangeX(uint8_t dx_num, uint16_t low, uint16_t high);


    /// @brief process button, check for event
    /// @param dx_num 
    /// @return true on event
    bool btnLoop(uint8_t dx_num);
    
    void btnSetRangeX(uint8_t dx_num, uint16_t low, uint16_t high); 
    
    void getCfg();
    //calibration
    // void calGetBtnStats(MultiButtons *mb, uint16_t num_points,uint16_t &min_val,uint16_t &avg_val, uint16_t &max_val,uint16_t exp_min = 1, uint16_t exp_max = 4095);
    // void calPrintDx(uint8_t dx_num, uint32_t num_points);
    // void calPrintLims(uint32_t num_points);
    // void calAutoDx(uint8_t dx_num, uint32_t num_points);
    // void calAutoLim(uint32_t num_points);
    // void calPrintCurent();
    // void calGetExpMinMax(uint8_t bnt_num, uint16_t &exp_min,uint16_t &exp_max);
    // void calPrintStream(uint8_t mb_num, uint32_t num_points, u_int32_t period);
    void printStream();
    void printStreamFiltered();

};



#endif