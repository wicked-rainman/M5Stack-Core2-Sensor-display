#include "store.h"

//-------------------------------------------------------------------------------
// Store wipe
//-------------------------------------------------------------------------------
void storeWipe(struct Store *ds) {
    struct Store *head;
    struct Store *next;
    xSemaphoreTake(xShmem,portMAX_DELAY);
    if(ds->name !=NULL) free(ds->name);
    if(ds->value !=NULL) free(ds->value);
    head=ds->next;
    while(head!=NULL) {
            printf("Removing %s\n",head->name);
            free(head->name);
            free(head->value);
            next=head->next;
            //free(head);
            head=next;
    }
    xSemaphoreGive(xShmem);
}
//----------------------------------------------------------
// storeDelete - Think this is okkay
//----------------------------------------------------------
bool storeDelete(struct Store *ds, char index[]) {
    struct Store *head;
    struct Store *previous;
    xSemaphoreTake(xShmem,portMAX_DELAY);
    head=ds;
    previous=NULL;
    while(head->name !=NULL) {
        if(! (bool) strcmp(head->name,index)) { //Key matches index
                free(head->name);
                free(head->value);
                if(previous==NULL) { //This is the first node
                        if(head->next==NULL) { //This first node is the only node
                                head->name=NULL;
                                head->value=NULL;
                                head->next=NULL;
                                xSemaphoreGive(xShmem);
                                return true;
                        }
                        else {//This is the first node but others follow
                                head->name=head->next->name;
                                head->value=head->next->value;
                                head->next=head->next->next;
                                xSemaphoreGive(xShmem);
                                return true;
                        }
                }
                else { //This isn't the first node.
                        previous->next=head->next; //Make the previous node point to the next node
                        free(head);     //Free this current node.
                        xSemaphoreGive(xShmem);
                        return true;
                }
        }
        if(head->next==NULL) {
            xSemaphoreGive(xShmem);
            return false;//End of the list
        }
        previous=head;
        head=head->next;
    }
    xSemaphoreGive(xShmem);
    return false;
}

//----------------------------------------------------------
// StoreRead - Think this is okay !!
// --------------------------------------------------------
bool storeRead(struct Store *ds, char index[], char retval[]) {
    static struct Store *head;
    static bool firstrun=true;
    xSemaphoreTake(xShmem,portMAX_DELAY);
    if(firstrun) { //Re-entrant function. At first call, store a copy of node address
        head=ds;
        firstrun=false;
    }
    if(head==NULL) {
        firstrun=true;
        xSemaphoreGive(xShmem);
        return false; //First node can never be null, last node must be.
    }
    if(head->name == NULL) {
        xSemaphoreGive(xShmem);
        return false; //First node can have a null name field if never used
    }
    strcpy(index,head->name);
    strcpy(retval,head->value);
    head=head->next;
    xSemaphoreGive(xShmem);
    return true;
}

//----------------------------------------------------------
// Get a value out of the linklist by specifying the index key
// Think this is okay !!
//----------------------------------------------------------
bool storeGet(struct Store *ds, char *index, char retval[]) {
    struct Store *head;
    xSemaphoreTake(xShmem,portMAX_DELAY);
    head=ds;
    while(head->name !=NULL) {
        if(! (bool) strcmp(head->name,index)) {
            strcpy(retval,head->value);
            xSemaphoreGive(xShmem);
            return true;
        }
        if(head->next==NULL) break;
        head=head->next;
    }
    xSemaphoreGive(xShmem);
    return false;
}
//---------------------------------------------------------
// storeWrite: Add or change a linklist key/value pair
//---------------------------------------------------------
bool storeWrite(struct Store *ds,char *index, char *val) {
    static bool firstrun=true;
    struct Store * previous;
    struct Store * head;
    xSemaphoreTake(xShmem,portMAX_DELAY);
    if(firstrun) {
            firstrun=false;
            ds->name=(char *)malloc(strlen(index)+1);
            if(ds->name==NULL) {
                xSemaphoreGive(xShmem);
                return false;
            }
            ds->value= (char *)malloc(strlen(val)+1);
            if(ds->value==NULL) {
                xSemaphoreGive(xShmem);
                return false;
            }
            strcpy(ds->name,index);
            strcpy(ds->value,val);
            ds->next=NULL;
            previous=ds;
            xSemaphoreGive(xShmem);
            return true;
    }
    head=ds;
    while(head!=NULL) {
            if(! (bool) strcmp(head->name, index)) { //Index matches a previously stored index value
                    free(head->value); //Free up the storage for the new value
                    head->value=(char *) malloc(strlen(val)+1); //Make new storage for the new value
                    if (head->value==NULL) {
                        xSemaphoreGive(xShmem);
                        return false; //Failed malloc
                    }
                    strcpy(head->value,val); //Copy new value into storage
                    xSemaphoreGive(xShmem);
                    return true; //Done, return true.
            }
            previous=head;
            head=head->next; //Move on to the next node
    }
    head=(struct Store *)malloc(sizeof(struct Store));
    if(head==NULL) {
        xSemaphoreGive(xShmem);
        return false; //Malloc failed
    }
    previous->next=head;
    head->name=(char *) malloc(strlen(index)+1);
    if(head->name==NULL) {
        xSemaphoreGive(xShmem);
        return false; //Malloc failed
    }
    strcpy(head->name,index);
    head->value=(char *) malloc(strlen(val)+1);
    if(head->value==NULL) {
        xSemaphoreGive(xShmem);
        return false; //Malloc failed
    }
    strcpy(head->value, val);
    head->next = NULL;
    xSemaphoreGive(xShmem);
    return true;
}


