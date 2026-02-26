#include "dxstatemachine.h"
#include "dxpinmap.h"
#include "serialparser.h"
#include "dxcfgid.h"
#include "dxbuzzer.h"
#include "dxproduct.h"
#include "dxmotors.h"
#include "dxI2Cio.h"
#include "dxbuttons.h"
#include "dxprinter.h"




DxProduct products;
DxMotors motors;
DxI2Cio dxi2c;
dxbuzzer buzzer;
SerialParser dxsp;
DxButtons btns;

void DxStateMachine::_loadCfgs()
{
    products.cfgIOflowRate(DX_CFG_READ);
    products.cfgIOTable(DX_CFG_READ);
    btns.cfgIOVranges(DX_CFG_READ);
    _cfgIO(DX_CFG_READ);
}

void DxStateMachine::_gotoIdleState()
{
  Serial.println(COM_DX_MSG COM_KEY_MSG "goto idle state");
  motors.stop();
  _dx_activated.setIndex(0);
  _dx_dispensed_price_tk = 0;
  _selected_amount_tk = 0;
  _selected_amount_ml = 0;
  _amount_index = 0;
  _selected_product = "";
  
  dxi2c.dispOnFinished();
  _sh.append(STATE_IDLE);
}

void DxStateMachine::_gotoActivatedState(uint8_t dx_num)
{
    if (!DxIndexHelper::isOk(dx_num)){
      Serial.printf(COM_DX_ERROR COM_KEY_MSG "could not activate dx num no ok, dx_num:%d\n",dx_num);
      return;
    }

    _dx_activated.setIndex(dx_num);
    _selected_product = *products.getName(dx_num);
    _selected_amount_tk = products.getSelectedVolumeTK(dx_num,_amount_index);
    _selected_amount_ml = products.getSelectedVolumeML(dx_num,_amount_index);
    Serial.printf(COM_DX_MSG COM_KEY_MSG "goto active state,dx:%d,name:%s,ml:%u,tk:%u\n",dx_num,_selected_product,_selected_amount_ml,_selected_amount_tk);

    // notify comms module that dispense has started
    dxi2c.comsOnStarted(_selected_product);
    // initialise display of the dispenser that was interacted with
    dxi2c.dispOnDxActive(dx_num); 
    // send the currently selected amount and its corresponding volume to the display
    dxi2c.dispOnselection(_selected_amount_tk,_selected_amount_ml);

    _sh.append(STATE_ACTIVE);
}

void DxStateMachine::_gotoWaitGreenState()
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "goto wait green,dx:%d,name:%s,ml:%u,tk:%u\n",dx_num,_selected_product,_selected_amount_ml,_selected_amount_tk);

  // inform comms module of chosen amount
  dxi2c.comsOnAmountSelected(_selected_product,_selected_amount_ml,_selected_amount_tk);
  // instruct display module to go to the "press green" screen
  dxi2c.dispPressGreen(); 
  _timer.start(_period_wait_event);
  _sh.append(STATE_WAIT_BTN_GREEN);
}

void DxStateMachine::_gotoWaitBeforeStartState()
{
    uint8_t dx_num = _dx_activated.getIndex();
    Serial.printf(COM_DX_MSG COM_KEY_MSG "goto wait for period before dispensing, dx: %d\n",dx_num);
    _timer.start(_period_before_start);
    dxi2c.dispPushBottle();
    _sh.append(STATE_WAIT_BEFORE_START);
}

