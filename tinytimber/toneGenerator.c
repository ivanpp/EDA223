#include "toneGenerator.h"


int playTone(ToneGenerator *self, int value) 
    { 
        int volatile * const p_reg = (int *) 0x4000741C;
        if (self->isPlaying) {        
            *p_reg = 0x9;
            self->isPlaying = 0;
            
        } else {
            *p_reg = 0x0;
            self->isPlaying = 1;
        }


        SEND(USEC(500), USEC(250), self, playTone, value);
        return value;
    }
