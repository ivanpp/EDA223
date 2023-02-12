#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 5

typedef struct 
{
    Object super;
    int backgroundLoopRange;
} BackgroundLoad;

#define initBackgroundLoad() \
    { initObject(), 100}


int loadLoop(BackgroundLoad *, int);
