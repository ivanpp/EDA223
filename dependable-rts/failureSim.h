#ifndef SILENT_FAIL_H
#define SILIENT_FAIL_H

#include "TinyTimber.h"
#include "canTinyTimber.h"


typedef struct{
    Object super;
    int failMode; // 0, 1, 2
} FailureSim;

#define initFailureSim() { \
    initObject(), \
    0, \
}

extern FailureSim failureSim;

/* CAN */
void monitor_can_failure(Can *obj, int unused);
void monitor_can_restore(Can *obj, int unused);
/* Failure */
void enter_failure1(FailureSim *self, int unused);
void enter_failure2(FailureSim *self, int unused);
void enter_failure3(FailureSim *self, int unused);
void enter_failure_mode(FailureSim *self, int mode);
/* Utils */
int gen_rand_num(int min, int max);
/* Information */
void print_failureSim_verbose(FailureSim *self, int unused);

#endif