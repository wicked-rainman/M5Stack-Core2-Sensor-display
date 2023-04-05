#include "BMP280drv.h"
void BMP280drv(void *summat) {    
Adafruit_BMP280 bme;
float temp,press;
float storedTemp,storedPress;
char index[20];
char Val[30];
    if(xSemaphoreTake(xI2c,portMAX_DELAY)) {
        bme.begin(0x76);
        xSemaphoreGive(xI2c);
        storedTemp=0;
        storedPress=0;
    }
    while(true) {
        if(xSemaphoreTake(xI2c,portMAX_DELAY)) {
            snprintf(index,20,"%s:BMP280:",mHostname);
            press = (bme.readPressure()/100);
            temp=bme.readTemperature();
            snprintf(Val,30,"%.2fMb %.2fC",press,temp);
            xSemaphoreGive(xI2c);
            storeWrite(&webdata,index,Val);
            if(storedTemp!=temp) {
                storedTemp=temp;
                if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
                    M5.Lcd.fillRect(0,145,105,25,TFT_BLACK);
                    M5.Lcd.setTextSize(1);
                    M5.Lcd.setCursor(1,165);
                    M5.Lcd.setTextColor(TFT_WHITE);
                    M5.Lcd.printf("%.1fC",temp);
                    xSemaphoreGive(xScreen);
                }
            }
            if(storedPress!=press) {
                storedPress=press;
                if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
                    M5.Lcd.setTextSize(1);
                    M5.Lcd.fillRect(106,145,105,25,TFT_BLACK);
                    M5.Lcd.setCursor(106,165);
                    M5.Lcd.setTextColor(TFT_WHITE);
                    M5.Lcd.printf("%.1f",press);
                    xSemaphoreGive(xScreen);
                }
            }
        }
        //Serial.printf("BMP280 High watermark=%d\n",uxTaskGetStackHighWaterMark(nullptr));
        vTaskDelay(I2C_DEVICE_RATE_MS/portTICK_RATE_MS);
    }
}
