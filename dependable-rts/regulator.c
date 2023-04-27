#include <string.h>
#include <stdio.h>
#include "regulator.h"


CANBuffer canBuffer = initCANBuffer();
Negulator regulator = initNegulator();


/* CANBuffer */

void buffer_push(CANBuffer *self, CANMsg *msg){
    if(self->counter < BUFFER_SIZE){
        memcpy(self->buffer+self->head, msg, sizeof(CANMsg));
        self->head = (self->head + 1) % BUFFER_SIZE;
        if (self->counter < BUFFER_SIZE)
            self->counter++;
    }
#ifdef DEBUG
    char debugInfo[64];
    snprintf(debugInfo, 64, "[BUF]: counter = %d\n", self->counter);
    SCI_WRITE(&sci0, debugInfo);
#endif
}


CANMsg buffer_pop(CANBuffer *self, int unused){
    CANMsg msg = self->buffer[self->tail];
    self->tail = (self->tail + 1) % BUFFER_SIZE;
    self->counter--;
#ifdef DEBUG
    char debugInfo[64];
    snprintf(debugInfo, 64, "[BUF]: counter = %d\n", self->counter);
    SCI_WRITE(&sci0, debugInfo);
#endif
    return msg;
}


void buffer_ready(CANBuffer *self, int unused){
    self->ready = 1;
}


void buffer_unready(CANBuffer *self, int unused){
    self->ready = 0;
}


int check_buffer_empty(CANBuffer *self, int unused){
    return self->counter == 0;
}


int check_buffer_full(CANBuffer *self, int unused){
    return self->counter == BUFFER_SIZE;
}


/* Negulator */

void handle_message(Negulator *self, CANMsg *msg){
    char msgInfo[64];
    snprintf(msgInfo, 64, "msg %d is handled correctly\n", msg->msgId);
    SCI_WRITE(&sci0, msgInfo);
    // also timer
}


void periodic_send(Negulator *self, int unused){
    // check buffer counter
    if(SYNC(&canBuffer, check_buffer_empty, unused)){
        SYNC(&canBuffer, buffer_ready, unused);
        SCI_WRITE(&sci0, "Periodic task\n");
        return;
    }
    AFTER(MSEC(2000), self, periodic_send, unused);
}


void reg_receiver(Negulator *self, int unused){
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    char debugInfo[64];
    if(canBuffer.ready && self->running == 0){ // ready
        handle_message(self, &msg); // 1. handle message
        SYNC(&canBuffer, buffer_unready, unused); // 2. set the buffer to not ready
        periodic_send(self, unused); // 3. start the priodic task (as a timer)
    }else{ // not ready
        if(SYNC(&canBuffer, check_buffer_full, 0)){ // not ready & buffer full
            snprintf(debugInfo, 64, "msg %d is discarded\n", msg.msgId);
            SCI_WRITE(&sci0, debugInfo);
            return;
        }else{ // not ready & buffer not full
            SYNC(&canBuffer, buffer_push, &msg);
        }
    }
}