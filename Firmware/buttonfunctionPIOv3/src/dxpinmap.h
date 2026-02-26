#ifndef DX_PIN_MAP_H_FILE
#define DX_PIN_MAP_H_FILE
#include <Arduino.h>

#include "dxConfigMain.h"
// onboard led
const uint8_t DX_PIN_ONBOARD_LED = 2;

// button control pins
const uint8_t DX_PIN_1     = 34;
const uint8_t DX_PIN_2     = 35;
const uint8_t DX_PIN_3     = 32;
const uint8_t DX_PIN_4     = 33;
const uint8_t DX_PIN_LIMS  = 25;

const uint8_t pin_lu[NUM_PINS] = {DX_PIN_1,
                                    DX_PIN_2,
                                    DX_PIN_3,
                                    DX_PIN_4,
                                    DX_PIN_LIMS
                                    };


// motor pins
const uint8_t DX_PIN_M1_P1 = 27;
const uint8_t DX_PIN_M1_P2 = 26;

// motor enable pins
const uint8_t DX_PIN_EN_M1 = 17;
const uint8_t DX_PIN_EN_M2 = 16;
const uint8_t DX_PIN_EN_M3 = 14;
const uint8_t DX_PIN_EN_M4 = 15;



#endif