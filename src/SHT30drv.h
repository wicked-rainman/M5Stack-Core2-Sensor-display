#include <Arduino.h>
#include <M5Core2.h>
#include <SHT3x.h>
#include <WiFi.h>
#include <WiFiUdp.h>
extern const char *mHostname;
extern SemaphoreHandle_t xI2c;
extern SemaphoreHandle_t xWifi;
extern SemaphoreHandle_t xScreen;
extern SemaphoreHandle_t xShmem;
extern struct Store webdata;
#define I2C_DEVICE_RATE_MS 10000
extern WiFiUDP UdpDiag;
extern void UdpWrite(WiFiUDP *, IPAddress, int, SemaphoreHandle_t, TickType_t, char *);
extern bool storeWrite(struct Store * ,char*, char*);