void DxStateMachine::_gotoWaitTriggerState()
{
    uint8_t dx_num = _dx_activated.getIndex();
    Serial.printf(COM_DX_MSG COM_KEY_MSG "goto wait for trigger, dx: %d\n",dx_num);
    _timer.start(_period_pause);
    _sh.append(STATE_WAIT_TRIGGER);
}
/*
Start Dispense.
Process:
 -- the dispense controlled by estimating the volume dispensed over time. 
 -- with each loop the time passed is mutliplied with a fixed flow rate to calucate the volume dispensed. 
 -- if the limit switch is disengaged, the dispensed is paused
 -- dispensing continues if the limit switch is engaged 
 -- A pause of longer than 20 seconds results in a time-out, in which case:
         dispense Completed() is called to print a receipt for the amount that was already dispensed, 
         provided it is non-zero.
*/
void DxStateMachine::_gotoDispenseState()
{
    uint8_t dx_num = _dx_activated.getIndex();
    _dx_flow_rate = products.getFlowRate(dx_num);
    Serial.printf(COM_DX_MSG COM_KEY_MSG "goto dispensing state,dx: %d\n",dx_num);
    Serial.printf(COM_DX_MSG COM_KEY_MSG "selected amount,ml:%u,tk:%u,flow_rate:%.2f\n",_selected_amount_ml,_selected_amount_tk,_dx_flow_rate);
    _updateTimer.start(_period_dx_update);
    dxi2c.dispOnDispensing(_dx_dispensed_price_tk,_selected_amount_tk);
    //start motors
    motors.run(dx_num,_dx_speed);
    _dx_time_start = millis();
    _dx_time_at_pause = 0;
    _sh.append(STATE_DISPENSING);
}

void DxStateMachine::_gotoPrintState()
{
    Serial.printf(COM_DX_MSG COM_KEY_MSG "goto print state,dx_ml:%u\n",_dx_dispensed_ml);
    _sh.append(STATE_PRINTING);
    if (_dx_dispensed_ml > 0) _doDispenseCompleted();
    else _doFinished();
       
}

void DxStateMachine::_gotoPausedState()
{
    uint8_t dx_num = _dx_activated.getIndex();
    motors.stop();
    _dx_time_at_pause = _dx_time_delta;
    if (_dx_dispensed_ml > 0)
        {
          motors.reverse(dx_num);
          delay(100);
          motors.stop();
        }
    _timer.start(_period_pause);
    dxi2c.dispPushBottle();
    Serial.printf(COM_DX_MSG COM_KEY_MSG "paused, ml:%u,tk:%u,time:%.3f\n",_dx_dispensed_ml,_dx_dispensed_price_tk,_dx_time_at_pause);
    _sh.append(STATE_PAUSED);
}

void DxStateMachine::_gotoCancelState()
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "goto cancel state, dx: %d\n",dx_num);
  //display go to the "cancel?" page
  dxi2c.dispCancel();
  //inform comms module of attempted cancel
  dxi2c.comsOnCancelInit(_selected_product,_selected_amount_tk);
  _timer.start(_period_wait_event);
  _sh.append(STATE_CANCEL);
}

void DxStateMachine::_gotoCancelInDxState()
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "goto cancel state after start, dx: %d\n",dx_num);
  //display go to the "cancel?" page
  dxi2c.dispCancel();
  //inform comms module of attempted cancel
  dxi2c.comsOnCancelInit(_selected_product,_selected_amount_tk);
  _timer.start(_period_wait_event);
  _sh.append(STATE_CANCEL_IN_DX);
}

void DxStateMachine::_onBtnInActiveState(BtnNumEnum btn_name)
{
    uint8_t dx_num = _dx_activated.getIndex();
    /*
    RX-DX:choosing amount,{'dx': 4.0, 'btn': 0.0}
    RX-DX:new selection,{'ml': 0.0, ' tk': 16128.0, ' ix': 2.0}
    */
    Serial.printf(COM_DX_MSG COM_KEY_MSG "choosing amount,dx:%u,btn:%u\n",dx_num,btn_name);
    
    switch (btn_name)
    {
        case BTN_GREEN:
            _gotoWaitGreenState();
            break;

        case BTN_RED:
            _gotoCancelState();
            break;

        case BTN_DOWN:
            // decrease amount  
            if (_amount_index > 0) _amount_index--;
            products.getAmounts(dx_num,_amount_index,_selected_amount_tk,_selected_amount_ml);

            // update the display
            dxi2c.dispOnselection(_selected_amount_tk,_selected_amount_ml);
            Serial.printf(COM_DX_MSG COM_KEY_MSG "new selection,ml:%d, tk:%d, ix:%d\n",_selected_amount_ml,_selected_amount_tk,_amount_index);
            break;

        // Increase Amount
        case BTN_UP:
            // increment the index of the selected popular amount
            if (_amount_index < NUM_AMOUNTS-1) _amount_index++;
            // update the display
            products.getAmounts(dx_num,_amount_index,_selected_amount_tk,_selected_amount_ml);
            dxi2c.dispOnselection(_selected_amount_tk,_selected_amount_ml);
            Serial.printf(COM_DX_MSG COM_KEY_MSG "new selection,ml:%d, tk: %d, ix:%d\n",_selected_amount_ml,_selected_amount_tk,_amount_index);
            break;

    }//SWITCH BTN PRESSED
}

