#include "dxbuttons.h"

#include <RunningAverage.h>
#include <RunningMedian.h>

void DxButtons::_printPressEvent(uint8_t dx_num, int btn_index, MultiButtons *mb)
{
  String btn_name = "??";
  switch (btn_index)
  {
    case BTN_GREEN:
        btn_name = "GREEN";
        break;
    case BTN_RED:
        btn_name = "RED";
        break;  
    case BTN_UP:
        btn_name = "UP";
        break;
    case BTN_DOWN:
        btn_name = "DOWN";
        break;
  default:
    break;
  }

  Serial.printf(COM_BTNS_MSG "btn:%s,num:%d\n",btn_name, btn_index);
  //mb->printStats();
}

MBtnEnum DxButtons::_getDxNumFromPin(uint8_t pin)
{
    MBtnEnum dx_source = BTN_EVENT_NA;
    switch (pin)
    {
    case DX_PIN_1:
        dx_source = BTN_EVENT_DX1;
        break;

    case DX_PIN_2:
        dx_source = BTN_EVENT_DX2;
        break;

    case DX_PIN_3:
        dx_source = BTN_EVENT_DX3;
        break;

    case DX_PIN_4:
        dx_source = BTN_EVENT_DX4;
        break;

    case DX_PIN_LIMS:
        dx_source = BTN_EVENT_LIM;
        break;

    default:
        break;
    }
    return dx_source;
}


void DxButtons::doInit()
{
    // Prepare reading button state
    for (size_t i = 0; i < _SIZE_MB_LU; i++)_mb_lu[i]->begin();

}

void DxButtons::resetEvent(uint8_t dx_num)
{
    if (dx_num > 0 && dx_num <= _SIZE_MB_LU){ 
        dx_num--;
        _mb_lu[dx_num]->resetEvent();
    }
}

BtnNumEnum DxButtons::getEventBtnNum(uint8_t dx_num,bool reset_after)
{
    BtnNumEnum btn_name = BTN_NO_BTN;
    if (dx_num > 0 && dx_num <= _SIZE_MB_LU){ 
        MultiButtons *mb = _mb_lu[dx_num - 1];
        int btn_num = mb->getEventBtn();
        if (btn_num >= 0 && btn_num < BTN_NO_BTN){
            btn_name = (BtnNumEnum) btn_num;
        }
        //print
        MBtnEnum dx_source = _getDxNumFromPin(mb->getPin());
        _printPressEvent(dx_source,btn_num,mb);

        //reset
        if (reset_after)  resetEvent(dx_num);
    }
    return btn_name;
}

bool DxButtons::btnLoop(uint8_t dx_num)
{
    bool had_event = false;
    if (_dxNumOk(dx_num)){
        MultiButtons *mb = _mb_lu[dx_num-1];
        mb->loop();
        had_event = mb->hasEvent();
        if (had_event) Serial.printf(COM_BTNS_MSG COM_KEY_MSG "event,dx:%d\n",dx_num);
    }
    return had_event;
}

bool DxButtons::limsIsPressing(uint8_t dx_num)
{
  if (_dxNumOk(dx_num)) return _lims.isPressing(dx_num-1);
  
  return false;
}

void DxButtons::limsSetRangeX(uint8_t dx_num, uint16_t low, uint16_t high)
{
    if (_dxNumOk(dx_num))
    {   
        uint8_t array_index =  dx_num - 1;
        _limsRanges[array_index][0] = low;
        _limsRanges[array_index][1] = high;
        Serial.printf(COM_BTNS_MSG COM_KEY_MSG "set lim range,dx_num:%u;low:%u,high:%u\n",dx_num,low,high);
        cfgIOVranges(DX_CFG_WRITE);
    }
    else Serial.printf(COM_BTNS_ERROR COM_KEY_MSG "could not set lim range, dx_num:%u\n",dx_num);
}



