#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
struct Store {
        char *name;
        char *value;
        struct Store *next;
};
extern SemaphoreHandle_t xShmem;
bool storeWrite(struct Store * ,char*, char*);
bool storeGet(struct Store *, char*, char[]);
bool storeRead(struct Store *, char [], char []);
bool storeDelete(struct Store *,char[]);
void storeWipe(struct Store *);