void DxStateMachine::_onBtnInWaitGreenState(BtnNumEnum btn_name)
{
    uint8_t dx_num = _dx_activated.getIndex();
    Serial.printf(COM_DX_MSG COM_KEY_MSG "btn in wait-green,dx:%u,btn:%u\n",dx_num,btn_name);
    switch (btn_name)
    { 
        case BTN_GREEN:
            _gotoWaitBeforeStartState();
            break;

        case BTN_RED:
            _gotoCancelState();
            break;

        default:
            //ignore other btns
            break;
    }//SWITCH BTN
}

void DxStateMachine::_onBtnInCancelState(BtnNumEnum btn_name)
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "btn in cancel screen,dx:%u,btn:%u\n",dx_num,btn_name);
  switch (btn_name)
  {
  case BTN_RED:
      _doFinished();
      break;

  case BTN_GREEN:
      _gotoActivatedState(dx_num);
      break;    

  default:  
      break;    
  }
}

void DxStateMachine::_onBtnInCancelInDxState(BtnNumEnum btn_name)
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "btn in cancel screen after dx start,dx:%u,btn:%u\n",dx_num,btn_name);
  switch (btn_name)
  {
  case BTN_RED:
      _gotoPrintState(); // print if any dispensed else just finish to idle
      break;

  case BTN_GREEN:
      _timer.start(_period_pause);
      dxi2c.dispPushBottle();
      Serial.printf(COM_DX_MSG COM_KEY_MSG "go back to paused state, ml:%u,tk:%u,time:%.3f\n",_dx_dispensed_ml,_dx_dispensed_price_tk,_dx_time_at_pause);
      _sh.append(STATE_PAUSED);
      break;    

  default:  
      break;    
  }
}

void DxStateMachine::_onBtnInPause(BtnNumEnum btn_name)
{
  uint8_t dx_num = _dx_activated.getIndex();
  Serial.printf(COM_DX_MSG COM_KEY_MSG "btn in pause screen,dx:%u,btn:%u\n",dx_num,btn_name);
  switch (btn_name)
  {
  case BTN_RED:
      _gotoCancelInDxState();
      break;

  default:  
      break;    
  }
}

void DxStateMachine::_onContinueDispense(uint32_t time_now)
{
  uint8_t dx_num = _dx_activated.getIndex();
  _dx_flow_rate = products.getFlowRate(dx_num);
  Serial.printf(COM_DX_MSG COM_KEY_MSG "gobackto dispensing state,dx: %d\n",dx_num);
  _updateTimer.start();
  _dx_time_start = time_now;
  //start motors
  motors.run(dx_num,_dx_speed);
  _sh.append(STATE_DISPENSING);
}

void DxStateMachine::_onSetTableCfg(float *args)
{
  uint8_t dx_num = (uint8_t)args[0];
  //amounts A,B,C:
  //args: 0:dx_num, A(1:ml,2:tk), B(3:ml,4:tk) , C(5:ml,6:tk)
  uint8_t arg_indices[NUM_AMOUNTS][2] = {{1,2},{3,4},{5,6}};
  for (size_t amount_i = 0; amount_i < NUM_AMOUNTS; amount_i++)
  {
    uint16_t amount_ml = (uint16_t)args[arg_indices[amount_i][0]];
    uint16_t amount_tk = (uint16_t)args[arg_indices[amount_i][1]];
    products.setTableXY(dx_num,       //dx_num
                      amount_i,       //amount index
                      amount_ml,      //amount i:vol in ml
                      amount_tk);     //amount i:price in tk;
  }
  products.cfgIOsaveTable();
}

