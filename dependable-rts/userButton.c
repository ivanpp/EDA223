#include "userButton.h"
#include "systemPorts.h"
#include "musicPlayer.h"
#include <stdio.h>


// UserButton interruption for problem 1
void reactUserButtonP1(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    if (currentStatus == PRESSED) 
    {
        if (app.mode == CONDUCTOR) 
        {
            self->abortMessage = AFTER(SEC(2), self, resetAllfromButton, 0);
        }
        T_RESET(&self->timerPressRelease);
        SIO_TRIG(&sio0, RELEASED);
#ifdef DEBUG
        SCI_WRITE(&sci0, "[UserButton ↧]: pressed\n");
#endif
    } 
    else // release
    { 
        int duration_sec, duration_msec;
        duration_sec = SEC_OF(T_SAMPLE(&self->timerPressRelease));
        duration_msec = MSEC_OF(T_SAMPLE(&self->timerPressRelease)) + duration_sec * 1000;
        if (app.mode == CONDUCTOR && duration_msec < 1999)
        {
            ABORT(self->abortMessage);
        }
        if (app.mode == MUSICIAN)
        {
            SYNC(&musicPlayer, toggleMusic, 0);
        }
        SIO_TRIG(&sio0, PRESSED);
#ifdef DEBUG
        char releasedInfo[64];
        snprintf(releasedInfo, 64,
                "[UserButton ↥]: released, duration: %d ms\n", duration_msec);
        SCI_WRITE(&sci0, releasedInfo);
#endif
    }
}


// UserButton interruption for EDA223
void reactUserButton(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    char releasedInfo [256] = {};
    char pressedInfo [64] = {};
    // PRESS_MOMENTARY mode
    if (self->mode == PRESS_MOMENTARY){
        if (currentStatus == PRESSED){
            self->abortMessage = AFTER(SEC(1), self, checkPressAndHold, 0);
            int interval_sec, interval_msec, interval_avg;
            T_RESET(&self->timerPressRelease);
            // tempo-setting burst
            interval_sec = SEC_OF(T_SAMPLE(&self->timerLastPress));
            interval_msec = MSEC_OF(T_SAMPLE(&self->timerLastPress)) + interval_sec * 1000;
            snprintf(pressedInfo, 64, "[UserButton ↧]: pressed, interval: %d ms\n", interval_msec);
            SCI_WRITE(&sci0, pressedInfo);
            if (self->index == 0)
                self->intervals[self->index] = interval_msec; // store for the first, but not moving index
            // comparable: no interval differs from another interval by 100 ms (that is strict...)
            //             also interval should no more than 366 ms
            if ((interval_msec > 366) && 0) { // LIMITATION 1
                clearIntervalHistory(self, /*unused*/0);
            } else if ((interval_msec < 100) || 0){
                clearIntervalHistory(self, /*unused*/0);
                //SCI_WRITE(&sci0, "CLR for too large value\n");
            } else if(compareIntervalHistory(self, interval_msec) || 0){ // LIMITATION 2
                clearIntervalHistory(self, /*unused*/0);
                //SCI_WRITE(&sci0, "CLR for interval diff\n");
            }else {
                self->intervals[self->index % MAX_BURST] = interval_msec;
                self->index = self->index + 1;
                if (self->index >= MAX_BURST){
                    interval_avg = treAverage(self, /*unused*/0);
                    int tempo1 = 60 * 1000 / interval_avg;
                    int tempo = SYNC(&musicPlayer, setTempo, tempo1);
                    //int tempo = SYNC(&musicPlayer, setTempo, interval_avg);
                    snprintf(pressedInfo, 64, "[UserButton]: Tempo set to %d, (attempt: %d)\n", 
                            tempo, tempo1);
                    //snprintf(pressedInfo, 64, "[UserButton]: Tempo set to %d, (attempt: %d)\n", 
                    //        tempo, interval_avg);
                    SCI_WRITE(&sci0, pressedInfo);
                }
            }
            printoutIntervals(self, /*unused*/0);
            T_RESET(&self->timerLastPress);
            SIO_TRIG(&sio0, 1);
        } else {           //RELEASED
            int duration_sec, duration_msec;
            duration_sec = SEC_OF(T_SAMPLE(&self->timerPressRelease));
            duration_msec = MSEC_OF(T_SAMPLE(&self->timerPressRelease)) + duration_sec * 1000;
            snprintf(releasedInfo, 64,
                "[UserButton ↥]: released, duration: %d ms\n", duration_msec);
            SCI_WRITE(&sci0, releasedInfo);
            SIO_TRIG(&sio0, 0);
            // PRESS_MOMENTARY -> PRESS_AND_HOLD
            if (duration_msec > 1999){
                int tempo = SYNC(&musicPlayer, setTempo, TEMPO_DEFAULT);
                snprintf(releasedInfo, 64, "[UserButton]: Tempo reset to: %d)\n", tempo);
                SCI_WRITE(&sci0, releasedInfo);
            }
            if (duration_msec < 999) {
                ABORT(self->abortMessage);
                snprintf(releasedInfo, 64,
                "[UserButton ↥]: released, duration: %d ms\n", duration_msec);
                SCI_WRITE(&sci0, releasedInfo);
            }
        }
    }
    // PRESS_AND_HOLD mode, you should speak less in this mode
    if (self->mode == PRESS_AND_HOLD){
        if (currentStatus == PRESSED){
            T_RESET(&self->timerPressRelease);
            SIO_TRIG(&sio0, 1);
        } else {           //RELEASED
            int duration_sec, duration_msec;
            duration_sec = SEC_OF(T_SAMPLE(&self->timerPressRelease));
            duration_msec = MSEC_OF(T_SAMPLE(&self->timerPressRelease)) + duration_sec * 1000;
            int difficulty = 10;
            int diff = duration_msec - 193;
            snprintf(releasedInfo, 256,
                "\nNow you're STUCKED in PRESS_AND_HOLD MODE\n"
                "Hit BUTTON, get close to 193 ms (± %d ms) to QUIT\n"
                "duration: %d ms, diff: %d ms\n",
                difficulty, duration_msec, diff);
            SCI_WRITE(&sci0, releasedInfo);
            SIO_TRIG(&sio0, 0);
            // PRESS_AND_HOLD -> PRESS_MOMENTARY
            diff = diff > 0 ? diff : -diff; // abs
            if (diff <= difficulty){
                self->mode = PRESS_MOMENTARY;
                SCI_WRITE(&sci0, "\nQuIt ElEgEnTlY, ENTER PRESS_MOMENTARY MODE\n");
                return;
            }
            if (duration_msec > 1999){
                self->mode = PRESS_MOMENTARY;
                SCI_WRITE(&sci0, "\nqUiT aNgRiLy, ENTER PRESS_MOMENTARY MODE\n");
                return;
            }
        }
    }
}

