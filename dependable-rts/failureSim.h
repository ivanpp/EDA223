#ifndef SILENT_FAIL_H
#define SILENT_FAIL_H

#include "TinyTimber.h"
#include "canTinyTimber.h"


typedef struct{
    Object super;
    int failMode; // 0, 1, 2
    Msg abortMessage; 
} FailureSim;

#define initFailureSim() { \
    initObject(), \
    0, \
}

extern FailureSim failureSim;

/* CAN */
void empty_can_interrupt(Can *obj, int unused);
void simulate_can_failure(Can *obj, int unused);
void simulate_can_restore(Can *obj, int unused);
/* Failure */
void toggle_failure1(FailureSim *self, int unused);
void enter_failure1(FailureSim *self, int unused);
void enter_failure2(FailureSim *self, int unused);
void enter_failure3(FailureSim *self, int unused);
void enter_failure_mode(FailureSim *self, int mode);
/* Utils */
int gen_rand_num(int min, int max);
void notify_failure(FailureSim *, int);
/* Information */
void print_failureSim_verbose(FailureSim *self, int unused);

#endif