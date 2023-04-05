#include "udp_utils.h"
//--------------------------------------------------------------------------------
// RTOS complient udp read
// WiFiUDP class, char*, sizeof(char*), mutex, delay
//--------------------------------------------------------------------------------
int UdpRead(WiFiUDP *port, char *pload, int psize, SemaphoreHandle_t sem, TickType_t dly) {
  int packetSize;
  int payloadSize;
   if(xSemaphoreTake(sem, dly)) {
    packetSize = port->parsePacket();
    if (packetSize) {
      payloadSize = port->read(pload, psize);
      if (payloadSize >0) {
        pload[payloadSize-1] = '\0';
        xSemaphoreGive(sem);
        return payloadSize;
      }
    }
    xSemaphoreGive(sem);
   }
  return 0;
}
//--------------------------------------------------------------------
void UdpWrite(WiFiUDP *Mystream, IPAddress Sendip, int port, SemaphoreHandle_t sem, TickType_t dly,char *msg) {
  if(xSemaphoreTake(sem, dly)) {
    Mystream->beginPacket(Sendip, port);
    Mystream->print(msg);
    Mystream->endPacket();
    xSemaphoreGive(sem);
  }
}
