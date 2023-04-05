#include "dust.h"

void setup() {
  char diag[100];
  M5.begin(true,true,true);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1,26,32);
  xWifi = xSemaphoreCreateMutex();
  xSemaphoreGive(xWifi);
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
  UdpDiag.begin(DIAG_PORT);
  UdpDust.begin(TALK_PORT);
  snprintf(diag,90,"%s: Lives %s\n",mHostname,WiFi.localIP().toString().c_str());
  Serial.print(diag);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
  xTaskCreatePinnedToCore(GatewayPing,"GatewayPing",5000,NULL,1, &tasks[0],1);
  xTaskCreatePinnedToCore(GetDust,"GetDust",5000,NULL,1, &tasks[1],1);

}
void loop() {
  ArduinoOTA.handle();
}
//---------------------------------------------------------------
void GetDust(void *abc123) {
  float pm25,pm10;
  static char udpChars[256];
  static char outstr[100];
  //Read the dust sensor once every 2 mins in order to extend
  //its life (Failure during continuous use is 8000 hours. Doing
  //this may extend it's life to 24,000 hours - Nearly 3 years)
  while(true) {
    vTaskDelay(90000/portTICK_RATE_MS);
    SDScmd(WAKECMD);
    vTaskDelay(30000/portTICK_RATE_MS);
    SDSread(&pm25,&pm10);
    SDScmd(SLEEPCMD);
    //Clear out all the buffered TAI packets;
    UdpTime.begin(TAI_PORT);
    //Wait for, then read the next TAI packet
    while(true) {
  	  if (UdpRead(&UdpTime, udpChars, sizeof(udpChars))) {
    	  snprintf(outstr, 60, "%s:SDS011: %s %.0f %.0f\n", mHostname,udpChars,pm25,pm10);
    	  UdpDust.beginPacket(SendIP, TALK_PORT);
    	  UdpDust.print(outstr);
    	  UdpDust.endPacket();
    	  memset(outstr, 0, 60);
		    break;
      }
    }
  }
}

//---------------------------------------------------------------
int SDSread(float *p25, float *p10) {
	byte buffer;
	int value;
	int len = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is;
	int checksum_ok = 0;
	int error = 1;
	while ((Serial2.available() > 0) && (Serial2.available() >= (10-len))) 	{
		buffer = Serial2.read();
		value = int(buffer);
		switch (len) {
			case (0): {
        if (value != 170) len = -1;  
        break;
      }
			case (1): {
        if (value != 192) len = -1; 
        break;
      }
			case (2): {
        pm25_serial = value; 
        checksum_is = value; 
        break;
      }
			case (3): {
        pm25_serial += (value << 8); 
        checksum_is += value; 
        break;
      }
			case (4): {
        pm10_serial = value; 
        checksum_is += value; 
        break;
      }
			case (5): {
        pm10_serial += (value << 8); 
        checksum_is += value; 
        break;
      }
			case (6): {
        checksum_is += value; 
        break;
      }
			case (7): {
        checksum_is += value; 
        break;
      }
			case (8): {
        if (value == (checksum_is % 256)) checksum_ok = 1;  
          else { len = -1; }; 
          break;
      }
			case (9): {
        if (value != 171) len = -1;  
        break;
      }
	}
		len++;
		if (len == 10 && checksum_ok == 1) {
			*p10 = (float)pm10_serial/10.0;
			*p25 = (float)pm25_serial/10.0;
			len = 0; checksum_ok = 0; pm10_serial = 0.0; pm25_serial = 0.0; checksum_is = 0;
			error = 0;
		}
	}
	return error;
}
// --------------------------------------------------------
void SDScmd(uint8_t cmdstr[]) {
	for (uint8_t i = 0; i < 19; i++) {
		Serial2.write(cmdstr[i]);
	}
	Serial2.flush();
	while (Serial2.available() > 0) {
		Serial2.read();
	}
}
//--------------------------------------------------------------------
int UdpRead(WiFiUDP *port, char *pload, int psize) {
  int packetSize;
  int payloadSize;
  packetSize = port->parsePacket();
  if (packetSize) {
    payloadSize = port->read(pload, psize);
    if (payloadSize > 5) {
      pload[payloadSize-1] = '\0';
      return payloadSize;
    }
  }
  return 0;
}

//--------------------------------------------------------------------
void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Lost IP connection");
  WiFi.disconnect(true);
  WiFi.begin(ssid, pass);
}

//--------------------------------------------------------------------
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Allocated new IP");
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
  for(k=0;k<TASK_COUNT;k++) {
    vTaskDelete(tasks[k]);
  }
  snprintf(diag,60,"%s:Ota: FW update\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
}
void handleOTAProgress(unsigned int progress, unsigned int total) {
  // Display the progress of the update to the user
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}
void handleOTAEnd() {
  static char diag[100];
  snprintf(diag,90,"%s:Ota: Done.\n",mHostname);
  UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
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
      UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
    } 
    else  {
      failcount++;
      if(failcount>GATEWAY_PING_MAX_FAILS) {
        snprintf(diag,90,"%s: Gatway ping failure count exceeded. Rebooting\n",mHostname);
        UdpWrite(&UdpDiag,SendIP,DIAG_PORT,xWifi,UDPWRITE_WAIT,diag);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
}
