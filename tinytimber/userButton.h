#ifndef USER_BUTTON_H
#define USER_BUTTON_H

#include "TinyTimber.h"
//#include "application.h"

#define RELEASED 1
#define PRESSED  0

typedef struct {
    Object super;
    int count;
    int history[4]; // store the history of PRESS interval (ms)
} UserButton;

#define initUserButton() \
    { initObject(), 0, {} }

void reactUserButton(UserButton*, int);

#endif