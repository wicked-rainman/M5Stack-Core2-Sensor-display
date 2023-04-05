#include <Arduino.h>
#include <WiFi.h>
//#include "extDefines.h"
extern void UdpWrite(WiFiUDP *, IPAddress, int, SemaphoreHandle_t, TickType_t, char *);
void LoadI2cDrivers(int, int[127],WiFiUDP *, int, SemaphoreHandle_t,int, bool);
extern void SHT30drv(void *);
extern void BMP280drv(void *);
#define MAXIMUM_TASK_COUNT 15
#define TOGGLE_CORE core ? core=false:core=true
extern int task_count;
extern const char *mHostname;
extern TaskHandle_t tasks[MAXIMUM_TASK_COUNT];