int8_t DxStateMachine::_checkCmdArgs(ComCmds cmd, float *args)
{
  int8_t error_code = 0;
  float value_1 = args[0];
  float value_2 = args[1];
  float value_3 = args[2];
  float value_4 = args[3];
  // float value_4 = args[4];
  // char (*cfg_strings)[SERIAL_NUM_RX_CHARS] = sp.getStringValues();
  //check vals
  switch (cmd){
    case CMD_CFG_FLOW_RATE:
        if (value_1 == 0 || value_2 == 0 || value_3 == 0 || value_4 == 0) error_code = -2;
        break;

    case CMD_CFG_RANGE_BTN_X: 
    case CMD_CFG_RANGE_LIM_X: 
        if (!DxIndexHelper::isOk((uint8_t) value_1)) error_code = -2;   //dx_num
        if (value_2 == 0 || value_3 == 0) error_code = -2;            //low, high
        break;

    case CMD_CFG_PRODUCT_TABLE:
      {
        if (!DxIndexHelper::isOk((uint8_t) value_1)) error_code = -2;
        //offset with 1, as value_1 is dx_num
        //1,3,5 --> (1,2),(3,4),(5,6)
        uint8_t arg_indices[NUM_AMOUNTS][2] = {{1,2},{3,4},{5,6}};
        for (size_t amount_i = 0; amount_i < NUM_AMOUNTS; amount_i++)
        {
          uint8_t ix_0 = arg_indices[amount_i][0];
          uint8_t ix_1 = arg_indices[amount_i][1];
          if (args[ix_0] <= 0 || args[ix_1] <= 0 ) error_code = -2;
          Serial.printf(COM_DX_MSG "amount_i:%d,v0:%.0f,v1:%.0f,e:%d\n",amount_i,args[ix_0],args[ix_1],error_code);
        }
      }
      break;
   

    // case CMD_CAL_DX:
    // case CMD_CAL_PRINT_DX:
    //   if (value_1 == 0 || value_2 == 0)  error_code = -2;
    //   else if (value_1 > 5 || value_2 > 50) error_code = -2;
    //   break;

    // case CMD_CAL_LIM:
    // case CMD_CAL_PRINT_LIMS:
    //   if (value_1 == 0)  error_code = -2;
    //   break;

    // case CMD_CAL_PRINT_STREAM:
    //   if (value_1 > 5) error_code = -2;
    //   break;

    case CMD_PRINT_SIM_DX:
      if (value_1 < 1 || value_1 > 4) error_code = -2; //DX-NUM
      if (value_2 < 0 || value_2 >= NUM_AMOUNTS) error_code = -2; //AMOUNT INDEX
      break;

    case CMD_MOTORS_RUNTIME:
      if (value_1 < 1 || value_1 > 4)   error_code = -2; //DX-NUM
      if (value_2 < 1 || value_2 > 255) error_code = -2; //SPEED
      break;

    default:
      break;
  }
  return error_code;
}

void DxStateMachine::_doIdleState()
{
    int8_t cmd_error = 0;
    //CONNECT TO COMS
    if (!dxi2c.isCommsRdy()) dxi2c.comsGetReady();

    //CHECK FOR FIRST ACTIVATION
    for (size_t i = 1; i <= NUM_DX; i++)
    {
        if (btns.btnLoop(i)) 
        {
            Serial.printf(COM_DX_MSG COM_KEY_MSG "btn event, i:%d\n",i);
            btns.getEventBtnNum(i); //prints event info, also resets event
            _gotoActivatedState(i);
            _timer.start(_period_wait_event);
            break;
        }//if BTN EVENT 
        
    }//FOR dx-i
    
}

