#include "SensorServer.h"

//--------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------
void setup() {
  char diag[100];
  M5.begin(true,true,true);
  //Standard startup block for OTA
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
  xSemaphoreGive (xUdp);
  UdpDiag.begin(DIAG_PORT);
  snprintf(diag,90,"%s: Lives %s\n",mHostname,WiFi.localIP().toString().c_str());
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  xTaskCreatePinnedToCore(GatewayPing,"GatewayPing",5000,NULL,1, &tasks[0],1);
  //Standard startup block OTA end
  UdpTime.begin(UDPTIME_PORT);
  UdpWeather.begin(TALK_PORT);
  sgp.begin();
  qmp6988.init();
}

//---------------------------------------------------------------------
void loop() {
  static char udpChars[256];
  static char udpArray[2][100];
  static char tstrArray[3][10];
  static char outstr[100];
  static int storedMins=0;
  int t;
  ArduinoOTA.handle();
  if(UdpRead(&UdpTime,timePacket,sizeof(timePacket),xUdp,portMAX_DELAY)) {
    Str2Array(2,100," ",timePacket,udpArray[0]);
    Str2Array(3,10,":",udpArray[1],tstrArray[0]);
    t=atoi(tstrArray[1]);
    if(storedMins!=t) {//Read and send sensor data every 1 min. 
      storedMins=t;
      ReadSHT30();
      ReadSGP30();
      ReadQMP6988();
      snprintf(outstr, 80, "%s:MULTI2: %s %s:%s:%s% .1f %.1f %.1f %d %d\n",mHostname,
        udpArray[0],tstrArray[0],tstrArray[1],tstrArray[2], 
        Temperature, Pressure,Humidity,Tvoc,Eco2);
      vTaskDelay(500/portTICK_RATE_MS);//Collision avoidance for other talkers
      UdpWrite(&UdpWeather,SendIP,TALK_PORT,xUdp,UDPWRITE_WAIT,outstr);
      memset(timePacket, 0, 100);
    }    
  }
}

//--------------------------------------------------------------------
// OutputSensorData
//
// Go through each of the attached sensors collecting data.
// Store the data in globals
//---------------------------------------------------------------------
void OutputSensorData() {
  snprintf(outstr, 80, "%s:MULTI2: %.1f %.1f %.1f %d %d\n",
    timePacket, Temperature, Pressure,Humidity,Tvoc,Eco2);
  UdpWrite(&UdpWeather,SendIP,TALK_PORT,xUdp,UDPWRITE_WAIT,outstr);
  memset(outstr, 0, 50);
}
//----------------------------------------------------------------------

void ReadSHT30() {
  if (sht30.get() == 0) {
    Temperature = sht30.cTemp;
    Temperature = Temperature - 2.4;
    Humidity = sht30.humidity;
  } 
  else {
    Temperature = 0;
    Humidity = 0;
  }
}
//----------------------------------------------------------------------
void ReadSGP30() {
  if (sgp.IAQmeasure()) {
    Tvoc = sgp.TVOC;
    Eco2 = sgp.eCO2;      
  } 
  else {
    Tvoc = 0;
    Eco2 = 0;
  }
}
//----------------------------------------------------------------------
void ReadQMP6988() {
  Pressure = (qmp6988.calcPressure()/100.0);
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
  M5.dis.drawpix(10,0x00a000);
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
  snprintf(diag,60,"%s:OTA: FW update\n",mHostname);
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
        snprintf(diag,90,"%s:Net: Gatway ping failure count exceeded. Rebooting\n",mHostname);
        UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
}