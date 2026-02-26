#ifndef DX_CONFIG_MAIN_H_FILE
#define DX_CONFIG_MAIN_H_FILE
#include <Arduino.h>

#define I2C_DEV_ADDR 0x56

const uint8_t NUM_DISPLAYS = 4;
/*
I2C comms: display is slave to main ESP
*/
void onRequest();
void onReceive(int len);

/*
Display Control
*/
void resetDisplays();
void resetDisplayX(uint8_t display_number);
void setActiveDisplay(uint8_t dx_number);
void showYouSaved(String msg);
void showChangedSelection(String msg);
void showProgress(String msg);
void showPressGreen();
void showPushBottle();
void showCancel();
void showPrinting();
void showFinished();
uint8_t getDigitsFromString(String msg, uint8_t start_pos, uint8_t num_digits, uint8_t &val1, uint8_t &val2, uint8_t &val3);

#endif