#ifndef DX_TIMER_HELPER_H
#define DX_TIMER_HELPER_H
#include <Arduino.h>

class DxTimerHelper{
    public:
    DxTimerHelper(bool is_active = true, uint32_t period = 100,bool use_millis = true)
    :_active {is_active},_period {period},_use_millis {use_millis}
    {}
    //START TIMER
    void enable()               {setCfg(true);}
    void start()                {enable();}
    void start(uint32_t period) {setCfg(true, period);}
    //STOP TIMER
    void disable()              {setCfg(false);}
    void stop()                 {disable();}

    //SET CFG
    //de/active timer, using preset period
    void setCfg(bool is_active)                 {setCfg(is_active, _period);}
    //set config
    void setCfg(bool is_active, uint32_t period){
                                                    _active = is_active;
                                                    _period = period;
                                                    if (_period == 0) _active = false;
                                                    if (_use_millis) _last_time = millis();
                                                    else             _last_time = micros();
                                                }

    //get
    uint32_t getPeriod()    {return _period;}
    uint32_t getLastTime()  {return _last_time;}
    bool getActive()        {return _active;}

    //return true if period has passed
    bool isPeriodDone() {
                        if (_use_millis) return _isPeriodDone(millis());
                        else             return _isPeriodDone(micros()); 
                        }
    bool isPeriodDone(uint32_t current_time) {return _isPeriodDone(current_time);}
    

    

    private:
    bool _active = false;
    uint32_t _period = 200;
    uint32_t _last_time = 0;
    bool _use_millis = true; //else use micros

    bool _isPeriodDone(uint32_t current_time) 
                                        {
                                            if (!_active) return false;
                                            if ((current_time - _last_time) >= _period){
                                                _last_time = current_time;
                                                return true;
                                            }
                                            return false;
                                        }

};

#endif