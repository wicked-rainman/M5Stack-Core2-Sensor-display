#include "GPSRTC.h"

void setup() {
  char diag[100];
  //M5.begin(true,false,true);
  GPSRaw.begin(9600, SERIAL_8N1, 32, 26);
  //M5.dis.setBrightness(1);
   //Standard startup code start -----------------------------------
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiStationConnected,ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED); 
  WiFi.onEvent(WiFiLostIP,ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED) vTaskDelay(WIFI_STARTUP_TIME_MS/portTICK_PERIOD_MS);
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
  xTaskCreatePinnedToCore(GatewayPing,"GatewayPing",5000,NULL,1, &tasks[0],0);
  //Standard startup code end -----------------------------------
  Udptime.begin(UDPTIME_PORT);
}
void loop() {
  ArduinoOTA.handle();
  ch = 0x00;
  while (ch != '$') {
    if (GPSRaw.available()) ch = GPSRaw.read();
    else vTaskDelay(200/portTICK_RATE_MS);
  }
  SentenceSize = 1;
  GpsSentence[0] = ch;
  while ((ch != '\n') && (SentenceSize < MAX_GPS_SENTENCE_SIZE)) {
    if (GPSRaw.available()) {
      ch = GPSRaw.read();
      GpsSentence[SentenceSize++] = ch;
    }
    else vTaskDelay(200/portTICK_RATE_MS);
  }
  if (SentenceSize <= MAX_GPS_SENTENCE_SIZE) {
    if ((!memcmp("$GNRMC", GpsSentence, 6)) && (DlmCount(',',GpsSentence) == 12)) {
      GpsSentence[SentenceSize - 2] = '\0';
      Str2Array(50,12,",",GpsData[0],GpsSentence);
      memcpy(year,GpsData[9]+4,2);
      if(atoi(year)>20) {
        memcpy(day,GpsData[9],2);
        memcpy(month,GpsData[9]+2,2);
        memcpy(hours,GpsData[1],2);
        memcpy(mins,GpsData[1]+2,2);
        memcpy(secs,GpsData[1]+4,2);
        snprintf(Udpline,40,"%s/%s/%s %s:%s:%s\n",day,month,year,hours,mins,secs);
        UdpWrite(&Udptime,SendIP,UDPTIME_PORT,xUdp,UDPWRITE_WAIT,Udpline);
      }
    }
  }
}
//----------------------------------------------------------------------
int DlmCount(char dlm, char *targetStr) {
  int ctr = 0;
  int len = strlen(targetStr);
  for (int k = 0; k < len; k++) {
    if (targetStr[k] == dlm) ctr++;
  }
  return ctr;
}
//--------------------------------------------------------------------
void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info){
  WiFi.disconnect(true);
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){  
}
//--------------------------------------------------------------------
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
}
//--------------------------------------------------------------------
void handleOTAStart() {
  static int k;
  static char diag[100];
  for(k=0;k<TASK_COUNT;k++) {
    vTaskDelete(tasks[k]);
  }
  snprintf(diag,60,"%s:Ota: FW update\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
}
//--------------------------------------------------------------------
void handleOTAProgress(unsigned int progress, unsigned int total) {
}
//--------------------------------------------------------------------
void handleOTAEnd() {
  static char diag[100];
  snprintf(diag,90,"%s:Ota: Done.\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
  vTaskDelay(1000/portTICK_RATE_MS);
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
        snprintf(diag,90,"%s:Net Gatway ping failure exceeded. Rebooting\n",mHostname);
        UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xUdp,UDPWRITE_WAIT,diag);
        vTaskDelay(1000/portTICK_RATE_MS);
        ESP.restart();
      }
    }
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
}