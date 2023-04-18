#include <stdio.h>
#include "heartbeat.h"
#include "musicPlayer.h"
#include "toneGenerator.h"


Heartbeat heartbeatCon = initHeartbeat(CONDUCTOR, heartbeatConductor);
Heartbeat heartbeatMus = initHeartbeat(MUSICIAN, heartbeatMusician);


void heartbeatConductor(Heartbeat *self, int unused){
    if(!self->enable) 
        return;
    if(app.mode == self->mode){
        char statusInfo[64];
        snprintf(statusInfo, 64, "[HB/CON]: current tempo %d\n", musicPlayer.tempo);
        SCI_WRITE(&sci0, statusInfo);
    }else{
        self->enable = 0;
        return;
    }
    SEND(self->periodicity, self->deadline, self, heartbeatConductor, unused);
}


void heartbeatMusician(Heartbeat *self, int unused){
    if(!self->enable)
        return;
    if(app.mode == self->mode){
        int muteStatus = toneGenerator.isMuted;
        //int volume = toneGenerator.volume;
        if (muteStatus) {
            SCI_WRITE(&sci0, "[HB/MUS]: speaker muted\n");
        } else{
            SCI_WRITE(&sci0, "[HB/MUS]: speaker is not muted\n");
        }
    }else{
        self->enable = 0;
        return;
    }
    SEND(self->periodicity, self->deadline, self, heartbeatMusician, unused);
}


void enableHeartbeat(Heartbeat *self, int unused){
    self->enable = 1;
    self->heartbeatFunc(self, 0);
}


void disableHeartbeat(Heartbeat *self, int unused){
    self->enable = 0;
}


int toggleHeartbeat(Heartbeat *self, int unused){
    if(!self->enable)
        enableHeartbeat(self, unused);
    else
        disableHeartbeat(self, unused);
    printHeartbeatInfo(self, unused);
    return self->enable;
}


int setHeartbeatPeriod(Heartbeat *self, int val){
    val = val < self->deadline ? self->deadline : val;
    val = val > MAX_HEARTBEAT_PERIOD ? MAX_HEARTBEAT_PERIOD : val;
    self->periodicity = SEC(val);
    printHeartbeatInfo(self, 0);
    return val;
}


int setHeartbeatDeadline(Heartbeat *self, int val){
    val = val < 0 ? 0 : val;
    val = val > MAX_HEARTBEAT_DEADLINE ? MAX_HEARTBEAT_DEADLINE : val;
    self->deadline = SEC(val);
    printHeartbeatInfo(self, 0);
    return val;
}


void printHeartbeatInfo(Heartbeat *self, int unused){
    int status = self->enable;
    if(status){
        char heartbeatInfo[64];
        snprintf(heartbeatInfo, 64,
                 "[HEARTBEAT]: enabled, periodicity %d s, deadline %d s\n",
                 SEC_OF(self->periodicity), SEC_OF(self->deadline));
        SCI_WRITE(&sci0, heartbeatInfo);
    }else{
        SCI_WRITE(&sci0, "[HEARTBEAT]: disabled\n");
    }
    
}