void DxButtons::btnSetRangeX(uint8_t dx_num, uint16_t low, uint16_t high)
{
    if (_dxNumOk(dx_num))
    {   
        uint8_t array_index =  dx_num - 1;
        _btnRanges[array_index][0] = low;
        _btnRanges[array_index][1] = high;
        Serial.printf(COM_BTNS_MSG COM_KEY_MSG "set btn range,dx_num:%u;low:%u,high:%u\n",dx_num,low,high);
        cfgIOVranges(DX_CFG_WRITE);
    }
    else Serial.printf(COM_BTNS_ERROR COM_KEY_MSG "could not set btn range, dx_num:%u\n",dx_num);
}

void DxButtons::getCfg()
{
  for (size_t i = 0; i < NUM_BTNS; i++)
  {
    Serial.printf(COM_BTNS_CFG "btn:%u,low:%u,high:%u\n",i+1,_btnRanges[i][0],_btnRanges[i][1]);
  }
  for (size_t i = 0; i < NUM_DX; i++)
  {
    Serial.printf(COM_BTNS_CFG "lim:%u,low:%u,high:%u\n",i+1,_limsRanges[i][0],_limsRanges[i][1]);
  }
  
  
}

// void DxButtons::calGetBtnStats(MultiButtons *mb, uint16_t num_points, uint16_t &min_val, uint16_t &avg_val, uint16_t &max_val, uint16_t exp_min, uint16_t exp_max)
// {
//   float stdev = 0;
//   uint32_t reading_num = 0;
//   uint16_t reading = 0;
//   uint16_t new_min_val = 10000;
//   uint16_t new_max_val = 0;
//   uint16_t min_val_input = min_val;
//   uint16_t max_val_input = max_val;

//   size_t buf_size = 20;
//   RunningAverage adc_buf(buf_size);
//   RunningMedian minval_array(num_points);
//   RunningMedian maxval_array(num_points);

  
//   adc_buf.clear();
//   while (reading_num < num_points)
//     {
//         reading = mb->getFilteredReading();
//         if (reading > 0)
//         {
//             adc_buf.add((float) reading);
           
//             if (adc_buf.bufferIsFull())
//             {
//                 avg_val = (uint16_t) adc_buf.getAverage();
//                 new_min_val = (uint16_t) adc_buf.getMin();
//                 new_max_val = (uint16_t) adc_buf.getMax();
//                 stdev = adc_buf.getStandardDeviation();
//                 if (new_min_val < min_val && new_min_val >= exp_min) min_val = new_min_val;
//                 if (new_max_val > max_val && new_max_val <= exp_max) max_val = new_max_val;
//                 bool stdev_ok = mb->isStdevOK();
//                 //reject reading -- out of range or not stable
//                 if (!stdev_ok || avg_val < exp_min || avg_val > exp_max){
//                   Serial.print(COM_BTNS_MSG "reject value: ");
//                   if (!stdev_ok) Serial.println("stdev too big");
//                   else Serial.println(COM_BTNS_MSG "avg outside of exp min-max bounds");
//                   stdev_ok = false;
//                 }
//                 else{
//                   reading_num++;
//                   minval_array.add(min_val);
//                   maxval_array.add(max_val);
//                   min_val = min_val_input;
//                   max_val = max_val_input;
                
//                 }
//                 adc_buf.clear();
//                 mb->clearBufs(); 
//                 Serial.printf(COM_BTNS_MSG "ok: %d, average: %u +- %.1f (min:%u, max:%u) [%u/%u]\n",stdev_ok,avg_val,stdev,min_val,max_val,reading_num,num_points);
                
//             }
          
//         }
//         delay(25);
//     } //done while
//   //
//   //calc med min and max
//   min_val = (uint16_t) minval_array.getMedian();
//   max_val = (uint16_t) maxval_array.getMedian();
// }

// void DxButtons::calPrintDx(uint8_t dx_num, uint32_t num_points)
// {
//     uint16_t min_val = 10000;
//     uint16_t avg_val = 0;
//     uint16_t max_val = 0;

