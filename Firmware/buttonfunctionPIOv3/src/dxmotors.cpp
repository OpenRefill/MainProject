#include "dxmotors.h"


void DxMotors::_stopIfDirChange(bool fwd)
{
  if (isRunning() && !isDirection(fwd)) stop();
}

void DxMotors::_stopIfDiffDx(uint8_t dx_index)
{
  if (isRunning() && dx_index != _dx_num) stop();
}

void DxMotors::doInit()
{
  /* --- PWM Properties --- */
  const int freq = 30000;
  const int pwmChannel1 = 0;
  const int pwmChannel2 = 1;
  const int pwmChannel3 = 2;
  const int pwmChannel4 = 3;
  const int resolution = 8;

  pinMode(DX_PIN_M1_P1, OUTPUT);
  pinMode(DX_PIN_M1_P2, OUTPUT);
  
  pinMode(DX_PIN_EN_M1, OUTPUT);
  pinMode(DX_PIN_EN_M2, OUTPUT);
  pinMode(DX_PIN_EN_M3, OUTPUT);
  pinMode(DX_PIN_EN_M4, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel1, freq, resolution);
  ledcSetup(pwmChannel2, freq, resolution);
  ledcSetup(pwmChannel3, freq, resolution);
  ledcSetup(pwmChannel4, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(DX_PIN_EN_M1, pwmChannel1);
  ledcAttachPin(DX_PIN_EN_M2, pwmChannel2);
  ledcAttachPin(DX_PIN_EN_M3, pwmChannel3);
  ledcAttachPin(DX_PIN_EN_M4, pwmChannel4);

  stop();
}

void DxMotors::stop()
{
  analogWrite(DX_PIN_EN_M1, 0);
  analogWrite(DX_PIN_EN_M2, 0);
  analogWrite(DX_PIN_EN_M3, 0);
  analogWrite(DX_PIN_EN_M4, 0);

  digitalWrite(DX_PIN_M1_P1, LOW);
  digitalWrite(DX_PIN_M1_P2, LOW);

  if (_is_running) Serial.println(COM_MOTORS_MSG COM_KEY_MSG "stopped");
  _is_running = false;
}

void DxMotors::run(uint8_t dx_index,uint16_t speed,bool fwd)
{
  dx_index = constrain(dx_index,1,4);
  _stopIfDirChange(fwd);
  _stopIfDiffDx(dx_index);

  speed = constrain(speed,0,255);
  uint8_t pin = _en_pins[dx_index  -1];
  Serial.printf(COM_MOTORS_MSG COM_KEY_MSG "run,dx:%u,speed:%u,dir:%d\n",dx_index,speed,fwd);
  analogWrite(pin, speed);

  if (fwd){
      digitalWrite(DX_PIN_M1_P1, HIGH);
      digitalWrite(DX_PIN_M1_P2, LOW);
  }
  else{
      digitalWrite(DX_PIN_M1_P1, LOW);
      digitalWrite(DX_PIN_M1_P2, HIGH);
  }
  _fwd = fwd;
  _is_running = true;
  _dx_num = dx_index;
}

void DxMotors::startRunForTimePeriod(uint8_t dx_index, uint16_t speed, bool fwd, uint32_t period)
{
  
  _time_period = period;
  _time_start = millis();
  Serial.printf(COM_MOTORS_MSG COM_KEY_MSG "run for period, period: %u\n",period);
  run(dx_index,speed,fwd); //will stop motor if different direction or dx_num
}

void DxMotors::doRunForPeriod(uint32_t time_stamp)
{
  uint16_t print_max = 500;
  static uint16_t print_cnt = print_max;
  if (_time_start >= time_stamp) return; //should not get here...
  if (_time_period == 0) return; //0 time period -- run infinitely
  if (!_is_running) return; //stopped already

  float progress = 100;
  uint32_t time_done = time_stamp - _time_start;
  //check if time to stop
  if (time_done >= _time_period)
  {
    stop();
    print_cnt = print_max;
  }
  //else update progress % 
  else progress = 100.0*time_done/_time_period;
  //report progress
  if (print_cnt++ >= print_max)
  {
    Serial.printf(COM_MOTORS_MSG "pro:%.2f\n",progress);
    print_cnt = 0;
  }
}
