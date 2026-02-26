/**
 * MultiButtons Library
 * Copyright (c) 2019 Mickey Chan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Developed for Arduino-ESP32
 * Created by Mickey Chan (developer AT comicparty.com)
 * 
 */
#ifndef MultiButtons_h
#define MultiButtons_h

#include "Arduino.h"
#include "RunningMedian.h"
#include "RunningAverage.h"

// #include "MultiButtons\aewma.h"


#define BTN_READ_PHASE_1ST_PASS  0
#define BTN_READ_PHASE_2ND_PASS  1
const uint8_t BTN_TRIGGER_EDGE_PRESS    = 0;
const uint8_t BTN_TRIGGER_EDGE_RELEASE  = 1;
const uint8_t BTN_TRIGGER_EDGE_MIN_PRESS = 2;

class MultiButtons {
  public:
    typedef void (*callback_t)(MultiButtons *mb, int btnIndex);
    
    /*
     * Constructor
     * Params:
     * pin               GPIO for button array
     * buttonCount       Number of buttons connected
     * arrVoltageRanges  Upper bound and Lower bound of voltage for each button
     * callback          Callback function of actions according to pressed button
     * adcMax            The maxmium value of ADC
     * triggerEdge       Which edge trigger button press event. On press (0) or on release (1)
     */
    MultiButtons(uint8_t pin, 
                uint8_t buttonCount, 
                uint16_t arrVoltageRanges[][2], 
                uint8_t triggerEdge=BTN_TRIGGER_EDGE_RELEASE,
                callback_t callback = nullptr,
                uint16_t adcMax=4095
                );


    ~MultiButtons();

    /*
     * Prepare for button press detection
     * Clear first reading and attach button handler with detect edge
     */
    void begin();

    /*
     * Check analog pin reading for the buttons every 50 milliseconds
     * Should be put in loop()
     */
    void loop();
    
    /*
     * Print analog button pin reading via serial debug console
     * when the reading greater than 100.
     */
    static int printReading(int pin);

    //NEW
    uint8_t getPin() {return (uint8_t) _pin;}
    static uint16_t getADCreading(int pin);
    uint16_t getFilteredReading();
    bool isStdevOK();
    
    void clearBufs();
    void printStats();

    void setAdcStdevMax(float new_max) {if (new_max >= 50) _adcStdevMax = new_max;
                                        // Serial.printf("new std max: %.1f\n",_adcStdevMax);
                                        }

    void setLoopInterval(int new_period){
                                          if (new_period >= 10 && new_period <= 1000) _loopInterval = new_period;
                                          // Serial.printf("new _loopInterval: %d\n",_loopInterval);
                                          }
    //END OF NEW

    static bool isPressingAny(int pin);
    /*
     * Check button is being pressed
     */
    bool isPressing();
    bool isPressing(int btnIndex);

    uint8_t getTriggerEdge();
    bool setTriggerEdge(uint8_t edge);

    bool hasEvent() {return _event_flag;}
    int getEventBtn() {return _event_btn;}
    void resetEvent();
    
  private:
    RunningMedian _median{15}; //filtered data
    RunningAverage _ave{15};
    // LowpassAEWMA _aewma{0.1,0.3,800,0.3};
    uint16_t _last_med = 0;
    uint16_t _last_raw = 0;
    float _last_std = 0;
    float _adcStdevMax = 100;

    uint8_t _pin;
    uint16_t _adcMax = 4095;
    uint16_t **_arrVoltageRanges;
    int _buttonCount = 0;
    callback_t _callback;
    uint8_t _triggerEdge = BTN_TRIGGER_EDGE_PRESS;
    unsigned long _lastLoop = 0;
    int _loopInterval = 25;
    int _readPhase = BTN_READ_PHASE_1ST_PASS;
    int _prevBtn = -1;
    int _lastPressedBtn = -1;

    bool _event_flag = false;
    int _event_btn = -1;

    
    /*
     * Debounce button, check valid button press and
     * pass the pressed button ID to handler function.
     */
    void _getButton();
    
    /*
     * Read analog value from button array pin 4 times and
     * return the avarage value for determine pressed button
     */
    int _readButton();

    //NEW
    uint16_t _filterReading(uint16_t reading);
    uint16_t _adcRead(uint8_t pin_number);
    uint16_t _adcRead();
    void _onEvent();
};

#endif /* MultiButtons_h */
