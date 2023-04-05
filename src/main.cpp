#include "WeatherRtos.h"
//-------------------------------------------------------------------
// Simple weather sensor display that runs on an M5stack Core2.
// Reads various UDP ports to collect sensor values and draws those
// values on the built in M5Stace Core2 display
// Accepts over the air updates
//-------------------------------------------------------------------
void setup() {
  bool core=false;
  char diag[100];
  int I2cDeviceList[127];
  int I2cDeviceCount = 0;
  M5.begin(true, true, true);
  Serial.begin(115200);
  M5.Lcd.setBrightness(80);
  M5.Lcd.setFreeFont(FS12);
  M5.Lcd.clear();
  M5CORE_SCREEN=true;
  //-------------------------------------------------
  // Set up WiFi and event handling
  //-------------------------------------------------
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiStationConnected,ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED); 
  WiFi.onEvent(WiFiLostIP,ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED) vTaskDelay(WIFI_STARTUP_TIME_MS/portTICK_PERIOD_MS);
  //-------------------------------------------------
  // Set up OTA firmware updating
  //-------------------------------------------------
  ArduinoOTA.setHostname(mHostname);
  ArduinoOTA.onStart(handleOTAStart);
  ArduinoOTA.onProgress(handleOTAProgress);
  ArduinoOTA.onEnd(handleOTAEnd);
  ArduinoOTA.begin();
  //-------------------------------------------------
  // Set up RTOS Semaphores
  //-------------------------------------------------
  xScreen = xSemaphoreCreateMutex();
  xWifi = xSemaphoreCreateMutex();
  xI2c = xSemaphoreCreateMutex();
  xShmem = xSemaphoreCreateMutex();
  xSemaphoreGive (xScreen);
  xSemaphoreGive (xWifi);
  xSemaphoreGive (xI2c);
  xSemaphoreGive (xShmem);
  //-------------------------------------------------
  // Set up UDP ports
  //-------------------------------------------------
  UdpDiag.begin(DIAG_PORT);
  UdpTime.begin(TAI_PORT);
  UdpTalk.begin(TALK_PORT);
  Wire.begin();
  vTaskDelay(500/portTICK_RATE_MS);
  //-------------------------------------------------
  // Broadcast start up message
  //-------------------------------------------------
  snprintf(diag,90,"%s: Lives %s\n",mHostname,WiFi.localIP().toString().c_str());
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
  //-------------------------------------------------
  // Look for locally connected I2C devices and load
  // tasks(drivers) as required
  //-------------------------------------------------
  I2cDeviceCount=I2cScan(&Wire,I2cDeviceList,xI2c,portMAX_DELAY);
  LoadI2cDrivers(I2cDeviceCount,I2cDeviceList,&UdpDiag,DIAG_PORT,xWifi,UDPWRITE_WAIT,core);
  //-------------------------------------------------
  // Start task to ping gateway in order to detect
  // any network problems
  //-------------------------------------------------
  xTaskCreatePinnedToCore(GatewayPing,"GatewayPing",5000,NULL,1, &tasks[task_count++],(int) core);
  TOGGLE_CORE;
  //-------------------------------------------------
  // Start up battery monitoring and UDP listners
  //-------------------------------------------------
  xTaskCreatePinnedToCore(UpdateBatteryStatus,"UBS",5000,NULL,2,&tasks[task_count++],(int) core);
  TOGGLE_CORE;
  xTaskCreatePinnedToCore(ReadUdpTime,"RUT", 5000, NULL,2,&tasks[task_count++],(int) core);
  TOGGLE_CORE; 
  xTaskCreatePinnedToCore(ReadTalkPort,"RTP",5000,NULL,2,&tasks[task_count++], (int) core );
  TOGGLE_CORE;
  xTaskCreatePinnedToCore(ReadUdpDiag,"RUD",5000, NULL,2,&tasks[task_count++], (int) core);
  //-------------------------------------------------
  // Start up web serving
  //-------------------------------------------------
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_OnConnect);
  server.begin();
}
//------------------------------------------------
// Task ReadUdpDiag
// Listen for and display any network diag broadcasts
//------------------------------------------------
void ReadUdpDiag(void *MagicWilderbeast) {
  char udpChars[200];
  int end_of_line=30;
  boolean circleState=false;
  while(true) {
    if(UdpRead(&UdpDiag, udpChars, sizeof(udpChars),xWifi,UDPREAD_WAIT)) {
      udpChars[end_of_line]=0x0;
      if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
        circleState ? circleState=false : circleState=true;
        circleState ? M5.Lcd.fillCircle(170,225,5,TFT_GREEN):M5.Lcd.fillCircle(170,225,5,TFT_BLACK);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.fillRect(0,180,320,25,TFT_BLACK);
        M5.Lcd.setCursor(1,200);
        M5.Lcd.printf("%s",udpChars);
        xSemaphoreGive(xScreen);
      }
    }    
    vTaskDelay(250/portTICK_RATE_MS);
  }
}
//------------------------------------------------
void ReadTalkPort(void *whatever) {
  char udpChars[200];
  char udpArray[10][80];
  bool circleState = false;
  while(true) {
    if(UdpRead(&UdpTalk, udpChars, sizeof(udpChars),xWifi,UDPREAD_WAIT)) {
      if (xSemaphoreTake(xScreen, portMAX_DELAY)) {
        circleState ? circleState=false : circleState=true;
        circleState ? M5.Lcd.fillCircle(150,225,5,TFT_RED):M5.Lcd.fillCircle(150,225,5,TFT_BLACK);
        xSemaphoreGive(xScreen);
      }
      if(!strncmp("AtomWea:MULTI2", udpChars,14)) ProcessUdpWeather(udpChars);
      else if(!strncmp("AtomLux:BH1750", udpChars,14)) ProcessUdpLux(udpChars);
      else if(!strncmp("AtomDst:SDS011", udpChars,14)) ProcessUdpDust(udpChars);
    }
    vTaskDelay(250/portTICK_RATE_MS);
  }
}
//-----------------------------------------------------
void ProcessUdpDust(char *udpChars) {
  int pm25 = 0;
  int pm10=0;
  static int StoredPm25=0;
  static int StoredPm10=0;
  char udpArray[8][20];
  char outstr[100];
  Str2Array(8,20, " ",udpChars,udpArray[0]);
  pm25 = atoi(udpArray[3]);
  pm10=atoi(udpArray[4]);
  outstr[0]=0x00;
  strcat(outstr, udpArray[3]);
  strcat(outstr," ");
  strcat(outstr, udpArray[4]);
  storeWrite(&webdata, udpArray[0],outstr);  
  if(StoredPm25 != pm25) {
    StoredPm25 = pm25;
    if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
      M5.Lcd.fillRect(0,115,105,25,TFT_BLACK);
      M5.Lcd.setCursor(1,135);
      M5.Lcd.setTextSize(1);
      if(pm25 >=15) M5.Lcd.setTextColor(TFT_RED);
      else if(pm25>=8) M5.Lcd.setTextColor(TFT_ORANGE);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.printf("%d",StoredPm25);
      xSemaphoreGive(xScreen);
    }
  }
  if(StoredPm10 != pm10) {
    StoredPm10 = pm10;
    if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
      M5.Lcd.fillRect(106,115,105,25,TFT_BLACK);
      M5.Lcd.setCursor(106,135);
      M5.Lcd.setTextSize(1);
      if(pm10 >=20) M5.Lcd.setTextColor(TFT_RED);
      else if(pm10 >=10 ) M5.Lcd.setTextColor(TFT_ORANGE);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.printf("%d",StoredPm10);
      xSemaphoreGive(xScreen);
    }
  }
}
//------------------------------------------------
void ProcessUdpWeather(char *UdpChars) {
  int humidity = 0;
  static char StoredTemperature[10];
  static char StoredPressure[10];
  static char StoredTVOC[10];
  static char StoredEco2[10];
  static char StoredHumidity[10];
  char udpArray[8][20];
  char output[200];
  int IntTemp, IntEco2, IntTVOC,field_count,k;
  float FloatHumidity;
  //Place new data in the linklist store
  field_count=Str2Array(8, 20," ", UdpChars,udpArray[0]);
  output[0]=0x00;
  for(k=3;k<field_count;k++) {
    strcat(output,udpArray[k]);
    strcat(output," ");
  }
  storeWrite(&webdata,udpArray[0], output);

  //Display temperature
  if (memcmp(udpArray[3], StoredTemperature, 10)) {
    IntTemp = atof(udpArray[3]);
    if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
      if (IntTemp < 1) M5.Lcd.setTextColor(TFT_BLUE); 
      else if (IntTemp <= 5) M5.Lcd.setTextColor(TFT_CYAN);
      else if (IntTemp <= 11) M5.Lcd.setTextColor(TFT_GREEN);
      else if (IntTemp <= 20) M5.Lcd.setTextColor(TFT_YELLOW);
      else M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.fillRect(0, 55, 105, 25, TFT_BLACK);
      M5.Lcd.setCursor(1, 75);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s C", udpArray[3]);
      xSemaphoreGive(xScreen);
    }
    strncpy(StoredTemperature, udpArray[3], 10);
  }
  //Display presssure
  if (memcmp(udpArray[4], StoredPressure, 10)) {
    if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
      if (atof(udpArray[4]) < 1000.0) M5.Lcd.setTextColor(TFT_RED);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.fillRect(106, 55, 105, 25, TFT_BLACK);
      M5.Lcd.setCursor(106, 75);
      M5.Lcd.setTextSize(1);    
      M5.Lcd.printf("%s", udpArray[4]);
      xSemaphoreGive(xScreen);
    }
    strncpy(StoredPressure, udpArray[4], 10);
  }
      //Display humidity
  if(memcmp(udpArray[5],StoredHumidity,10)) {
    strncpy(StoredHumidity, udpArray[5], 10);
    FloatHumidity = atof(udpArray[5]);
    if(xSemaphoreTake(xScreen,portMAX_DELAY)){
      if(FloatHumidity > 90) M5.Lcd.setTextColor(TFT_RED);
      else if(FloatHumidity > 80) M5.Lcd.setTextColor(TFT_ORANGE);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.fillRect(212,55,105,25,TFT_BLACK);
      M5.Lcd.setCursor(213,75);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s%%",StoredHumidity);
      xSemaphoreGive(xScreen);
    }
  }
      //Display TVOC
  if(memcmp(udpArray[6],StoredTVOC,10)) {
    strncpy(StoredTVOC, udpArray[6], 10);
    IntTVOC=atoi(udpArray[6]);
    if(xSemaphoreTake(xScreen,portMAX_DELAY)){
      if(IntTVOC > 600) M5.Lcd.setTextColor(TFT_RED);
      else if(IntTVOC > 300) M5.Lcd.setTextColor(TFT_ORANGE);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.fillRect(0,85,105,25,TFT_BLACK);
      M5.Lcd.setCursor(1,105);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s",StoredTVOC);
      xSemaphoreGive(xScreen);
    }
  }
      //Display Eco2
  if(memcmp(udpArray[7],StoredEco2,10)) {
    strncpy(StoredEco2, udpArray[7], 10);
    IntEco2 = atoi(udpArray[7]);
    if(xSemaphoreTake(xScreen,portMAX_DELAY)){
      if(IntEco2 > 600) M5.Lcd.setTextColor(TFT_RED);
      else if(IntEco2 > 500) M5.Lcd.setTextColor(TFT_ORANGE);
      else M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.fillRect(106,85,105,25,TFT_BLACK);
      M5.Lcd.setCursor(106,105);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s",StoredEco2);
      xSemaphoreGive(xScreen);
    }
  }
}
//--------------------------------------------------------------------------------
void ReadUdpTime(void *parms) {
  int hrs, mins, secs;
  int StoredHours, StoredMins;
  char timeArray[3][5];
  char udpArray[2][10];
  char udpChars[200];
  bool circleState=true;
  while(true) {
    if(UdpRead(&UdpTime, udpChars, sizeof(udpChars),xWifi,portMAX_DELAY)) {
      storeWrite(&webdata,"AtomTAI::",udpChars);
      Str2Array(2,10," ",udpChars,udpArray[0]);
      Str2Array(3,5,":",udpArray[1],timeArray[0]);
      hrs = atoi(timeArray[0]);
      mins = atoi(timeArray[1]);
      secs = atoi(timeArray[2]);
      if (hrs != StoredHours) {
        StoredHours = hrs;
        if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
          M5.Lcd.fillRect(60, 0, 59, 38, TFT_BLACK);
          M5.Lcd.setCursor(61, 34);
          M5.Lcd.setTextSize(2);
          M5.Lcd.setTextColor(TFT_WHITE);
          M5.Lcd.printf("%02d:", hrs);
          xSemaphoreGive(xScreen);
        }
      }
      if (mins != StoredMins) {
        StoredMins = mins;
        if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
          M5.Lcd.fillRect(130, 0, 59, 38, TFT_BLACK);
          M5.Lcd.setCursor(131, 34);
          M5.Lcd.setTextSize(2);
          M5.Lcd.setTextColor(TFT_WHITE);            
          M5.Lcd.printf("%02d:", mins);
          xSemaphoreGive(xScreen);
        }
      }
      //Secs
      if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
        circleState ? circleState=false : circleState=true;
        circleState ? M5.Lcd.fillCircle(190,225,5,TFT_BLUE):M5.Lcd.fillCircle(190,225,5,TFT_BLACK);
        M5.Lcd.fillRect(200, 0, 59, 38, TFT_BLACK);
        M5.Lcd.setCursor(201, 34);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.printf("%02d", secs);
        xSemaphoreGive(xScreen);
      }
    }
    vTaskDelay(UDPTIME_WAIT / portTICK_PERIOD_MS);
  }
}
//--------------------------------------------------------------------------------
void ProcessUdpLux(char *UdpChars) {
  static char udpArray[8][20];
  static char lux[20];
  Str2Array(8, 20, " ",UdpChars,udpArray[0]);
  storeWrite(&webdata, udpArray[0],udpArray[3]);
  if(memcmp(udpArray[3],lux, sizeof(udpArray[3]))) {
    strncpy(lux,udpArray[3],10);
    if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
      M5.Lcd.fillRect(212,85,105,25,TFT_BLACK);
      M5.Lcd.setCursor(213,105);
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s",lux);
      xSemaphoreGive(xScreen);
    }
  }
}
//--------------------------------------------------------------------
void UpdateBatteryStatus(void *whatever) {
  int StoredBatteryLevel = 1;
  int BatteryLevel = 0;
  int CellColours[4];
  char diag[100];
  
  //Draw the battery icon outline just once
  M5.Lcd.drawLine(2, 215, 54, 215, TFT_WHITE);
  M5.Lcd.drawLine(2, 230, 54, 230, TFT_WHITE);
  M5.Lcd.drawLine(2, 215, 2, 230, TFT_WHITE);
  M5.Lcd.drawLine(54, 215, 54, 230, TFT_WHITE);
  M5.Lcd.fillRect(54, 218, 6, 9, TFT_WHITE);

  while(true) {
    M5.update();

    BatteryLevel = round(axp.GetBatteryLevel());
    if (BatteryLevel != StoredBatteryLevel) {
      snprintf(diag,80,"%s: Battery %.1f%% Draw %.1fma Charge %.1fma\n",mHostname,
        axp.GetBatteryLevel(),axp.GetBatPower(),axp.GetBatChargeCurrent());
      UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
      StoredBatteryLevel = BatteryLevel;

      if(BatteryLevel >=75){
        //memset(CellColours, TFT_GREEN, 4);
        CellColours[0] = TFT_GREEN;
        CellColours[1] = TFT_GREEN;
        CellColours[2] = TFT_GREEN;
        CellColours[3] = TFT_GREEN;
      }
      else if(BatteryLevel >=50) {
        CellColours[0] = TFT_GREEN;
        CellColours[1] = TFT_GREEN;
        CellColours[2] = TFT_GREEN;
        CellColours[3] = TFT_BLACK;
      }
      else if (BatteryLevel >= 25) {
        CellColours[0] = TFT_GREEN;
        CellColours[1] = TFT_GREEN;
        CellColours[2] = TFT_BLACK;
        CellColours[3] = TFT_BLACK;
      }
      else {
        CellColours[0] = TFT_GREEN;
        CellColours[1] = TFT_BLACK;
        CellColours[2] = TFT_BLACK;
        CellColours[3] = TFT_BLACK;
      }
      if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.setCursor(70, 230);
        M5.Lcd.fillRect(70, 210, 55, 25, TFT_BLACK);
        M5.Lcd.printf("%d%%", BatteryLevel);
        M5.Lcd.fillRect(5, 217, 10, 11, CellColours[0]);
        M5.Lcd.fillRect(17, 217, 10, 11,CellColours[1]);
        M5.Lcd.fillRect(29, 217, 10, 11,CellColours[2]);
        M5.Lcd.fillRect(41, 217, 10, 11,CellColours[3]);
        xSemaphoreGive(xScreen);
      }
    }
    vTaskDelay(60000/portTICK_RATE_MS);
  }
}
//--------------------------------------------------------------------
// Standard code block start
//--------------------------------------------------------------------
void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Lost IP connection");
  WiFi.disconnect(true);
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("Allocated new IP: ");
  Serial.println(WiFi.localIP());
  MDNS.begin(mHostname);
}
//--------------------------------------------------------------------
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Access point disconnected");
  WiFi.begin(ssid, pass);
}
//--------------------------------------------------------------------
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to access point");
}
//--------------------------------------------------------------------
void handleOTAStart() {
  static int k;
  static char diag[100];
  for(k=0;k<task_count;k++) {
    vTaskDelete(tasks[k]);
  }
  server.close(); //Stop web serving
  snprintf(diag,60,"%s:Ota: FW update\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
  if(M5CORE_SCREEN) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(50,70);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.printf("OTA update");
  }
}
//--------------------------------------------------------------------
void handleOTAProgress(unsigned int progress, unsigned int total) {
  if(M5CORE_SCREEN) {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.fillRect(140,80,50,25,TFT_BLACK);
    M5.Lcd.setCursor(50,100);
    M5.Lcd.printf("Progress: %u%%", (progress / (total / 100)));
  }
}
//--------------------------------------------------------------------
void handleOTAEnd() {
  static char diag[100];
  snprintf(diag,90,"%s:Ota: Done.\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
  if(M5CORE_SCREEN) {
    M5.Lcd.setCursor(50,130);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.printf("Complete");
  }
  vTaskDelay(1000/portTICK_RATE_MS);
  ESP.restart();
}
//--------------------------------------------------------------------
void GatewayPing(void *parms) {
  float Average;
  int failcount=0;
  char diag[100];
  bool circleState=false;
  while(true) {
    if(Ping.ping(WiFi.gatewayIP())) {
      failcount=0;
      Average=Ping.averageTime();
      snprintf(diag,90,"%s:Net: %.2frtt,%ddb\n",mHostname, Average,WiFi.RSSI());
      UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
      //This is me, so the message never gets caught by ReadUdpDiag routine.
      //Display message on the attached screen
      if(xSemaphoreTake(xScreen,portMAX_DELAY)) {
        circleState ? circleState=false : circleState=true;
        circleState ? M5.Lcd.fillCircle(210,225,5,TFT_RED):M5.Lcd.fillCircle(210,225,5,TFT_BLACK);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.fillRect(0,180,300,25,TFT_BLACK);
        M5.Lcd.setCursor(1,200);
        M5.Lcd.printf("%s",diag);
        xSemaphoreGive(xScreen);
      }
    } 
    else  {
      failcount++;
      if(failcount>GATEWAY_PING_MAX_FAILS) {
        snprintf(diag,90,"%s: Gatway ping failure > %d. Rebooting\n",mHostname, GATEWAY_PING_MAX_FAILS);
        UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
        vTaskDelay(1000/portTICK_RATE_MS);
        ESP.restart();
      }
    }
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
}
//--------------------------------------------------------------------
void loop() {
    ArduinoOTA.handle();
    server.handleClient();
}
//-------------------------------------------------------------------- 

void handle_OnConnect() {
  static bool circleState=false;
  if(xSemaphoreTake(xScreen, portMAX_DELAY)) {
    circleState ? circleState=false : circleState=true;
    circleState ? M5.Lcd.fillCircle(230,225,5,TFT_GREEN):M5.Lcd.fillCircle(230,225,5,TFT_BLACK);
    xSemaphoreGive(xScreen);
  }
  server.send(200, "text/html", SendHTML(1,1)); 
}
//-------------------------------------------------------------------- 

String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String ptr;
  char key[20];
  char val[80];
  ptr = "<!DOCTYPE html> <html>\n\
    <head><meta name=\"viewport\" content=\"width=device-width, \
    initial-scale=1.0, user-scalable=no\">\n\
    <meta http-equiv=\"refresh\" content=\"30\"><style>\
    table, th, td { border: 1px solid black;border-collapse: collapse;}</style>\
    </head>\n<body>\n\
    <table><tr><th>Device</th><th>Value</th></tr>\n";
  while(storeRead(&webdata,key,val)) {
    //Serial.printf("Web:%s %s\n",key,val);
    ptr+="<tr><td><b>";
    ptr+=key;
    ptr+="</b></td><td> ";
    ptr+=val;
    ptr+="</td><tr>";
  }
  ptr+="</table></body>\n<html>\n";
  return ptr;
}
