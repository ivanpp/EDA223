#ifndef REGULATOR_H
#define REGULATOR_H

#include "TinyTimber.h"
#include "canTinyTimber.h"
#include "sciTinyTimber.h"

#define BUFFER_SIZE 8

extern Can can0;
extern Serial sci0;

typedef struct{
    Object super;
    int head;
    int tail;
    int counter;
    int ready;
    CANMsg buffer[BUFFER_SIZE];
} CANBuffer;

#define initNegulator(){ \
    initObject(), \
    /*periodicity*/ 1000, \
    /*not running*/ 0, \
}


typedef struct{
    Object super;
    int periodicity;
    int running;
} Negulator;

#define initCANBuffer(){ \
    initObject(), \
    /*head*/ 0, \
    /*tail*/ 0, \
    /*counter*/ 0, \
    /*ready*/ 1, \
    {}, \
}


/* CANBuffer */
void buffer_push(CANBuffer *self, CANMsg *msg);
void buffer_push2(CANBuffer *self, CANMsg *msg);
CANMsg buffer_pop(CANBuffer *self, int unused);
void buffer_ready(CANBuffer *self, int unused);
void buffer_unready(CANBuffer *self, int unused);
int check_buffer_empty(CANBuffer *self, int unused);
int check_buffer_full(CANBuffer *self, int unused);
/* Negulator */
void handle_message(Negulator *self, CANMsg *msg);
void periodic_send(Negulator *self, int unused);
void reg_receiver(Negulator *self, int unused);

#endif