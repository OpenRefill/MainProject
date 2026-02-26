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
#include "Arduino.h"
#include "MultiButtons.h"

MultiButtons::MultiButtons(uint8_t pin, 
                          uint8_t buttonCount, 
                          uint16_t arrVoltageRanges[][2], 
                          uint8_t triggerEdge,
                          callback_t callback, 
                          uint16_t adcMax) 
{
  this->_pin = pin;
  
  this->_arrVoltageRanges = new uint16_t* [buttonCount];
  for (int i = 0; i < buttonCount; i++) {
    this->_arrVoltageRanges[i] = arrVoltageRanges[i];
  }
  
  this->_buttonCount = buttonCount;
  this->_callback = callback;
  this->_adcMax = adcMax;
  this->_triggerEdge = triggerEdge;
}


MultiButtons::~MultiButtons() {
  // release allocated memories
  for (int i = 0; i < this->_buttonCount; i++) {
    delete [] this->_arrVoltageRanges[i];
  }
  delete [] this->_arrVoltageRanges;
}

void MultiButtons::begin() {
  analogRead(this->_pin);
  delay(1);
}

void MultiButtons::loop() {
  unsigned long _curTime = millis();
  if (_curTime - this->_lastLoop >= this->_loopInterval) {
    this->_lastLoop = _curTime;
    this->_getButton();
  }
}

void MultiButtons::_getButton() {
  int _button;
  static uint32_t last_event = 0;
  uint32_t period = 100;

  _button = this->_readButton();
  bool pressed = _button > -1;
  //FIRST PASS
  if (this->_readPhase == BTN_READ_PHASE_1ST_PASS && _button != this->_lastPressedBtn) {
    this->_prevBtn = _button;
    this->_readPhase = BTN_READ_PHASE_2ND_PASS;
    last_event = millis();
  } 
  //SECOND PASS
  else if (this->_readPhase == BTN_READ_PHASE_2ND_PASS) 
  {
    //debounce
    if (millis() - last_event >= period)
    {
      last_event = millis();
      bool reset_phase = true;
      bool is_event = false;
      bool same_button = _button == this->_prevBtn; 
    
      switch (this->_triggerEdge )
        {
          //ON EDGE PRESSING 
          case BTN_TRIGGER_EDGE_PRESS:
            if (same_button && pressed) is_event = true;
            break;

          //ON EDGE RELEASE
          case BTN_TRIGGER_EDGE_RELEASE:
            if (!pressed)  is_event = true;
            break;


        // case BTN_TRIGGER_EDGE_MIN_PRESS:
        //     if (same_button)
        //     {
        //       if (millis() - last_event >= period){
        //         do_callback = true;
        //         reset_phase = true;
        //       }
        //       else reset_phase = false;
        //     }
        //     break;
          
          default:
            break;
        }//switch

      //event/callback on first phase btn, on edge released current button is -1
      // Serial.printf("getBtn:2ndCB:num:%d,cb:%d,pressed:%d\n",_button,do_callback,pressed);
      if (is_event && this->_lastPressedBtn != -1) _onEvent();
      //store last btn
      this->_lastPressedBtn = _button; //can aslo be -1 if triggered on release

      if (reset_phase){
        this->_readPhase = BTN_READ_PHASE_1ST_PASS; // anyway, reset read state
        clearBufs();
      }
    }//debounce
  } //if 2nd pass
}

int MultiButtons::_readButton () {
  // int z, _sum;
  int _button = -1;
  
  uint16_t z = getFilteredReading();
  bool std_ok = isStdevOK();

  // TODO: check ADC value with an array of buttons with value ranges
  if (z == 0 || z > this->_adcMax || !std_ok) _button = -1;
  else 
  {
    for (int i = 0; i < this->_buttonCount; i++) {
      if (z > this->_arrVoltageRanges[i][0] && z < this->_arrVoltageRanges[i][1]) 
      {
        _button = i;
        break;
      }
    }
  }

  return _button;
}

uint16_t MultiButtons::_filterReading(uint16_t reading)
{
  //ingnore low values (not pressed), so not to fill array with zeros.
  _last_raw = reading; //last filtered raw that is
  if (reading > 100)
  {
    
    //filter
    _median.add((float) reading);
    if (_median.isFull()) reading = (uint16_t) round(_median.getMedianAverage(3));
    else if (_median.getCount() > 3) reading = (uint16_t) _median.getMedian();
    //else z = reading;
    _ave.add((float)reading);
    _last_med = reading;
  }
  else{
    reading = 0;
    _median.clear();
    _ave.clear();
    }
  _last_med = reading;
  return reading;
  // return (uint16_t) round(_aewma.update((float) reading));
}

uint16_t MultiButtons::_adcRead(uint8_t pin_number)
{
    return analogRead(pin_number);
}

uint16_t MultiButtons::_adcRead()
{
    return analogRead(this->_pin);
}

void MultiButtons::_onEvent()
{
  int btn_index = this->_lastPressedBtn;
  if (this->_callback != nullptr) _callback(this,btn_index);
  _event_flag = true;
  _event_btn = btn_index;
}

int MultiButtons::printReading(int pin) {
  int z;
  z = analogRead(pin);
  if (z > 100) Serial.println(z);
  return z;
}

uint16_t MultiButtons::getADCreading(int pin)
{
  return  analogRead(pin);
}


bool MultiButtons::isPressingAny(int pin) {
  return (bool)analogRead(pin);
}

bool MultiButtons::isPressing() {
  // int z;
  // z = analogRead(this->_pin);
  uint16_t z = getFilteredReading();
  if (z > 100) {
    return true;
  }
  else return false;
}

bool MultiButtons::isPressing(int btnIndex) {
  
  // z = analogRead(this->_pin);
  uint16_t z = getFilteredReading();
  if (isStdevOK() && z > this->_arrVoltageRanges[btnIndex][0] && z < this->_arrVoltageRanges[btnIndex][1]) {
    return true;
  }
  return false;
}

uint16_t MultiButtons::getFilteredReading()
{
    return _filterReading(_adcRead());
}

bool MultiButtons::isStdevOK()
{
  _last_std = _adcStdevMax+1;
  if (_ave.getCount() >= 3) _last_std = _ave.getStandardDeviation();
  bool ok =  _last_std <= this->_adcStdevMax;
  // if (!ok && stdev != _adcStdevMax+1)  Serial.printf("btn stdev: %.1f <= %.1f\n",stdev,this->_adcStdevMax);
  return ok;
}

void MultiButtons::clearBufs()
{
  _median.clear();
  _ave.clear();
}

void MultiButtons::printStats()
{
  Serial.printf("raw:%d,med:%d,std:%.1f\n",_last_raw, _last_med,_last_std);
}

uint8_t MultiButtons::getTriggerEdge() {
  return this->_triggerEdge;
}

bool MultiButtons::setTriggerEdge(uint8_t edge) {
  if (edge < BTN_TRIGGER_EDGE_PRESS || edge > BTN_TRIGGER_EDGE_RELEASE) {
    return false;
  }
  this->_triggerEdge = edge;
  return true;
}

void MultiButtons::resetEvent()
{
  _event_flag = false;
  _event_btn = -1;
}
