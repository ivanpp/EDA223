#include <stdlib.h>
#include <stdio.h>
#include "failureSim.h"
#include "systemPorts.h"
#include "network.h"
#include "heartbeat.h"
#include "stm32f4xx_rng.h"

#define CAN_PORT0   (CAN_TypeDef *)(CAN1)
#define CAN_PORT1   (CAN_TypeDef *)(CAN2)


FailureSim failureSim = initFailureSim();

/* CAN */

void empty_receiver(Can *obj, int unused){
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "CANMsg ignored\n");
    return;
    //SCI_WRITE(&sci0, "EMPTY CAN TRIGGLED\n");
}


void simulate_can_failure(Can *obj, int unused){
    obj->meth = (Method)empty_receiver;
    //obj->port = NULL;
}


void simulate_can_restore(Can *obj, int unused){
    obj->meth = (Method)receiver;
    //obj->port = CAN_PORT0;
}


/* Failure */

void leave_failure_mode(FailureSim *self, int unused){
    SCI_WRITE(&sci0, "[FM]: Leave Failure mode\n");
    self->failMode = 0;
    ABORT(self->abortMessage);
    SIO_WRITE(&sio0, 0); // lit
    SYNC(&can0, simulate_can_restore, 0);
    /*
    Rejoin Pipeline (after leaving failure mode)

    [1, 1, 1]                   [0, 1, 0]
    NODE_LOGIN_REQUEST ->
                                    handle_login_request()
                                <- NODE_LOGIN_CONFIRM
    node_login()
                                <- OBTAIN_CONDUCTORSHIP @CON
    change_conductor()
    [0, 0, 0]                   [0, 0, 0]
    */
}

void toggle_failure1(FailureSim *self, int unused)
{
    if(self->failMode == 1) {
        leave_failure_mode(self, 0);
    } 
    else {
        enter_failure1(self, 0);
    }
}

void enter_failure1(FailureSim *self, int unused){
    self->failMode = 1;
    SCI_WRITE(&sci0, "[FM]: Mode F1, need to restore manually\n");
    SIO_WRITE(&sio0, 1); // unlit
    SYNC(&can0, simulate_can_failure, 0);
    /*
    Failure Detection Pipeline (self)


    
    */

    // TODO: after implement detection (self's), this should be REMOVED
    //SYNC(&network, node_logout, 0);
}


void enter_failure2(FailureSim *self, int unused){
    self->failMode = 2;
    int delay = gen_rand_num(10, 30);
    char failInfo[64];
    snprintf(failInfo, 64, "[FM]: Mode F2, restore automatcially in %d s\n", delay);
    SCI_WRITE(&sci0, failInfo);
    SIO_WRITE(&sio0, 1); // unlit
    SYNC(&can0, simulate_can_failure, 0);
    //ABORT(self->abortMessage);
    self->abortMessage = AFTER(SEC(delay), self, leave_failure_mode, 0);
    // TODO: detect manually, this is only for test functionality
    //SYNC(&network, node_logout, 0);
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


// Only used for test
void notify_failure(FailureSim *self, int unused){
    CANMsg msg;
    construct_can_message(&msg, TEST_NOTIFY_FAILURE, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> set_node_offline()
}


/* Information */

void print_failureSim_verbose(FailureSim *self, int unused){
    // to implement
}
