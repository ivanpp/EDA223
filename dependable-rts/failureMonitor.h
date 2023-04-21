#ifndef SILENT_FAIL_H
#define SILIENT_FAIL_H

#include "TinyTimber.h"
#include "canTinyTimber.h"


typedef struct{
    Object super;
    int failMode; // 0, 1, 2
} FailureMonitor;

#define initFailureMonitor() { \
    initObject(), \
    0, \
}

extern FailureMonitor failureMonitor;

/* CAN */
void monitor_can_failure(Can *obj, int unused);
void monitor_can_restore(Can *obj, int unused);
/* Failure */
void enter_failure1(FailureMonitor *self, int unused);
void enter_failure2(FailureMonitor *self, int unused);
void enter_failure3(FailureMonitor *self, int unused);
void enter_failure_mode(FailureMonitor *self, int mode);
/* Utils */
int gen_rand_num(int min, int max);
/* Information */
void printFailureMonitorVerbose(FailureMonitor *self, int unused);

#endif