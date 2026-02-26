#ifndef PIN_MAP_H_FILE
#define PIN_MAP_H_FILE
#include <Arduino.h>

// TTGO T-Call pins
const uint8_t PIN_MODEM_RST         = 5;
const uint8_t PIN_MODEM_PWKEY       = 4;
const uint8_t PIN_MODEM_POWER_ON    = 23;
const uint8_t PIN_MODEM_TX          = 27;
const uint8_t PIN_MODEM_RX          = 26;

// I2C
const uint8_t PIN_I2C_SDA           = 21;
const uint8_t PIN_I2C_SCL           = 22;

// LOAD CELL
const uint8_t LOADCELL_DOUT_PIN1 = 34;
const uint8_t LOADCELL_DOUT_PIN2 = 35;
const uint8_t LOADCELL_DOUT_PIN3 = 32;
const uint8_t LOADCELL_DOUT_PIN4 = 33;
const uint8_t LOADCELL_SCK_PIN = 18;

#endif