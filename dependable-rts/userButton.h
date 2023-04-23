#ifndef USER_BUTTON_H
#define USER_BUTTON_H

#include "TinyTimber.h"

#define RELEASED 1
#define PRESSED  0
#define MAX_BURST 3

#define BUTTON_BG_DEADLINE    USEC(800)
#define BUTTON_BG_PERIODICITY USEC(1300)

typedef enum {
    PRESS_MOMENTARY,
    PRESS_AND_HOLD,
} USER_BUTTON_MODE;

typedef struct {
    Object super;
    USER_BUTTON_MODE mode;
    int lastStatus; // not needed if using interrupt
    Timer timerPressRelease;
    Timer timerLastPress;
    int index;
    int intervals[MAX_BURST]; // store the history of PRESS interval (ms)
    Msg abortMessage; 
} UserButton;

#define initUserButton() \
    { initObject(), PRESS_MOMENTARY, RELEASED, initTimer(), initTimer(), 0, {},  }


void react_userButton_P1(UserButton*, int);
void react_userButton_P2(UserButton*, int);
void react_userButton(UserButton*, int);
void reset_all_from_button(UserButton*, int);
void claim_confrom_button(UserButton *, int);
void clear_interval_history(UserButton*, int);
int compare_interval_history(UserButton*, int);
int tre_average(UserButton*, int);
void printout_intervals(UserButton *, int);
void checkPressAndHold(UserButton *self, int unused);


#endif