void DxStateMachine::_doDispense(uint32_t time_now)
{
    // uint16_t p_count_max = 80;
    // uint32_t update_period = 100;
    // static uint16_t p_cnt = p_count_max;
    uint8_t dx_num = _dx_activated.getIndex();
    if (_dx_time_start >= time_now) return;
    //DETERMINE DISPENSE TOTAL TIME (s)
    _dx_time_delta = (time_now - _dx_time_start)/1000.0 + _dx_time_at_pause; //cvt to seconds
    if (_dx_time_delta < 0.2) return;
    //DETERMINE VOLUME and PRICE, given TIME (s) and FLOWRATE
    _dx_dispensed_ml = (uint16_t) ceil(_dx_time_delta*_dx_flow_rate);
    _dx_dispensed_price_tk = products.getPriceOfVolumeDx(dx_num,_dx_dispensed_ml);
    //PRINT PROGRESS
    if (_updateTimer.isPeriodDone())
    {
      if (_selected_amount_ml == 0) _selected_amount_ml = 1;
      float progress = constrain(100*_dx_dispensed_ml/_selected_amount_ml,0,100);
      Serial.printf(COM_DX_MSG COM_KEY_MSG "dispensing,ml:%u,tk:%u,time:%.3f,pro:%.1f\n",_dx_dispensed_ml,_dx_dispensed_price_tk,_dx_time_delta,progress);
      dxi2c.dispOnDispensing(_dx_dispensed_price_tk,_selected_amount_tk);
      _updateTimer.start();
    }
    //CHECK IF DONE
    if (_dx_dispensed_ml >= _selected_amount_ml || _dx_dispensed_price_tk >= _selected_amount_tk)
    {
        motors.stop();
        Serial.printf(COM_DX_MSG COM_KEY_MSG "done,dx_ml:%u,dx_tk:%u\n",_dx_dispensed_ml,_dx_dispensed_price_tk);
        _dx_dispensed_ml = _selected_amount_ml;
        _dx_dispensed_price_tk = _selected_amount_tk;

        // 100ms in reverse to pump excess product back into tubing
        Serial.println(COM_DX_MSG COM_KEY_MSG "reverse motors");
        motors.reverse(dx_num);
        float progress = constrain(100*_dx_dispensed_ml/_selected_amount_ml,0,100);
        Serial.printf(COM_DX_MSG COM_KEY_MSG "dispensing,ml:%u,tk:%u,time:%.3f,pro:%.1f\n",_dx_dispensed_ml,_dx_dispensed_price_tk,_dx_time_delta,progress);
        delay(100);
        motors.stop();
        dxi2c.dispPrinting();
        _gotoPrintState();
    }
}


/* 
 *  Print a receipt for the dispense via the BlueTooth Printer.
 *  Also send the dispense telemetry to the Comms ESP32 module 
 *  via I2C and sounds the buzzer upon dispense completion.
 *  
 *  @param dispenserIndex     The index of the currently active dispenser.
 *  @param choice             The name of the chosen product.
 *  @param amountDispensedTK  The amount that was actually dispensed, in Taka.
 *  @param enteredAmountTK    The amount that was entered by the user to dispense, in Taka.
 *  
 *  @notes  `amountDispensedTK` may not be equal to `enteredAmountTK`, since the user may 
 *          stop dispensing mid-way through. If the dispense times out, a receipt is 
 *          printed for the amount that was already dispensed at time-out.
 *  
 */
void DxStateMachine::_doDispenseCompleted()
{
  uint8_t dx_num = _dx_activated.getIndex();
  uint16_t savings = products.getSavings(dx_num, _dx_dispensed_price_tk);
 
  Serial.printf(COM_DX_MSG COM_KEY_MSG "report results,dx_ml:%u,dx_tk:%u,savings:%u\n",_dx_dispensed_ml,_dx_dispensed_price_tk,savings);
  dxi2c.dispPrinting();

  //PLAY SONG
  if (_has_buzzer) buzzer.playSong();


  //SEND TO DISPLAY
  if (savings > 0)
  {
    delay(1000); 
    dxi2c.dispOnSavings(savings);
  }

  //SEND TO COMS
  dxi2c.comsOnFinished(_selected_product,_selected_amount_ml,_dx_dispensed_ml,_dx_dispensed_price_tk);
  //sendtoSlave("Finished:" + choice + ":" + String(volEnteredML) + ":" + String(volDispensedML) + ":" + String(price) + ":");
  
  //PRINT
  _doPrintReceipt(savings);
  
  //PAUSE DISPLAY at SAVINGS
  delay(5000);

  //go back to idle
  _doFinished();
    
}

