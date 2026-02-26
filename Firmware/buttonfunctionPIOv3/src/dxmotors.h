#ifndef DX_MOTORS_H_FILE
#define DX_MOTORS_H_FILE

#include <Arduino.h>
#include "dxConfigMain.h"
#include "dxpinmap.h"

class DxMotors
{
private:
    //state
    bool _is_running = false;
    bool _fwd = true;
    uint8_t _dx_num = 0; //current dx running, indexed from 1-4

    //run for time period
    uint32_t _time_start = 0;
    u_int32_t _time_period = 0;
    //pins
    const uint8_t _en_pins[NUM_DX] = {
                                    DX_PIN_EN_M1,
                                    DX_PIN_EN_M2,
                                    DX_PIN_EN_M3,
                                    DX_PIN_EN_M4
                                    };

    void _stopIfDirChange(bool fwd); //stop motors if it is running and requires a change in direction
    void _stopIfDiffDx(uint8_t dx_index); //stop motors if new dx_num is different than current 

public:
    void doInit();
    void stop();
    void run(uint8_t dx_index,uint16_t speed = 255, bool fwd = true);
    void reverse(uint8_t dx_index,uint16_t speed = 255)                 {run(dx_index,speed,false);}
    void startRunForTimePeriod(uint8_t dx_index,uint16_t speed, bool fwd, uint32_t period);
    void doRunForPeriod(uint32_t time_stamp);

    bool isRunning()    {return _is_running;}
    bool isDirection(bool direction) {return direction == _fwd;}

    bool getDirection() {return _fwd;}     
};


#endif

