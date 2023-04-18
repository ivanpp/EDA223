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

void reactUserButton(UserButton*, int);
void reactUserButtonP1(UserButton*, int);
void resetAllfromButton(UserButton*, int);
void clearIntervalHistory(UserButton*, int);
int compareIntervalHistory(UserButton*, int);
int treAverage(UserButton*, int);
void printoutIntervals(UserButton *, int);
void checkPressAndHold(UserButton *self, int unused);

void buttonBackground(UserButton*, int);

#endif