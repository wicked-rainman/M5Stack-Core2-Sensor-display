#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <BH1750.h>
#include <ESP32Ping.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include "udp_utils.h"
#include "string_utils.h"
#include "i2cscan.h"

//-------------------------------------------------
// Includes for standard startup code
//-------------------------------------------------
#include <ArduinoOTA.h>
#include <ESP32Ping.h>
#include <ESPmDNS.h>
#define DIAG_PORT 6125
#define TAI_PORT 6123
#define LUX_PORT 6124
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
const char *mHostname = "AtomLux";
const char* ssid = "xxxx";
const char* pass = "xxxxx";
IPAddress SendIP(192, 168, 68, 255);
SemaphoreHandle_t xUdp = NULL;
bool M5CORE2_SCREEN;
//-------------------------------------------------
// end of Includes for standard startup code
//-------------------------------------------------

WiFiUDP UdpTime,UdpLux;
BH1750 LuxSensor;