void DxStateMachine::_doPrintReceipt(uint16_t savings)
{
  if (_has_printer)
  {
    if (!isPrinterConnected()) return; 

    char char_buffer[20];
    uint8_t dx_num = _dx_activated.getIndex();
    String date = dxi2c.comsGetDate();
    String batch_number = "12B";
    String expiry_date = "12-01-2025";
    
    //start print
    Serial.println(COM_TP_MSG COM_KEY_MSG "printing start");
    tpPrintCarriageReturnNtimes(3);

    //URefill
    tpPrintBold("URefill");
    tpPrintCarriageReturnNtimes(1);

    //PRODUCT NAME
    tpPrintBoldAndNormal("Product Name: ",products.getName(dx_num)->c_str(),1);

    //PRODUCT CATERGORY
    tpPrintBoldAndNormal("Catergory: ",products.getCat(dx_num)->c_str(),1);

    //VOLUME DISPENSED
    sprintf(char_buffer, "%u ml\r", _dx_dispensed_ml);
    tpPrintBoldAndNormal("Net Volume: ",char_buffer,1);

    //PURCHASE DATE
    tpPrintBoldAndNormal("Date of purchase: ",date.c_str(),1);

    // expiry date
    tpPrintBoldAndNormal("Expiry Date: ",expiry_date.c_str(),1);

    // batch number
    tpPrintBoldAndNormal("Batch Number: ",batch_number.c_str(),1);
    
    // ingredients
    tpPrintCarriageReturnNtimes(1);
    tpPrintBoldAndNormal("Ingredients: ",products.getIngredients(dx_num)->c_str(),1);

    //PRICE
    sprintf(char_buffer, "%u Taka\r", _dx_dispensed_price_tk);
    tpSetFont(1, 0, 1, 1, 1);
    tpPrint("Price: ");
    tpPrint(char_buffer);
    tpPrintCarriageReturnNtimes(1);
    
    //YOU SAVED
    // if (savings > 0){
    //     tpPrintWFont("You Saved: ",1, 0, 1, 0, 0);
    //     sprintf(char_buffer, "%d Taka\r", savings);
    //     tpPrint(char_buffer);
    // }

    //LAST MSG
    tpPrintCarriageReturnNtimes(3);
    tpPrintWFont("Please wash and dry your bottle regularly.Use clean water.",1, 0, 0, 0, 0);
    tpSetFont(0, 0, 0, 0, 0);
    

    #ifdef OPT_PRINT_LOGO
        tpPrintCarriageReturnNtimes(1);
        tpPrintLogo();
    #endif
    tpPrintCarriageReturnNtimes(3);

    delay(1000);
    Serial.println(COM_TP_MSG COM_KEY_MSG "done printing");
  } //has printer
}

void DxStateMachine::_doFinished()
{
  Serial.println(COM_DX_MSG COM_KEY_MSG "do finished dispense");
  // dxi2c.dispOnFinished();
  // delay(5000);
  _gotoIdleState();
}

void DxStateMachine::_doCheckSerial()
{
  //CHECK SERIAL
  if (dxsp.checkSerial())
  { 
      ComCmds cmd_num = dxsp.getCurrentCmd();
      float *data_ptr = dxsp.getRxDataPointer();
      if (_sh.isState(STATE_LOCKED))
      {
        if (cmd_num == CMD_ESP_UNLOCK)
        {
          char (*cfg_strings)[SERIAL_NUM_RX_CHARS] = dxsp.getStringValues();
          if (cfgUnlock(cfg_strings[0]))
          {
            Serial.println(COM_DX_MSG COM_KEY_MSG "unlocked - do reset");
            esp_restart();
          }
          else 
          {
            Serial.println(COM_DX_MSG COM_KEY_MSG "unlocked failed");
          }
          
        }
        dxsp.clear(0);
      }
      else
      {
        int8_t cmd_error = _checkCmdArgs(cmd_num,data_ptr);
        if (cmd_error == 0) cmd_error = _doParseCmd(cmd_num,data_ptr);
        dxsp.clear(cmd_error);
      }
  }
}

