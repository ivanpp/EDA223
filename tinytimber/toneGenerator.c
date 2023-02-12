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
        int tone = genTone(self, self->toneFreq);

        SEND(USEC(tone), USEC(tone/2), self, playTone, value);
        SEND(USEC(500), USEC(250), self, playTone, value);
        return value;
    }

int genTone(ToneGenerator *self, int frequency) {
    return 1000000 / (2 * frequency);
}