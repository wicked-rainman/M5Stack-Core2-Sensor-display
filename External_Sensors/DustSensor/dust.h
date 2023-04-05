#include <Arduino.h>
#include <M5Atom.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Ping.h>
#include <ESPmDNS.h>
#include "udp_utils.h"

#define TAI_PORT 6123
#define TALK_PORT 6124
#define DIAG_PORT 6125
#define TASK_COUNT 2
#define UDPWRITE_WAIT 1000
#define GATEWAY_PING_MAX_FAILS 10


IPAddress SendIP(192, 168, 68, 255);
WiFiUDP UdpTime,UdpDust,UdpDiag;
TaskHandle_t tasks[TASK_COUNT];
SemaphoreHandle_t xWifi = NULL;
const char *ssid = "xxxxx";
const char *pass = "xxxxxx";
const char *mHostname = "AtomDst";

static uint8_t SLEEPCMD[19] = {0xAA, 0xB4, 0x06, 0x01, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0xFF,	0xFF, 0x05, 0xAB};
static uint8_t WAKECMD[19] = {0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0xAB};
static uint8_t CONTINUOUS_CMD[19] = {0xAA, 0xB4, 0x08, 0x01, 0x00, 0x00,0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0xAB};

int SDSread(float *, float *); 
void SDScmd(uint8_t[]);
int UdpRead(WiFiUDP* , char *, int);
void WiFiStationConnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiGotIP(WiFiEvent_t, WiFiEventInfo_t);
void WiFiStationDisconnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiLostIP(WiFiEvent_t, WiFiEventInfo_t);
void handleOTAStart();
void handleOTAProgress(unsigned int, unsigned int);
void handleOTAEnd();
void GatewayPing(void *parms);
void GetDust(void *parms);
