#include <Arduino.h>
#include <Adafruit_SGP30.h>
#include <M5_ENV.h>
#include <Wire.h>
#include <M5Atom.h>
#include "udp_utils.h"
#include "string_utils.h"
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
const char *mHostname = "AtomWea";
const char* ssid = "xxxxx";
const char* pass = "xxxxxx";
IPAddress SendIP(192, 168, 68, 255);
SemaphoreHandle_t xUdp = NULL;
bool M5CORE2_SCREEN;
//-------------------------------------------------
// end of Includes for standard startup code
//-------------------------------------------------

SHT3X sht30;
QMP6988 qmp6988;
Adafruit_SGP30 sgp;
#define UDPTIME_PORT 6123
#define TALK_PORT 6124

void OutputSensorData();
void ReadSHT30();
void ReadSGP30();
void ReadQMP6988();
WiFiUDP UdpWeather, UdpTime;
int Tvoc, Eco2,packetSize,payloadSize = 0;
float Temperature, Humidity, Pressure;
static char timePacket[100];
static char outstr[100];
