#include <stdio.h>
#include "heartbeat.h"
#include "musicPlayer.h"
#include "toneGenerator.h"


Heartbeat heartbeatCon = initHeartbeat(CONDUCTOR, heartbeat_conductor);
Heartbeat heartbeatMus = initHeartbeat(MUSICIAN, heartbeat_musician);


void heartbeat_conductor(Heartbeat *self, int unused){
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
    SEND(self->periodicity, self->deadline, self, heartbeat_conductor, unused);
}


void heartbeat_musician(Heartbeat *self, int unused){
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
    SEND(self->periodicity, self->deadline, self, heartbeat_musician, unused);
}


void enable_heartbeat(Heartbeat *self, int unused){
    self->enable = 1;
    self->heartbeatFunc(self, 0);
}


void disable_heartbeat(Heartbeat *self, int unused){
    self->enable = 0;
}


int toggle_heartbeat(Heartbeat *self, int unused){
    if(!self->enable)
        enable_heartbeat(self, unused);
    else
        disable_heartbeat(self, unused);
    print_heartbeat_info(self, unused);
    return self->enable;
}


int set_heartbeat_period(Heartbeat *self, int val){
    val = val < self->deadline ? self->deadline : val;
    val = val > MAX_HEARTBEAT_PERIOD ? MAX_HEARTBEAT_PERIOD : val;
    self->periodicity = SEC(val);
    print_heartbeat_info(self, 0);
    return val;
}


int set_heartbeat_deadline(Heartbeat *self, int val){
    val = val < 0 ? 0 : val;
    val = val > MAX_HEARTBEAT_DEADLINE ? MAX_HEARTBEAT_DEADLINE : val;
    self->deadline = SEC(val);
    print_heartbeat_info(self, 0);
    return val;
}


void print_heartbeat_info(Heartbeat *self, int unused){
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