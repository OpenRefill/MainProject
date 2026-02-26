/*
 * AEWMA Filter - Addaptive Exponentially Weighted Moving Average filter used for smoothing data series readings.
 *
 *      output = alpha * input + (1 - alpha) * last_output
 * 
 *      y_diff = output - input
 *      y_diff_mean = mean(y_diff_hist)
 *      span = max_alpha - min_alpha
 *      alpha = min_alpha + span * y_diff_mean/y_diff_max
 *
 * Where:
 *  -   alpha           = continously updated factor greater than 0 and less or equal to 1
 *  -   input           = current input value
 *  -   last_output     = last filter output value
 *  -   output          = filter output value after the last reading
 *  -   y_diff_mean     = average difference between input and output over N values
 *  -   min_alpha       = alpha minimum: >0 and <=max_alpha
 *  -   max_alpha       = alpha maxium: >min_alpha and <=1.0
 *  -   y_diff_max      = maximum difference between input and output, at this level, alpha is at max.
 * 
 * Based on: jonniezg/EWMA@^1.0.2
 */

#ifndef LOWPASS_AEWMA_H_FILE
#define LOWPASS_AEWMA_H_FILE

#include <Arduino.h>
// #include <RunningAverage.h>

/// @brief EWMA low-pass filter (1st order IIR), with adaptive alpha coefficient.  
class LowpassAEWMA
{
private:
    float _y            = 0;            // current filter output
    float _alpha        = 0.01;         // current filter alpha
    float _alpha_min    = 0.01;         // alpha min value
    float _alpha_max    = 0.03;         // alpha max value
    float _alpha_span   = _alpha_max - _alpha_min;  // span, used to scale y diff
    float _ydiff_alpha  = 0.01;
    float _ydiff        = 0;            // y diff = y-x = output - input
    float _ydiff_max   = 100;           // max diff, at which alpha == max
    float _max_percentage = 1.0;        // used in min(), alpha <= 1.0
    bool _has_initial   = false;        // set x to y, for first x or after filter clear
    bool _delay_active = false;         // only start filtering after a delay time 
    uint32_t _delay_period_ms = 500;    // delay period to wait.
    uint32_t _delay_start_ms = 0;       // start time for delay
    
    // static const uint16_t N_DIFF = 100;
    // RunningAverage _y_diff_hist{N_DIFF};

    /// @brief update alpha, based on mean difference with X over history of N_DIFF points
    /// @param new_y_diff y - x or filter output - input
    void _update_alpha(float new_y_diff)
    {
        // _y_diff_hist.add(new_y_diff);
        // float y_diff_mean = _y_diff_hist.getFastAverage();
        _ydiff = _ydiff_alpha * (new_y_diff - _ydiff) + _ydiff;
        float percentage = min(_max_percentage,abs(_ydiff/_ydiff_max));
        float _alpha = _alpha_min + percentage*_alpha_span;     
    }
public:
    LowpassAEWMA(float alpha_min, float alpha_max, float y_diff_max, float y_diff_alpha){
        setAlphaRange(alpha_min, alpha_max);
        setYdiffMax(y_diff_max);
        setYdiffAlpha(y_diff_alpha);
    }

    void clear(){
        // _y_diff_hist.clear();
        _ydiff = 0;
        _alpha = _alpha_max;
        _has_initial = false;
    }
    void activateOnDelay()
    {
        _delay_active = true;
        _delay_start_ms = millis();
    }
    void setAlphaRange(float new_min, float new_max){
        _alpha_min = constrain(new_min,0.0001,1.0);
        _alpha_max = constrain(new_max,_alpha_min+0.001,1.1);
        _alpha_span = _alpha_max - _alpha_min;
        clear();
    }

    void setYdiffMax(float new_max)     { _ydiff_max = constrain(new_max,0.1,1000);}
    void setYdiffAlpha(float new_alpha) { _ydiff_alpha = constrain(new_alpha,0.0001,1.0);}
    void setDelayPeriod(uint32_t new_dt)   { _delay_period_ms = constrain(new_dt,0,10000);}

    int8_t setAllParamaters(float alpha_min,
                            float alpha_max, 
                            float y_diff_max,
                            float y_diff_alpha,
                            uint32_t delay_period = 0)
    {
        int8_t error_code = 0;
        bool alpha_min_ok = alpha_min > 0.0 && alpha_min <= 0.5;
        bool alpha_max_ok = alpha_max > alpha_min && alpha_max <= 0.5;
        bool ydiff_max_ok = y_diff_max > 0;
        bool ydiff_alpha_ok = y_diff_alpha > 0.0 && y_diff_alpha <= 0.5; 
        if (alpha_min_ok && alpha_max_ok && ydiff_max_ok && ydiff_alpha_ok)
        {
          setAlphaRange(alpha_min,alpha_max);
          setYdiffMax(y_diff_max);
          setYdiffAlpha(y_diff_alpha);
          setDelayPeriod(delay_period);
        }
        else error_code = -2;
        return error_code;
    }

    float update(float x)
    {
        if (_has_initial) {
            _y = _alpha * (x - _y) + _y;
        } 
        else {
            _y = x;
            _has_initial = true;
        }
        _update_alpha(_y-x);
        return _y;
    }

    float updateAfterDelay(float x)
    {
        if (_delay_active)
        {
            if ((millis() - _delay_start_ms) >= _delay_period_ms)
            {
                _delay_active = false;
                return update(x);
            }
            return x;
        }
        else return update(x);

    }
    float getAlpha()        {return _alpha;}
    float getAlphaMax()     {return _alpha_max;}
    float getAlphaMin()     {return _alpha_min;}
    float getYdiffMax()     {return _ydiff_max;}
    float getYdiffAlpha()   {return _ydiff_alpha;}
    uint32_t getDelayPeriod()  {return _delay_period_ms;}
    
};





#endif