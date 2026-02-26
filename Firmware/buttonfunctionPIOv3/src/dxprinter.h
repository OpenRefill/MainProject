#ifndef DX_PRINTER_H_FILE
#define DX_PRINTER_H_FILE
#include <Arduino.h>

void printerScanBT();
bool printerLoop();
void printerStart();
// void printerDoReceipt();

bool isPrinterConnected();

void tpPrint(const char *txt);
void tpSetFont(uint8_t new_font, uint8_t underline, uint8_t doubleWide, uint8_t doubleTall, uint8_t emphasized);
void tpWriteData(uint8_t *pData, int iLen);

void tpSingleCarriageReturn(); //'\r'
void tpPrintCarriageReturnNtimes(uint8_t N);
void tpPrintLogo();
void tpPrintBoldAndNormal(const char *txt_title, const char *txt_item, uint8_t carriage_return);
void tpPrintNormal(const char *txt);
void tpPrintBold(const char *txt);
void tpPrintWFont(const char *txt, uint8_t new_font, uint8_t underline, uint8_t doubleWide, uint8_t doubleTall, uint8_t emphasized);


#endif