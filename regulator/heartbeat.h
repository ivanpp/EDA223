#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "TinyTimber.h"
#include "application.h"

#define DEFAULT_HEARTBEAT_PERIOD 5
#define MAX_HEARTBEAT_PERIOD 60
#define MAX_HEARTBEAT_DEADLINE 120
#define DEFAULT_HEARTBEAT_DEADLINE 0


typedef struct Heartbeat Heartbeat;

struct Heartbeat{
    Object super;
    PlayerMode mode; // register as CONDUCTOR/MUSICIAN
    int periodicity; // SEC
    int deadline;
    int enable;
    void (*heartbeatFunc)(Heartbeat *, int);
};

#define initHeartbeat(mode, func) { \
    initObject(), \
    mode, \
    SEC(DEFAULT_HEARTBEAT_PERIOD), \
    SEC(DEFAULT_HEARTBEAT_DEADLINE), \
    0, \
    func, \
}

void heartbeatConductor(Heartbeat *, int);
void heartbeatMusician(Heartbeat *, int);
void enableHeartbeat(Heartbeat *, int);
void disableHeartbeat(Heartbeat *, int);
int toggleHeartbeat(Heartbeat *, int);
int setHeartbeatPeriod(Heartbeat *, int);
int setHeartbeatDeadline(Heartbeat *, int);
void printHeartbeatInfo(Heartbeat *, int);

extern Heartbeat heartbeatCon;
extern Heartbeat heartbeatMus;

#endif