int8_t DxStateMachine::_doParseCmd(ComCmds cmd, float *args)
{
  int8_t error_code = 0;
  float value_1 = args[0];
  float value_2 = args[1];
  float value_3 = args[2];
  float value_4 = args[3];
  uint8_t dx_num = 0;

  switch (cmd){
    case CMD_DX_ID:
      Serial.printf(COM_DX_MSG "version:%s\n",VERSION_BUTTONFUNCTIONPIO);
      break;

    case CMD_DX_SET_FEATURES:
      _has_printer = value_1 > 0;
      _has_buzzer = value_2 > 0;
      Serial.printf(COM_DX_MSG "has_printer:%d,has_buzzer:%d\n",_has_printer,_has_buzzer);
      _cfgIO(DX_CFG_WRITE);
      break;

    //==================================
    //PRODUCT CFG
    case CMD_CFG_GET_ALL:
      btns.getCfg();
      products.getCfg();
      break;
    
    case CMD_CFG_FLOW_RATE: 
        products.setFlowRate(value_1,value_2,value_3,value_4);   
      break;
    //==================================
    //set buttons voltages for btn (dx_num, low, high)
    case CMD_CFG_RANGE_BTN_X: 
      dx_num = (uint8_t) value_1;
      btns.btnSetRangeX(dx_num,(uint16_t) value_2,(uint16_t) value_3);
      break;
    //==================================
    //set limit ranges
    case CMD_CFG_RANGE_LIM_X: 
      dx_num = (uint8_t) value_1;
      btns.limsSetRangeX(dx_num,(uint16_t) value_2,(uint16_t) value_3);
      break;
    //==================================
    //SET PRODUCT TABLE
    case CMD_CFG_PRODUCT_TABLE:
      _onSetTableCfg(args);
      break;    
    //==================================
    case CMD_CAL_PRINT_RAW:
      if (value_1 == 1) _sh.append(STATE_RAW_STREAM);
      else              _sh.append(STATE_IDLE);
      break;
    //==================================
    //PRINTER 
    case CMD_PRINT_SIM_DX:
      _dx_activated.setIndex((uint8_t) value_1);
      _amount_index = (uint8_t) value_2;
      _dx_dispensed_ml = products.getSelectedVolumeML(_dx_activated.getIndex(),_amount_index);
      _dx_dispensed_price_tk = products.getSelectedVolumeTK(_dx_activated.getIndex(),_amount_index);
      _doDispenseCompleted();
      _gotoIdleState();
      break;

    case CMD_PRINT_SCAN_BT:
      if (_has_printer)  printerScanBT();
      break;

    case CMD_PRINT_CONNECT:
      // if (_has_printer)  printerTryConnect();
      break;

    //==================================
    //MOTORS
    //run motor for test over time period (ms) at speed (1-255)
    case CMD_MOTORS_RUNTIME:
      Serial.println(COM_DX_MSG COM_KEY_MSG "run motors");
      motors.startRunForTimePeriod((uint8_t) value_1, (uint16_t) value_2, value_3 > 0, (uint32_t) value_4);
      _sh.append(STATE_MOTOR_TEST);
      break;
    //stop motors, also go back to idle state
    case CMD_MOTORS_STOP:
      motors.stop();
      if (_sh.isState(STATE_MOTOR_TEST)) _gotoIdleState();
      break;
    //==================================
    //ESP CMDS
    case CMD_ESP_RESET:
      Serial.println(COM_DX_MSG COM_KEY_MSG "esp reset");
      esp_restart();
      break;
    case CMD_ESP_LOCK:
      Serial.println(COM_DX_MSG COM_KEY_MSG "relock device - do reset");
      cfgLock();
      esp_restart();
      break;
    //==================================
    default:
      error_code = -1;
      break;
  };

  return error_code;
}

void DxStateMachine::doInit()
{
  //get id from MAC
  dxMakeDevId();
  //SERIAL INIT
  Serial.begin(BAUD_RATE);
  dxsp.doStartup();
  //Check unlock
  if (cfgIsLocked()){
    Serial.println(COM_DX_MSG COM_KEY_MSG "device locked");
    _sh.setState(STATE_LOCKED);
  }
  else
  {
    Serial.println(COM_DX_MSG COM_KEY_MSG "device is unlocked");
    Serial.println(COM_DX_MSG COM_KEY_MSG "Button Board Init");
    //I2C INIT
    dxi2c.doInit();
    //SETUP MOTORS
    motors.doInit();
    //LOAD CONFIGS
    _loadCfgs();
    //PRINTER INIT
    if (_has_printer) printerStart();
  
    //BUTTONS
    btns.doInit();

    //PLAY START SONG
    if (_has_buzzer) buzzer.playSong();
    
    _gotoIdleState();
    Serial.println(COM_DX_MSG COM_KEY_MSG "init done");
  }
}

