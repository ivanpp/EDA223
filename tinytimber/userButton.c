#include "userButton.h"
#include "systemPorts.h"
#include <stdio.h>

// bind to sci interrupt
void reactUserButton(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    char releasedInfo [64] = {};
    char pressedInfo [32] = {};
    // default behavior: trigger call-back when released->pressed
    if (currentStatus == PRESSED){
        T_RESET(&self->timer);
        snprintf(pressedInfo, 32, "Button Pressed\n");
        SCI_WRITE(&sci0, pressedInfo);
        SIO_TRIG(&sio0, 1);
    }
    if (currentStatus == RELEASED){
        int duration_sec, duration_msec;
        duration_sec = SEC_OF(T_SAMPLE(&self->timer));
        duration_msec = MSEC_OF(T_SAMPLE(&self->timer));
        snprintf(releasedInfo, 64,
            "Button Released: duration: %d s %d ms\n", duration_sec, duration_msec);
        SCI_WRITE(&sci0, releasedInfo);
        SIO_TRIG(&sio0, 0);
        if (duration_sec >= 1){
            // change mode
        }
    }
}


// DEPRECATED
// 'background load' for button
void buttonBackground(UserButton *self, int unused){
    // sample the current state
    int currentStatus = SIO_READ(&sio0);
    if (self->mode == PRESS_MOMENTARY){
        // pressed -> released
        if (self->lastStatus == PRESSED && currentStatus == RELEASED){
            char releasedInfo [64] = {};
            self->lastStatus = currentStatus;
            // sample the time
            Time duration = T_SAMPLE(&self->timer);
            snprintf(releasedInfo, 64, "Button Released, duration: %d ms\n", MSEC_OF(duration));
            SCI_WRITE(&sci0, releasedInfo);
            // > 1s change mode
        }
        // released -> pressed
        if (self->lastStatus == RELEASED && currentStatus == PRESSED){
            //char pressedInfor [32] = {};
            self->lastStatus = currentStatus;
            T_RESET(&self->timer);
            SCI_WRITE(&sci0, "Button Pressed\n");
        }
    }
    if (self->mode == PRESS_AND_HOLD){

    }
    SEND(BUTTON_BG_PERIODICITY, BUTTON_BG_DEADLINE, self, buttonBackground, unused);
}