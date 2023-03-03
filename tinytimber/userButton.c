#include "userButton.h"
#include "systemPorts.h"

// bind to sci interrupt
void reactUserButton(UserButton *self, int unused){
    SCI_WRITE(&sci0, "Invoke from UserButton Object\n");
}

// 'background load' for button
void buttonBackground(UserButton *self, int unused){
    // sample the current state
    int currentStatus = SIO_READ(&sio0);
    if (self->mode == PRESS_MOMENTARY){
        // pressed -> released
        if (self->lastStatus == PRESSED && currentStatus == RELEASED){
            SCI_WRITE(&sci0, "Button Released\n");
            self->lastStatus = currentStatus;
            // > 1s change mode
        }
        // released -> pressed
        if (self->lastStatus == RELEASED && currentStatus == PRESSED){
            SCI_WRITE(&sci0, "Button Pressed\n");
            self->lastStatus = currentStatus;
        }
    }
    if (self->mode == PRESS_AND_HOLD){

    }
    SEND(BUTTON_BG_PERIODICITY, BUTTON_BG_DEADLINE, self, buttonBackground, unused);
}