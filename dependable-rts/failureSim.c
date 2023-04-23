#include <stdlib.h>
#include <stdio.h>
#include "failureSim.h"
#include "systemPorts.h"
#include "stm32f4xx_rng.h"

#define CAN_PORT0   (CAN_TypeDef *)(CAN1)
#define CAN_PORT1   (CAN_TypeDef *)(CAN2)


FailureSim failureSim = initFailureSim();


/* CAN */

void monitor_can_failure(Can *obj, int unused){
    obj->port = CAN_PORT1;
}


void monitor_can_restore(Can *obj, int unused){
    obj->port = CAN_PORT0;
}


/* Failure */

void leave_failure_mode(FailureSim *self, int unused){
    SCI_WRITE(&sci0, "[FM]: Leave Failure mode\n");
    self->failMode = 0;
    SYNC(&can0, monitor_can_restore, 0);
}


void enter_failure1(FailureSim *self, int unused){
    SCI_WRITE(&sci0, "[FM]: Mode F1, need to restore mannually\n");
    self->failMode = 1;
    SYNC(&can0, monitor_can_failure, 0);
}


void enter_failure2(FailureSim *self, int unused){
    int delay = gen_rand_num(10, 30);
    char failInfo[64];
    snprintf(failInfo, 64, "[FM]: Mode F2, restore automatcially in %d s\n", delay);
    SCI_WRITE(&sci0, failInfo);
    self->failMode = 2;
    SYNC(&can0, monitor_can_failure, 0);
    AFTER(SEC(delay), self, leave_failure_mode, 0);
}


void enter_failure3(FailureSim *self, int unused){
    // unplugged your can cable
    // shoule be automatically restore if connect it again
}


void enter_failure_mode(FailureSim *self, int mode){
    switch (mode)
    {
    case 0:
        leave_failure_mode(self, 0);
        break;
    case 1:
        enter_failure1(self, 0);
        break;
    case 2:
        enter_failure2(self, 0);
        break;
    default:
        SCI_WRITE(&sci0, "[FM WARN]: undefined silent failure mode:");
        SCI_WRITECHAR(&sci0, mode);
        SCI_WRITE(&sci0, "\n");
        break;
    }
}


/* Utils */

int gen_rand_num(int min, int max){
    while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET)
        ;
    return min + RNG_GetRandomNumber() % (max - min + 1);
}


/* Information */

void printFailureSimVerbose(FailureSim *self, int unused){
    // to implement
}
