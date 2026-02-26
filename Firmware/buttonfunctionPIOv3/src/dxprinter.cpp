#include "dxprinter.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "escprinterble/escprinterble.h"

#include "dxConfigMain.h"
#include "images.h"
#include "dxcfgid.h"

BLEScan *pBLEScanMain;
EscPos esc;

// bool is_connected = false;
void printerScanBT()
{
    int scanTime = 5;// time in seconds
    BLEDevice::init("");
    pBLEScanMain = BLEDevice::getScan(); //create new scan
    pBLEScanMain->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScanMain->setInterval(100);
    pBLEScanMain->setWindow(99);  // less or equal setInterval value

    // put your main code here, to run repeatedly:
    Serial.printf(COM_TP_MSG COM_KEY_MSG "start scan\n");
    BLEScanResults foundDevices = pBLEScanMain->start(scanTime, false);
    Serial.printf(COM_TP_MSG "Devices found:%d\n",foundDevices.getCount());
    for (int i = 0; i < foundDevices.getCount(); i++) 
    {
        Serial.printf(COM_TP_MSG "Devices num:%d",i);
        BLEAdvertisedDevice advertisedDevice = foundDevices.getDevice(i);
        Serial.printf(",address: %s\n",advertisedDevice.getAddress().toString().c_str());
        Serial.printf(",haveName:%d,name:%s\n",advertisedDevice.haveName(),advertisedDevice.getName().c_str());
        Serial.printf(",haveServiceUUID:%d,UUID:%s\n",advertisedDevice.haveServiceUUID(),advertisedDevice.getServiceUUID().toString().c_str());
        Serial.printf(",haveServiceData:%d,dataUUID:%s\n",advertisedDevice.haveServiceData(),advertisedDevice.getServiceDataUUID().toString().c_str());
        Serial.printf(",haveAppearance:%d,getAppearance:%d\n",advertisedDevice.haveAppearance(),advertisedDevice.getAppearance());
        Serial.printf(",haveManufacturerData:%d,getManufacturerData:%s\n",advertisedDevice.haveManufacturerData(),advertisedDevice.getManufacturerData().c_str());


    // If no name was advertised, try to connect and read the device name
    if (advertisedDevice.haveName() == false) {
        Serial.println(COM_TP_MSG COM_KEY_MSG "--> no name, create client");
        BLEClient* pClient  = BLEDevice::createClient();
        Serial.println(COM_TP_MSG COM_KEY_MSG "connect to client...");
        pClient->connect(&advertisedDevice);
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID((uint16_t)0x1800));
        if (pRemoteService != nullptr) {
        Serial.printf(COM_TP_MSG "pRemoteService:%s\n",pRemoteService->toString().c_str());
        BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID((uint16_t)0x2A00));
        if (pRemoteCharacteristic != nullptr) {
            std::string name = pRemoteCharacteristic->readValue();
            Serial.printf(COM_TP_MSG "Device Name:%s\n",name.c_str());
        }
        }
        else
        {
            Serial.println(COM_TP_MSG COM_KEY_MSG "pRemoteService == nullptr");
        }
        pClient->disconnect();
    }
    }
    Serial.println(COM_TP_MSG COM_KEY_MSG "Scan done!");
    pBLEScanMain->clearResults();   // delete results fromBLEScan buffer to release memory

}

bool printerLoop()
{
    esc.connectLoop();
    return esc.isConnected();
}

// bool printerTryConnect()
// {
//     is_connected = false;  
//     if (esc.connect())
//     { 
//         is_connected = true;
//         Serial.println(COM_TP_MSG COM_KEY_MSG "printer connected");

//     }
//     else{
//         Serial.println(COM_TP_ERROR COM_KEY_MSG"printer not connected")    ;
//     }
//     return is_connected;
// }

void printerStart()
{
    // You can change service, characteristic UUID for a compatible esc BLE printer
    //Scan Result: Name: , 
    //Address: 86:67:7a:88:28:64, 
    //serviceUUID: 000018f0-0000-1000-8000-00805f9b34fb, 
    //serviceUUID: e7810a71-73ae-499d-8c15-faa9aef0c3f2, rssi: -67
    char service[] = "000018F0-0000-1000-8000-00805F9B34FB";
    char characteristic[] = "00002AF1-0000-1000-8000-00805F9B34FB";
    esc = EscPos(service, characteristic);
    esc.start(dx_dev_id);
}

bool isPrinterConnected()
{
    return esc.isConnected();
}

void tpPrint(const char *txt)
{
    if (isPrinterConnected())
    {
        esc.println(txt);
    }
}

void tpSetFont(uint8_t new_font, uint8_t underline, uint8_t doubleWide, uint8_t doubleTall, uint8_t emphasized)
{
    esc.effectOff();
    if (underline == 1)     esc.effectUnderline();
    if (doubleWide == 1 )   esc.effectDoubleWidth();
    if (doubleTall == 1)    esc.effectDoubleHeight();
    if (emphasized == 1)    esc.effectBold();
}

void tpSingleCarriageReturn()
{
    tpPrint("\r");
}

void tpPrintCarriageReturnNtimes(uint8_t N)
{
    for (size_t i = 0; i < N; i++) tpSingleCarriageReturn();
}

void tpPrintLogo()
{
    esc.align(ALIGN_CENTER);
    esc.printImage(logo_buffer, logo_width, logo_height);
    esc.println("");
}

void tpWriteData(uint8_t *pData, int iLen)
{
    // if (!is_connected) // || !pRemoteCharacteristicData)
    //     return;
    // // Write BLE data without response, otherwise the printer
    // // stutters and takes much longer to print
    // // For some reason the ESP32 sends some corrupt data if we ask it
    // // to write more than 20 bytes at a time (used to be 48)
    // while (iLen > 20) {
    //    pRemoteCharacteristicData->writeValue(pData, 20, bWithResponse);
    //    if (!bWithResponse) delay(4);
    //    pData += 20;
    //    iLen -= 20;
    // }
    // if (iLen) {
    //   pRemoteCharacteristicData->writeValue(pData, iLen, bWithResponse);
    // }

}

void tpPrintWFont(const char *txt, uint8_t new_font, uint8_t underline, uint8_t doubleWide, uint8_t doubleTall, uint8_t emphasized)
{
    tpSetFont(new_font, underline, doubleWide, doubleTall, emphasized);
    tpPrint(txt);
}

void tpPrintBold(const char *txt){
    esc.effectOff();
    esc.effectBold();
    tpPrint(txt);
}

void tpPrintNormal(const char *txt){
    esc.effectOff();
    tpPrint(txt);
}

void tpPrintBoldAndNormal(const char *txt_title, const char *txt_item, uint8_t carriage_return){
    tpPrintBold(txt_title);
    tpPrintNormal(txt_item);
    if (carriage_return > 0) tpPrintCarriageReturnNtimes(carriage_return);
}