//     if (dx_num == 0) dx_num = 1; //index -->0
//     if (dx_num >= 5) dx_num = 4; //index --> 3

//     calGetBtnStats(_mb_lu[dx_num-1],num_points,min_val,avg_val,max_val);
// }

// void DxButtons::calPrintLims(uint32_t num_points){
  
//   uint16_t min_val = 10000;
//   uint16_t avg_val = 0;
//   uint16_t max_val = 0;

//   calGetBtnStats(_mb_lu[4],num_points,min_val,avg_val,max_val);

// }  
// void DxButtons::calAutoDx(uint8_t dx_num, uint32_t num_points){
//   uint32_t reading_num = 0;
//   // uint8_t pin_number = DX_PIN_1;
//   uint16_t min_val = 10000;
//   uint16_t avg_val = 0;
//   uint16_t max_val = 0;
//   uint16_t prev_max_val = 0;
//   uint16_t exp_min,exp_max;
//   if (dx_num == 0) dx_num = 1;
//   if (dx_num >= 5) dx_num = 4; //index --> 3
//   const char *btn_names[] = {"UP","RED","DOWN","GREEN"};



//   Serial.println("========================");
//   Serial.printf("calibrate dispener number: %d\n",dx_num); 
//   for (size_t btn_num = 0; btn_num < 4; btn_num++){
//     if (btn_num > 0) {
//       Serial.println("GO TO NEXT BUTTON");
//       delay(5000);
//       }
//     Serial.printf("button: %d: %s\n",btn_num,btn_names[btn_num]);
//     min_val = max(prev_max_val,(uint16_t) _btnRanges[btn_num][0]);
//     max_val = (uint16_t) _btnRanges[btn_num][1];
//     Serial.printf("current min: %d, max: %d\n",min_val,max_val);
//     Serial.printf("Press button %s down...\n",btn_names[btn_num]);
//     delay(1000);
    
//     Serial.println("...measuring");
//     calGetExpMinMax(btn_num,exp_min,exp_max);
//     min_val = min(max(min_val,exp_min),exp_max);
//     max_val = max(min(exp_max,max_val),exp_min);
//     calGetBtnStats(_mb_lu[dx_num-1],num_points,min_val,avg_val,max_val,exp_min,exp_max);
  
//     //offset min/max
//     //min_val += 10
//     //print
//     Serial.printf("-- min: %d, max: %d\n", min_val,max_val);
//     _btnRanges[btn_num][0] = (int) min_val;
//     _btnRanges[btn_num][1] = (int) max_val;
//     prev_max_val = max_val;
//     Serial.println("---------------------");
    
//   }//for each btn

//   //done
//   cfgIOVranges(DX_CFG_WRITE);
//   Serial.println("========================");
// }   

// void DxButtons::calAutoLim(uint32_t num_points){
//   uint16_t min_val = 10000;
//   uint16_t avg_val = 0;
//   uint16_t max_val = 0;
//   uint16_t prev_max_val = 0;
//   uint16_t exp_min,exp_max;

//   Serial.println("========================");
//   for (size_t btn_num = 0; btn_num < 4; btn_num++){
//      if (btn_num > 0) {
//       Serial.println("GO TO NEXT SWITCH");
//       delay(5000);
//       }
//     Serial.printf("calibrate limit switch: %d\n",btn_num);
//     min_val = max(prev_max_val,(uint16_t) _limsRanges[btn_num][0]);
//     max_val = (uint16_t) _limsRanges[btn_num][1];
//     Serial.printf("current min: %d, max: %d\n",min_val,max_val);
//     Serial.println("Press limit down...");
//     delay(1000);
//     Serial.println("measuring");
//     calGetExpMinMax(btn_num,exp_min,exp_max);
//     min_val = min(max(min_val,exp_min),exp_max);
//     max_val = max(min(exp_max,max_val),exp_min);
//     calGetBtnStats(_mb_lu[4],num_points,min_val,avg_val,max_val,exp_min,exp_max);
//     //offset min/max
//     //min_val += 10
//     //store/print
//     _limsRanges[btn_num][0] = (int) min_val;
//     _limsRanges[btn_num][1] = (int) max_val;
//     Serial.printf("[%d] --> min: %d, max: %d\n", btn_num,_limsRanges[btn_num][0],_limsRanges[btn_num][1]);
//     prev_max_val = max_val;
//     Serial.println("---------------------");
//   }//for each btn

