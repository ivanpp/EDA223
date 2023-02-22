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
///@brief   define background Load deadline 
#define BGLOAD_DEADLINE    USEC(800)
#define BGLOAD_PERIODICITY USEC(1300)

typedef struct 
{
    Object super;
    int backgroundLoopRange;

    /// @brief  member variable to check if deadline is enabled or disabled : part1_task3
    bool isDeadlineEnabled;

    /// @brief  bgLoad specific deadline
    int bgLoadDeadline;
} BackgroundLoad;

#define initBackgroundLoad() \
    { initObject(), 1000, false, 0 }


void loadLoop(BackgroundLoad *, int);
int adjustLoad(BackgroundLoad *, int);
int toggleDeadlineBG(BackgroundLoad *, int);


///@brief   Function to update 'backgroundLoopRange' member variable of BackgroundLoad
///@param[in]   f_self: Pointer to BackgroundLoad object for Tinytimber
///@param[in]   f_newBackgroundLoadValue: New value for backgroundLoopRange
void updateLoad(BackgroundLoad *f_self, int f_newBackgroundLoadValue);

#endif