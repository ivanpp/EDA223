#include "userButton.h"
#include "systemPorts.h"
#include "musicPlayer.h"
#include <stdio.h>


// UserButton interruption for problem 1
void react_userButton_P1(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    if (currentStatus == PRESSED) 
    {
        if (app.mode == CONDUCTOR) 
        {
            self->abortMessage = AFTER(SEC(2), self, reset_all_from_button, 0);
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
            SYNC(&musicPlayer, toggle_music, 0);
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


void react_userButton_P2(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    if (currentStatus == PRESSED) 
    {
#ifdef DEBUG
        SCI_WRITE(&sci0, "[UserButton ↧]: pressed\n");
#endif
        T_RESET(&self->timerPressRelease);
        SIO_TRIG(&sio0, RELEASED);
        if (app.mode == MUSICIAN)
            self->abortMessage = AFTER(SEC(5), self, claim_confrom_button, 0);
    }
    else // release
    {
        int duration_sec, duration_msec;
        duration_sec = SEC_OF(T_SAMPLE(&self->timerPressRelease));
        duration_msec = MSEC_OF(T_SAMPLE(&self->timerPressRelease)) + duration_sec * 1000;
        SIO_TRIG(&sio0, PRESSED);
#ifdef DEBUG
        char releasedInfo[64];
        snprintf(releasedInfo, 64,
                "[UserButton ↥]: released, duration: %d ms\n", duration_msec);
        SCI_WRITE(&sci0, releasedInfo);
#endif
        if (app.mode == MUSICIAN && duration_msec < 5000)
            ABORT(self->abortMessage);
    }
}


void react_userButton_P3(UserButton *self, int unused){
    int currentStatus = SIO_READ(&sio0);
    if (currentStatus == PRESSED) 
    {
        if (app.mode == MUSICIAN) 
        {
            self->abortMessage = AFTER(SEC(2), self, toggleSilentFailure, 0);
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
        if (app.mode == MUSICIAN && duration_msec < 1999)
        {
            ABORT(self->abortMessage);
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
void react_userButton(UserButton *self, int unused){
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
                clear_interval_history(self, /*unused*/0);
            } else if ((interval_msec < 100) || 0){
                clear_interval_history(self, /*unused*/0);
                //SCI_WRITE(&sci0, "CLR for too large value\n");
            } else if(compare_interval_history(self, interval_msec) || 0){ // LIMITATION 2
                clear_interval_history(self, /*unused*/0);
                //SCI_WRITE(&sci0, "CLR for interval diff\n");
            }else {
                self->intervals[self->index % MAX_BURST] = interval_msec;
                self->index = self->index + 1;
                if (self->index >= MAX_BURST){
                    interval_avg = tre_average(self, /*unused*/0);
                    int tempo1 = 60 * 1000 / interval_avg;
                    int tempo = SYNC(&musicPlayer, set_tempo, tempo1);
                    //int tempo = SYNC(&musicPlayer, set_tempo, interval_avg);
                    snprintf(pressedInfo, 64, "[UserButton]: Tempo set to %d, (attempt: %d)\n", 
                            tempo, tempo1);
                    //snprintf(pressedInfo, 64, "[UserButton]: Tempo set to %d, (attempt: %d)\n", 
                    //        tempo, interval_avg);
                    SCI_WRITE(&sci0, pressedInfo);
                }
            }
            printout_intervals(self, /*unused*/0);
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
                int tempo = SYNC(&musicPlayer, set_tempo, TEMPO_DEFAULT);
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
void clear_interval_history(UserButton *self, int unused){
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

void toggleSilentFailure(UserButton *self, int unused){
    self->mode = PRESS_AND_HOLD;
    SCI_WRITE(&sci0, "Two second passed.\n entered PRESS_AND_HOLD MODE and request silent failure\n");
    
    return;
}


// problem 1: @CONDUCTOR
void reset_all_from_button(UserButton *self, int unused){
    SYNC(&musicPlayer, reset_all, 0);
    SCI_WRITE(&sci0, "[UserButton]: 2 s passed, reset key and tempo.\n");
    return;
}


// problem 2: @MUSICIAN
void claim_confrom_button(UserButton *self, int unused){
    SYNC(&network, claim_conductorship, 0);
    SCI_WRITE(&sci0, "[Userbutton]: 5 s passed, claim conductorship\n");
}


// compare the interval with intervals already stored in self->intervals[]
// return 1 if difference is greater than tolerance
int compare_interval_history(UserButton *self, int interval){
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


int tre_average(UserButton *self, int unused){
    int average = (self->intervals[0] + self->intervals[1] + self->intervals[2]) / 3;
    return average;
}


void printout_intervals(UserButton *self, int unused){
    char intervalsInfo [32] = {};
    snprintf(intervalsInfo, 32, "[%d, %d, %d], index: %d\n",
            self->intervals[0], self->intervals[1], self->intervals[2], self->index);
    SCI_WRITE(&sci0, intervalsInfo);
}