//   //done
//   cfgIOVranges(DX_CFG_WRITE);
//   Serial.println("========================");
// }   

// void DxButtons::calPrintCurent(){
//   Serial.println("_btnRanges:");
//   for (size_t i = 0; i < 4; i++)
//     {
//         Serial.printf("--%d: %d, %d\n", i,_btnRanges[i][0],_btnRanges[i][1]);
//     }
//   Serial.println("_limsRanges:");
//   for (size_t i = 0; i < 4; i++)
//     {
//         Serial.printf("--%d: %d, %d\n", i,_limsRanges[i][0],_limsRanges[i][1]);
//     }

// }
// void DxButtons::calGetExpMinMax(uint8_t bnt_num, uint16_t &exp_min,uint16_t &exp_max)
// {
//   uint16_t range_lims[5] ={0, 1024, 2048, 3072, 4096}; 
//   exp_min = 1;
//   exp_max = 4095;//4096-1;
//   if (bnt_num < 4){
//     exp_min = range_lims[bnt_num]+1;
//     exp_max = range_lims[bnt_num+1]-1;
//     Serial.printf("btn stats -- exp min:%d, max:%d\n",exp_min,exp_max);
//   }
// }

// void DxButtons::calPrintStream(uint8_t mb_num, uint32_t num_points, u_int32_t period)
// {
//   MultiButtons *mb;
//   if (mb_num == 0 || mb_num == 1) mb = _mb_lu[0]; //i.e. dis1
//   else if (mb_num >= 2 && mb_num <= 4) mb = _mb_lu[mb_num - 1];
//   else mb = &_lims;

//   if (num_points > 20000) num_points = 20000;
//   if (period < 25) period = 25;

//   Serial.println("=====================");
//   Serial.printf("start stream in 1 second mb:%d, pnts:%d, period:%d\n",mb_num,num_points,period);
//   delay(1000);

//   for (size_t i = 0; i < num_points; i++)
//   {
//     mb->getFilteredReading();
//     mb->printStats();
//     delay(period);
//   }
// }

void DxButtons::printStream()
{
  static uint32_t t_last = 0;
  uint16_t reading;
  uint32_t t_now = millis();
  
  if (t_now- t_last >= T_RAW_PERIOD)
  {
    Serial.print(COM_BTNS_MSG);
    for (size_t i = 0; i < NUM_PINS; i++)
    {
      reading = analogRead(pin_lu[i]);
      //PRINT ONLY READING
      Serial.printf("pin%d:%d",i,reading);

      if (i < NUM_PINS - 1) Serial.print(","); //dont print , on last value
    }
    Serial.println();
    t_last = t_now;
  }
}

void DxButtons::printStreamFiltered()
{
  static uint32_t t_last = 0;
  uint16_t reading;
  uint32_t t_now = millis();
  
  if (t_now- t_last >= T_RAW_PERIOD)
  {
    Serial.print(COM_BTNS_MSG);
    for (size_t i = 0; i < NUM_DX; i++)
    {
        MultiButtons *mb = _mb_lu[i];
        reading = mb->getFilteredReading();
        Serial.printf("pin%d:%d,",i,reading);
    }
    reading = _lims.getFilteredReading();
    Serial.printf("pin%d:%d\n",4,reading);
    t_last = t_now;
  }
}
