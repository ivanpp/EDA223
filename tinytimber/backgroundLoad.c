#include "backgroundLoad.h"
#include <stdlib.h>

int loadLoop(BackgroundLoad *self, int value) { 
    for(int i = 0; i<self->backgroundLoopRange; i++) {

    }
    /* deadline */
    if(true == self->isDeadlineEnabled) self->bgLoadDeadline = BGLOAD_DEADLINE;
    else self->bgLoadDeadline = 0;
    
    SEND(USEC(1300),self->bgLoadDeadline, self, loadLoop, value);
    return 1;
}

int adjustLoad(BackgroundLoad *self, int value){
    self->backgroundLoopRange += value;
    return self->backgroundLoopRange;
}

void updateLoad(BackgroundLoad *f_self, int f_newBackgroundLoadValue)
{
    f_self->backgroundLoopRange = f_newBackgroundLoadValue;
}