/* UserButton utils */
void clearIntervalHistory(UserButton *self, int unused){
    for (int i = 0; i < MAX_BURST; i++){
        self->intervals[i] = 0;
    }
    self->index = 0;
}


void checkPressAndHold(UserButton *self, int unused){
    self->mode = PRESS_AND_HOLD;
    SCI_WRITE(&sci0, "One second passed.\n entered PRESS_AND_HOLD MODE\n");
    return;
}


void resetAllfromButton(UserButton *self, int unused){
    SYNC(&musicPlayer, resetAll, 0);
    SCI_WRITE(&sci0, "[UserButton]: 2 s passed, reset key and tempo.\n");
    return;
}


// compare the interval with intervals already stored in self->intervals[]
// return 1 if difference is greater than tolerance
int compareIntervalHistory(UserButton *self, int interval){
    int tolerance = 100; // max diff 100 ms
    int diff, max_check;
    max_check = self->index > MAX_BURST ? MAX_BURST : self->index;
    for(int i = 0; i < max_check; i++){
        diff = interval - self->intervals[i];
        diff = diff > 0 ? diff : -diff;
        if (diff > tolerance)
            return 1;
    }
    return 0;
}


int treAverage(UserButton *self, int unused){
    int average = (self->intervals[0] + self->intervals[1] + self->intervals[2]) / 3;
    return average;
}


void printoutIntervals(UserButton *self, int unused){
    char intervalsInfo [32] = {};
    snprintf(intervalsInfo, 32, "[%d, %d, %d], index: %d\n",
            self->intervals[0], self->intervals[1], self->intervals[2], self->index);
    SCI_WRITE(&sci0, intervalsInfo);
}