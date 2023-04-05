#include "i2cscan.h"
//---------------------------------------------------------------
// I2cScan(&TwoWire, SemaphoreHandle_t, TickType_t)
// TwoWire - I2C buss to scan
// SemaphoreHandle_t - I2C Mutex
// TickType_t - Probably portMAX_DELAY to block for scan
//---------------------------------------------------------------
int I2cScan(TwoWire *w, int devlist[127],SemaphoreHandle_t i2csem, TickType_t dly) {   
    int address;
    int error;
    int devcount;
    devcount=0;
    for(address=0;address<127;address++) devlist[address]=0;
    address=0;
    if(xSemaphoreTake(i2csem,dly)) {
        for (address = 1; address < 127; address++) {
            w->beginTransmission(address);
            error = w->endTransmission(); 
            if (error == 0) {
                devlist[devcount++] = address;
            } 
        }
        xSemaphoreGive(i2csem);
    }
    return devcount;
}