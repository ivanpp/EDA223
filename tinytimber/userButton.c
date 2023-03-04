#include "userButton.h"
#include "systemPorts.h"
#include <stdio.h>

// bind to sci interrupt
void reactUserButton(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    char releasedInfo [128] = {};
    char pressedInfo [32] = {};
    // PRESS_MOMENTARY mode
    if (self->mode == PRESS_MOMENTARY){
        if (currentStatus == PRESSED){
            T_RESET(&self->timer);
            snprintf(pressedInfo, 32, "Button Pressed\n");
            SCI_WRITE(&sci0, pressedInfo);
            SIO_TRIG(&sio0, 1);
        } else {           //RELEASED
            int duration_sec, duration_msec;
            duration_sec = SEC_OF(T_SAMPLE(&self->timer));
            duration_msec = MSEC_OF(T_SAMPLE(&self->timer));
            snprintf(releasedInfo, 64,
                "Button Released: duration: %d s %d ms\n", duration_sec, duration_msec);
            SCI_WRITE(&sci0, releasedInfo);
            SIO_TRIG(&sio0, 0);
            // PRESS_MOMENTARY -> PRESS_AND_HOLD
            if (duration_sec >= 1){
                self->mode = PRESS_AND_HOLD;
                SCI_WRITE(&sci0, "the fact (I'm funny)\n");
                SCI_WRITE(&sci0, "PRESS_AND_HOLD MODE\n");
                return;
            }
        }
    }
    // PRESS_AND_HOLD mode, you should speak less in this mode
    if (self->mode == PRESS_AND_HOLD){
        
        if (currentStatus == PRESSED){
            T_RESET(&self->timer);
            SIO_TRIG(&sio0, 1);
        } else {           //RELEASED
            int duration_sec, duration_msec;
            duration_sec = SEC_OF(T_SAMPLE(&self->timer));
            duration_msec = MSEC_OF(T_SAMPLE(&self->timer));
            int difficulty = 10;
            int diff = duration_msec - 193;
            snprintf(releasedInfo, 128,
                "\nNow you're STUCKED in PRESS_AND_HOLD MODE\n"
                "Hit BUTTON, get close to 193 ms (± %d ms) to QUIT\n"
                "time: %d ms, diff: %d ms\n",
                difficulty, duration_msec, diff);
            SCI_WRITE(&sci0, releasedInfo);
            SIO_TRIG(&sio0, 0);
            diff = diff > 0 ? diff : -diff; // abs
            if (diff <= difficulty){
                self->mode = PRESS_MOMENTARY;
                SCI_WRITE(&sci0, "\nQuIt ElEgEnTlY\n");
                return;
            }
            if (duration_sec >= 3){
                self->mode = PRESS_MOMENTARY;
                SCI_WRITE(&sci0, "\nqUiT aNgRiLy\n");
                return;
            }
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