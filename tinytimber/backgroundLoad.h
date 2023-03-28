#ifndef BGLOAD_H
#define BGLOAD_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define LOAD_STEP 500
#define MIN_LOOP_RANGE 0
#define MAX_LOOP_RANGE 30000

#define BGLOAD_DEADLINE    USEC(800)
#define BGLOAD_PERIODICITY USEC(1300)

typedef struct 
{
    Object super;
    int backgroundLoopRange;
    bool isDeadlineEnabled;
    int bgLoadDeadline;
} BackgroundLoad;

#define initBackgroundLoad() \
    { initObject(), 0, false, 0 }


void loadLoop(BackgroundLoad *, int);
int adjustLoad(BackgroundLoad *, int);
int setLoad(BackgroundLoad *, int);
int toggleDeadlineBG(BackgroundLoad *, int);

#endif