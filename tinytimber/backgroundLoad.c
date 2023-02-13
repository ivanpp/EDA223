#include "backgroundLoad.h"
#include "stdlib.h"

int loadLoop(BackgroundLoad *self, int value) 
    { 
        for(int i = 0; i<self->backgroundLoopRange; i++) {

        }
        AFTER(USEC(1300), self, loadLoop, value);
        return 1;
    }

void updateLoad(BackgroundLoad *f_self, int f_newBackgroundLoadValue)
{
    f_self->backgroundLoopRange = f_newBackgroundLoadValue;
}
