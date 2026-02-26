#ifndef DX_STATE_MACHINE_H_FILE
#define DX_STATE_MACHINE_H_FILE
#include <Arduino.h>

#include "dxConfigMain.h"
#include "dxConfigNVM.h"
#include "dxindexhelper.h"
#include "dxtimerhelper.h"
#include "statehelper.h"

class DxStateMachine
{
private:
    StateHelper<DxStates>_sh{STATE_IDLE,COM_DX_STATE,true};

    DxIndexHelper _dx_activated;        //dx number currently activated, 0 -- none
    uint16_t _selected_amount_tk = 0;   // entered amount price in TK
    uint16_t _selected_amount_ml = 0;   //selected amount volume in ml
    String _selected_product = "";
    uint8_t _amount_index = 0; 
    uint16_t _dx_dispensed_price_tk = 0;    //dispensed amount price
    uint16_t _dx_dispensed_ml = 0;          //volume dispensed so far in ml
    uint32_t _dx_time_start = 0;            //time when dispense started in ms
    uint16_t _dx_speed = 255;
    float _dx_flow_rate = 1.0;            //flow rate for current product
    float _dx_time_delta = 0;             //time delta in s, for caclulating vol dispened 
    float _dx_time_at_pause = 0;         //time delta in s when dispense was paused

    
    //wait periods in milli-seconds
    uint32_t _period_before_start = 1000;   //1  s, wait for trigger after green press
    uint32_t _period_pause= 20000;          //20 s, wait during pause, before going to complete dispense
    uint32_t _period_wait_event = 30000;    //30 s, wait for btn event, e.g.: after activation, if no event return to idle

    uint32_t _period_dx_update = 400;       //0.4   s, period to update progress to display

    //state machine control
    bool _has_printer = true;   //disable printer for testing purposes
    bool _has_buzzer = true;    //disable buzzer for testing purposes

    DxTimerHelper _timer{};
    DxTimerHelper _updateTimer{};

    void _loadCfgs();

    void _gotoIdleState(); 
    void _gotoActivatedState(uint8_t dx_num);
    void _gotoWaitGreenState();
    void _gotoWaitBeforeStartState();
    void _gotoWaitTriggerState();
    void _gotoDispenseState();
    void _gotoPrintState();
    void _gotoPausedState();
    void _gotoCancelState();
    void _gotoCancelInDxState();

    void _onBtnInActiveState(BtnNumEnum btn_name);
    void _onBtnInWaitGreenState(BtnNumEnum btn_name);
    void _onBtnInCancelState(BtnNumEnum btn_name);
    void _onBtnInCancelInDxState(BtnNumEnum btn_name);
    void _onBtnInPause(BtnNumEnum btn_name);
    
    void _onContinueDispense(uint32_t time_now);
    void _onSetTableCfg(float *args);
    
    int8_t _checkCmdArgs(ComCmds cmd,float *args);

    void _doIdleState();
    void _doDispense(uint32_t time_now);
    void _doDispenseCompleted();
    void _doPrintReceipt(uint16_t savings);
    void _doFinished();
    void _doCheckSerial();
    int8_t _doParseCmd(ComCmds cmd, float *args);

    void _cfgIO(DxCfgDirection dir) {cfgStateMachine(dir,_has_printer,_has_buzzer);}

public:
    void doInit();
    void process(); //process current state

};


#endif