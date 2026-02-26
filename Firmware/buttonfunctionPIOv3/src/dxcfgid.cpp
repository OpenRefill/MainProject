#include "dxcfgid.h"

char dx_dev_id[30] = "DF0"; //HEX 12 digit number

void dxMakeDevId()
{
    //get zero filled MAC address
    snprintf(dx_dev_id, sizeof(dx_dev_id), "%012llX", ESP.getEfuseMac());
}

