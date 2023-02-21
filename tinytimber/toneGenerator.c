#include "toneGenerator.h"
#include "stdlib.h"

int playTone(ToneGenerator *self, int value) 
    { 
        int volatile * const p_reg = (int *) 0x4000741C;
        if (self->isMuted) //Have we muted sound?
        {
            *p_reg = 0;
            self->isPlaying = 0;
        } 
        else if (self->isPlaying) //Are we playing
        {        
            *p_reg = (((self->volume)<(SAFE_VOLUME)) ? (self->volume) : (SAFE_VOLUME));
            self->isPlaying = 0;
        } else //Are we not playing
        {
            *p_reg = 0x0;
            self->isPlaying = 1;
        }
        int tone = genTone(self, self->toneFreq);

        /* update deadline based on isDeadlineEnabled flag */
        if(true == self->isDeadlineEnabled)
        {
            // part2_task3: update deadline for playing
            self->toneGenDeadline = TONE_GEN_DEADLINE;
        }
        else
        {
            // default value
            self->toneGenDeadline = 0;//tone/2;
        }

        SEND(USEC(tone), USEC(self->toneGenDeadline), self, playTone, value);
        return value;
    }

int genTone(ToneGenerator *self, int frequency) {
    return 1000000 / (2 * frequency);
}

int setFrequency(ToneGenerator *self, int frequency) {
    if (frequency > 4000) {
        self->toneFreq = 4000;
    } else if (frequency < 0) {
        self->toneFreq = 0;
    } else {
        self->toneFreq = frequency;
    }
    return self->toneFreq;
}

int setVolume(ToneGenerator *self, int volume) {
    if (volume > SAFE_VOLUME) {
        self->volume = SAFE_VOLUME;
    } else if (volume < 0) {
        self->volume = 0;
    } else {
        self->volume = volume;
    }
    return self->volume;
}

int adjustVolume(ToneGenerator *self, int volume) {
    int adjusted = self->volume + volume;
    int max = (((adjusted)>(0)) ? (adjusted) : (0));
    int min = (((max)<(SAFE_VOLUME)) ? (max) : (SAFE_VOLUME));
    self->volume = min;
    return self->volume;
}

int toggleAudio(ToneGenerator *self, int value) {

    self ->isMuted = self->isMuted ? 0 : 1;

    return self->volume;
}
