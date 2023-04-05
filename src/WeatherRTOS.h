#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
#include <WebServer.h>
//#include <FS.h>
//#include "Free_Fonts.h"
#include "udp_utils.h"
#include "string_utils.h"
#include "i2cscan.h"
#include "i2cdrivers.h"
#include "BMP280drv.h"
#include "SHT30drv.h"
#include "store.h"

AXP192 axp;
void ReadUdpTime(void *);
void ProcessUdpWeather(char *);
void ProcessUdpLux(char *);
void ProcessUdpDust(char *);
void ReadTalkPort(void *);
void UpdateBatteryStatus(void *);
void ReadUdpDiag(void *);

//-------------------------------------------------
// Includes for standard startup code
//-------------------------------------------------
#include <ArduinoOTA.h>
#include <ESP32Ping.h>
#include <ESPmDNS.h>
#define DIAG_PORT 6125
#define TALK_PORT 6124
#define TAI_PORT 6123
#define UDPTIME_WAIT 100
#define WIFI_STARTUP_TIME_MS 500
#define GATEWAY_PING_MAX_FAILS 10
#define UDPWRITE_WAIT 1000
#define UDPREAD_WAIT 1000
#define TOGGLE_CORE core ? core=false:core=true
#define MAXIMUM_TASK_COUNT 15

void WiFiStationConnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiGotIP(WiFiEvent_t, WiFiEventInfo_t);
void WiFiStationDisconnected(WiFiEvent_t, WiFiEventInfo_t);
void WiFiLostIP(WiFiEvent_t, WiFiEventInfo_t);
void handleOTAStart();
void handleOTAProgress(unsigned int, unsigned int);
void handleOTAEnd();
void GatewayPing(void *);
String SendHTML(uint8_t,uint8_t);
void handle_OnConnect();

WiFiUDP UdpDiag,UdpTime,UdpTalk;

TaskHandle_t tasks[MAXIMUM_TASK_COUNT];
int task_count=0;

const char *mHostname = "Core2Den";
const char* ssid = "xxxxx";
const char* pass = "xxxxx";
IPAddress SendIP(192, 168, 68, 255);
SemaphoreHandle_t xScreen = NULL;
SemaphoreHandle_t xWifi = NULL;
SemaphoreHandle_t xI2c  = NULL;
SemaphoreHandle_t xShmem = NULL;
bool M5CORE_SCREEN;
struct Store webdata;
WebServer server(80);
