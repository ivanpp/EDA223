#include "backgroundLoad.h"
#include <stdlib.h>

void loadLoop(BackgroundLoad *self, int unused) {
    for(int i = 0; i<self->backgroundLoopRange; i++) {

    }
    /* deadline */
    if(true == self->isDeadlineEnabled) self->bgLoadDeadline = BGLOAD_DEADLINE;
    else self->bgLoadDeadline = 0;
    /* periodic call, wi/wo ddl*/
    SEND(BGLOAD_PERIODICITY, self->bgLoadDeadline, self, loadLoop, unused);
}

// adjust load
int adjustLoad(BackgroundLoad *self, int diff) {
    int value = self->backgroundLoopRange + diff;
    value = value > MAX_LOOP_RANGE ? MAX_LOOP_RANGE : value;
    value = value < MIN_LOOP_RANGE ? MIN_LOOP_RANGE : value;
    self->backgroundLoopRange = value;
    return value;
}

// set load, ignore the MIN MAX bound
int setLoad(BackgroundLoad *self, int value) {
    self->backgroundLoopRange = value;
    return value;
}

int toggleDeadlineBG(BackgroundLoad *self, int unused) {
    self->isDeadlineEnabled = !self->isDeadlineEnabled;
    return self->isDeadlineEnabled;
}