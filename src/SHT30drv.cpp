#include "SHT30drv.h"
void SHT30drv(void *summat) {    
SHT3x Sensor;
char index[20];
char Val[30];
float humidity;
float storedHumidity;
    if(xSemaphoreTake(xI2c,portMAX_DELAY)) {
        storedHumidity=0;
        Sensor.Begin();
        xSemaphoreGive(xI2c);
    }
    while(true) {
        if(xSemaphoreTake(xI2c,portMAX_DELAY)) {
            Sensor.UpdateData();
            humidity=Sensor.GetRelHumidity();
            xSemaphoreGive(xI2c);
            snprintf(index,20,"%s:SHT30:",mHostname);
            snprintf(Val,30,"%.2f%%",humidity);
            storeWrite(&webdata,index,Val);
            if(storedHumidity!=humidity) {
                storedHumidity=humidity;
                if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
                    M5.Lcd.setTextSize(1);
                    M5.Lcd.fillRect(212,145,105,25,TFT_BLACK);
                    M5.Lcd.setCursor(212,165);
                    M5.Lcd.setTextColor(TFT_WHITE);
                    M5.Lcd.printf("%.1f%%",humidity);
                    xSemaphoreGive(xScreen);
                }
            }
            //storeWrite(&webdata,index,Val);
        }
        //Serial.printf("SHT30 High watermark=%d\n",uxTaskGetStackHighWaterMark(nullptr));
        vTaskDelay(I2C_DEVICE_RATE_MS/portTICK_RATE_MS);
    }
}
