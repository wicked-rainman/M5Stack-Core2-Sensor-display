#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "udp_utils.h"
#include "string_utils.h"

HardwareSerial GPSRaw(2);
WiFiUDP Udptime;
#define MAX_GPS_SENTENCE_SIZE 100
#define GNRMC_FIELD_COUNT 12
#define UDPTIME_PORT 6123

static char ch=0x00;
static char GpsSentence[100];
static char Udpline[50];
static int SentenceSize=0;
static char GpsData[GNRMC_FIELD_COUNT][50];
static char day[3]={'0','0',0x00};
static char month[3]={'0','0',0x00};
static char year[3]={'0','0',0x00};
static char hours[3]={'0','0',0x00};
static char mins[3]={'0','0',0x00};
static char secs[3]={'0','0',0x00};
static char Substr[2];
int DlmCount(char, char *); 
//-------------------------------------------------
// Includes for standard startup code
//-------------------------------------------------
#include <ArduinoOTA.h>
#include <ESP32Ping.h>
#include <ESPmDNS.h>
#define DIAG_PORT 6125
#define TASK_COUNT 1
#define WIFI_STARTUP_TIME_MS 500
#define GATEWAY_PING_MAX_FAILS 10
#define UDPWRITE_WAIT 1000
void WiFiStationConnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiGotIP(WiFiEvent_t, WiFiEventInfo_t);
void WiFiStationDisconnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiLostIP(WiFiEvent_t, WiFiEventInfo_t);
void handleOTAStart();
void handleOTAProgress(unsigned int, unsigned int);
void handleOTAEnd();
void GatewayPing(void *);
WiFiUDP UdpDiag;
TaskHandle_t tasks[TASK_COUNT];
const char *mHostname = "AtomTai";
const char* ssid = "xxxxx";
const char* pass = "xxxxx";
IPAddress SendIP(192, 168, 68, 255);
SemaphoreHandle_t xUdp = NULL;
