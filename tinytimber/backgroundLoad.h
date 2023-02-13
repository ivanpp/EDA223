#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 5
#define LOAD_STEP 500
typedef struct 
{
    Object super;
    int backgroundLoopRange;
} BackgroundLoad;

#define initBackgroundLoad() \
    { initObject(), 1000}


int loadLoop(BackgroundLoad *, int);

///@brief   Function to update 'backgroundLoopRange' member variable of BackgroundLoad
///@param[in]   f_self: Pointer to BackgroundLoad object for Tinytimber
///@param[in]   f_newBackgroundLoadValue: New value for backgroundLoopRange
void updateLoad(BackgroundLoad *f_self, int f_newBackgroundLoadValue);
