#include "lux.h"
float Lux;

void setup() {
  char diag[100];
  int I2cDeviceCount=0;
  int I2cDeviceList[127];
  int k;
  SemaphoreHandle_t xI2c  = NULL;
  Serial.begin(115200);
  Wire.begin(26,32);

  M5CORE2_SCREEN=false;
  Serial.begin(115200);
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiStationConnected,ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED); 
  WiFi.onEvent(WiFiLostIP,ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) vTaskDelay(100/portMAX_DELAY);
  ArduinoOTA.setHostname(mHostname);
  ArduinoOTA.onStart(handleOTAStart);
  ArduinoOTA.onProgress(handleOTAProgress);
  ArduinoOTA.onEnd(handleOTAEnd);
  ArduinoOTA.begin();
  xUdp = xSemaphoreCreateMutex();
  xI2c = xSemaphoreCreateMutex();
  xSemaphoreGive(xUdp);
  xSemaphoreGive(xI2c);
  UdpDiag.begin(DIAG_PORT);
  snprintf(diag,90,"%s: Lives %s\n",mHostname,WiFi.localIP().toString().c_str());
  Serial.print(diag);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  I2cDeviceCount=I2cScan(&Wire,I2cDeviceList,xI2c,portMAX_DELAY);
  snprintf(diag,90,"%s:I2c: Found %d devices\n",mHostname,I2cDeviceCount);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  for(k=0;k<I2cDeviceCount;k++) {
    vTaskDelay(300/portTICK_RATE_MS);
    snprintf(diag,90,"%s:I2C: Device %02x\n",mHostname,I2cDeviceList[k]);
    UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  }
  xTaskCreatePinnedToCore(GatewayPing,"GatewayPing",5000,NULL,1, &tasks[0],1);
  //Standard startup block OTA end
  LuxSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  //LuxSensor.configure(BH1750::CONTINUOUS_HIGH_RES_MODE);
  //LuxSensor.begin();
  UdpLux.begin(LUX_PORT);
  UdpTime.begin(TAI_PORT);
}

void loop() {
  static char udpChars[256];
  static char udpArray[2][100];
  static char tstrArray[3][10];
  static char outstr[100];
  static int storedSecs=0;
  int t;
  ArduinoOTA.handle();
  //Read the TAI. When the min value changes, read the sensor and output
  //the value.
  if (UdpRead(&UdpTime, udpChars, sizeof(udpChars),xUdp,portMAX_DELAY)) {
    Str2Array(2,100," ",udpChars,udpArray[0]);
    Str2Array(3,10,":",udpArray[1],tstrArray[0]);
    t=atoi(tstrArray[1]);
    if(storedSecs!=t) {
      storedSecs=t;
      Lux = LuxSensor.readLightLevel();
      snprintf(outstr, 90, "%s:BH1750: %s %s:%s:%s %.0f\n", mHostname,udpArray[0],
      tstrArray[0],tstrArray[1],tstrArray[2], Lux);
      UdpWrite(&UdpLux,SendIP,LUX_PORT,xUdp,UDPWRITE_WAIT,outstr);
      memset(outstr, 0, 90);
    }
  }
}
//--------------------------------------------------------------------
void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info){
printf("#Wifi connection lost\n");
  WiFi.disconnect(true);
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  printf("#Wifi allocated IP\n");
}
//--------------------------------------------------------------------
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  printf("#Wifi disconnected\n");
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  printf("#Wifi connected\n");
}
void handleOTAStart() {
  static int k;
  static char diag[100];
  for(k=0;k<TASK_COUNT;k++) {
    vTaskDelete(tasks[k]);
  }
  snprintf(diag,60,"%s:Ota: FW update\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
}
void handleOTAProgress(unsigned int progress, unsigned int total) {
  // Display the progress of the update to the user
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}
void handleOTAEnd() {
  static char diag[100];
  snprintf(diag,90,"%s:Ota: Done.\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  ESP.restart();
}

//--------------------------------------------------------------------
void GatewayPing(void *parms) {
  static float Average;
  static int failcount=0;
  static char diag[100];
  while(true) {
    if(Ping.ping(WiFi.gatewayIP())) {
      failcount=0;
      Average=Ping.averageTime();
      snprintf(diag,90,"%s:Net: %.2frtt,%ddb\n",mHostname, Average,WiFi.RSSI());
      UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
    } 
    else  {
      failcount++;
      if(failcount>GATEWAY_PING_MAX_FAILS) {
        snprintf(diag,90,"%s: Gatway ping failure count exceeded. Rebooting\n",mHostname);
        UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
}
