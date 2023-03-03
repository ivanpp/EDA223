#ifndef USER_BUTTON_H
#define USER_BUTTON_H

#include "TinyTimber.h"

#define RELEASED 1
#define PRESSED  0

#define BUTTON_BG_DEADLINE    USEC(800)
#define BUTTON_BG_PERIODICITY USEC(1300)

typedef enum {
    PRESS_MOMENTARY,
    PRESS_AND_HOLD,
} USER_BUTTON_MODE;

typedef struct {
    Object super;
    USER_BUTTON_MODE mode;
    int lastStatus;
    Time lastTime;
    int count;
    int history[4]; // store the history of PRESS interval (ms)
} UserButton;

#define initUserButton() \
    { initObject(), PRESS_MOMENTARY, RELEASED, 0, 0, {} }

void reactUserButton(UserButton*, int);
void buttonBackground(UserButton*, int);

#endif