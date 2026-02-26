
#include "dxConfigMain.h"
#include "dxpinmap.h"
#include "dxstatemachine.h"

DxStateMachine sm;
uint32_t HB_PERIOD = 500;

void setup()
{
  //HB LED
  pinMode(DX_PIN_ONBOARD_LED,OUTPUT);
  digitalWrite(DX_PIN_ONBOARD_LED,HIGH);
  //State Machine
  sm.doInit();
}

void loop()
{
  static uint32_t hb_last_update = 0;
  
  sm.process();
  if (millis() - hb_last_update >= HB_PERIOD) {
    hb_last_update = millis();
    digitalWrite(DX_PIN_ONBOARD_LED, !(bool)digitalRead(DX_PIN_ONBOARD_LED));
  }

}
