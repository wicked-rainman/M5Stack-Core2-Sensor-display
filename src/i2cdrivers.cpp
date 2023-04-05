#include "i2cdrivers.h"

void LoadI2cDrivers(int DeviceCount, int DeviceList[127],WiFiUDP *Dg, int Pno, SemaphoreHandle_t xWifi, int WaitTime, bool core) {
    char diag[100];
    snprintf(diag,100,"%s:I2C: %d\n",mHostname, DeviceCount);
    UdpWrite(Dg,WiFi.broadcastIP(),Pno,xWifi,WaitTime,diag);
    for(int k=0;k<DeviceCount;k++) {
        switch(DeviceList[k]) {
            case 0x44 : {
                snprintf(diag,100,"%s:I2c:0x%x: SHT30 %d\n",mHostname,DeviceList[k],(int)core);
                UdpWrite(Dg,WiFi.broadcastIP(),Pno, xWifi,WaitTime,diag);
                vTaskDelay(100/portTICK_RATE_MS);
                TOGGLE_CORE;
                xTaskCreatePinnedToCore(SHT30drv,"SHT30",3400,NULL,1, &tasks[task_count++],(int)core);
                break;
            }
            case 0x76 : {
                snprintf(diag,100,"%s:I2c:0x%x: BMP280 %d\n",mHostname,DeviceList[k],(int) core);
                UdpWrite(Dg,WiFi.broadcastIP(),Pno, xWifi,WaitTime,diag);
                vTaskDelay(100/portTICK_RATE_MS);
                TOGGLE_CORE;
                xTaskCreatePinnedToCore(BMP280drv,"BMP280",3300,NULL,1, &tasks[task_count++],(int) core);
                break;
            }
            default : {
                snprintf(diag,100,"%s:i2c:0x%x: unk\n",mHostname,DeviceList[k]);
                UdpWrite(Dg,WiFi.broadcastIP(),Pno, xWifi,WaitTime,diag);
                break;
            }
        }
    }
    snprintf(diag,100,"%s:I2c: Done\n",mHostname);
    UdpWrite(Dg,WiFi.broadcastIP(),Pno, xWifi,WaitTime,diag);

}