void DxStateMachine::process()
{
    uint8_t dx_num = _dx_activated.getIndex();
    uint32_t time_stamp = millis();
    _doCheckSerial();
    switch (_sh.state())
    {
      case STATE_LOCKED:
        break;

      case STATE_IDLE: //i.e. _dx_activated == 0
        _doIdleState();
        break;//CASE IDLE
      
      case STATE_ACTIVE:
        if (btns.btnLoop(dx_num)) _onBtnInActiveState(btns.getEventBtnNum(dx_num)); //aslo resets event flag

        else if (_timer.isPeriodDone(time_stamp))
        {
          Serial.println(COM_DX_MSG COM_KEY_MSG  "time elapsed--goto idle");
          _gotoIdleState();
        }
        break;

      case STATE_WAIT_BTN_GREEN:
        if (btns.btnLoop(dx_num)) _onBtnInWaitGreenState(btns.getEventBtnNum(dx_num)); //also resets event flag

        else if (_timer.isPeriodDone(time_stamp))
        {
          Serial.println(COM_DX_MSG COM_KEY_MSG "time elapsed--goto idle");
          _gotoIdleState();
        }
        break;

      case STATE_WAIT_BEFORE_START:
        //this is just a wait time to allow user to press trigger, before displaying "paused" screen on trigger press
        if (_timer.isPeriodDone(time_stamp)) _gotoWaitTriggerState();
        break;

      case STATE_WAIT_TRIGGER:
        //no trigger event, go to cancel
        if (_timer.isPeriodDone(time_stamp))
        {
            Serial.println(COM_DX_MSG COM_KEY_MSG "time elapsed--goto cancel");
            _gotoCancelState();
        }
        //trigger event, start dispense
        else if (btns.limsIsPressing(dx_num)) _gotoDispenseState();

        break;
      
      case STATE_DISPENSING:
        //pressing: run dispense
        if (btns.limsIsPressing(dx_num)) _doDispense(time_stamp);
        //released: paused
        else _gotoPausedState();
        break;

      
      case STATE_PAUSED:
        //pressing again, continue dispense
        if (btns.limsIsPressing(dx_num)) _onContinueDispense(time_stamp);
        //time passed, finish dispense
        else if (_timer.isPeriodDone(time_stamp))
        {
          Serial.println(COM_DX_MSG COM_KEY_MSG "paused timed out");
          dxi2c.comsOnTimeOut(_selected_product,_selected_amount_tk,_dx_dispensed_ml);
          _gotoPrintState();
        }
        //check if red button to goto cancel
        else if (btns.btnLoop(dx_num)) _onBtnInPause(btns.getEventBtnNum(dx_num)); //also resets event flag

        break;

      case STATE_CANCEL:
        if (btns.btnLoop(dx_num)) _onBtnInCancelState(btns.getEventBtnNum(dx_num)); //also resets event flag
        //time passed, assume cancel
        else if (_timer.isPeriodDone(time_stamp))
        {
          Serial.println(COM_DX_MSG COM_KEY_MSG "time elapsed -- goto idle");
          _gotoIdleState();    
        }
        break;

      
      case STATE_CANCEL_IN_DX:
        if (btns.btnLoop(dx_num)) _onBtnInCancelInDxState(btns.getEventBtnNum(dx_num)); //also resets event flag
        //time passed, assume cancel
        else if (_timer.isPeriodDone(time_stamp))
        {
          Serial.println(COM_DX_MSG COM_KEY_MSG "time elapsed -- goto print");
          _gotoPrintState(); // print if any dispensed else just finish to idle  
        }
        break;

      case STATE_MOTOR_TEST:
        motors.doRunForPeriod(time_stamp);
        break;

      case STATE_RAW_STREAM:
        // btns.printStream();
        btns.printStreamFiltered();
        break;

      default:
        break;
  }//switch state

  //APPLY state change (set to state to X)
  _sh.apply();
}
