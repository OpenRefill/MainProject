#ifndef STATE_HELPER_H
#define STATE_HELPER_H
#include "dxConfigMain.h"

template <class StateType>
class StateHelper
{
private:
    StateType _prev_state;
    StateType _state;
    StateType _queued_state;
    bool _print_on_change;
    const char* _com_header;

    void _print_change(){ Serial.printf("%sprev:%d,new:%d\n",_com_header,_prev_state,_state);}

public:
    StateHelper(StateType init_state, const char* com_header,bool print_on_change = false):
    _state {init_state}, _print_on_change {print_on_change}, _com_header{com_header}
    {
        //_queued_state{init_state},_prev_state{init_state},
        _queued_state = init_state;
        _prev_state = init_state;
        //_print_change();
    }

    StateType state() {return _state;}
    /// @brief append new state to "queue", do not go to state imediately
    /// @param new_state 
    void append(StateType new_state) {_queued_state = new_state;} 
    void apply() { if (_queued_state != _state) setState(_queued_state);}
    void setState(StateType new_state) {_prev_state = _state;
                                        //optional checks
                                        _state = new_state;
                                        _queued_state = new_state;
                                        if (_print_on_change && _prev_state != new_state) _print_change();
                                    }

    //bool hasQueued(){return _queued_state != _state;}
    bool isState(StateType cmp_state){return cmp_state == _state;}

    //~StateHelper();
};



#endif