#include <Arduino.h>
#include <WiFiUdp.h>
#include <WiFi.h>
int UdpRead(WiFiUDP* , char *, int, SemaphoreHandle_t, TickType_t);
void UdpWrite(WiFiUDP *, IPAddress, int, SemaphoreHandle_t, TickType_t, char *);
