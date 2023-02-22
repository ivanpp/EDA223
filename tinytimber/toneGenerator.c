#include "toneGenerator.h"
#include <stdlib.h>

void playTone(ToneGenerator *self, int unused) { 
    int volatile * const p_reg = (int *) 0x4000741C;
    /* sound */
    if (self->isMuted) { //Have we muted sound?
        *p_reg = 0;
        self->isPlaying = 0;
    } else if (self->isPlaying) { //Are we playing
        *p_reg = (((self->volume)<(SAFE_VOLUME)) ? (self->volume) : (SAFE_VOLUME));
        self->isPlaying = 0;
    } else { //Are we not playing
        *p_reg = 0x0;
        self->isPlaying = 1;
    }
    int tone = genTone(self, self->toneFreq);
    /* deadline */
    if(true == self->isDeadlineEnabled) self->toneGenDeadline = TONE_GEN_DEADLINE;
    else self->toneGenDeadline = 0;
    /* periodic call, wi/wo ddl*/
    SEND(USEC(tone), USEC(self->toneGenDeadline), self, playTone, unused);
}

int genTone(ToneGenerator *self, int frequency) {
    return 1000000 / (2 * frequency);
}

int setFrequency(ToneGenerator *self, int frequency) {
    if (frequency > MAX_FREQ) {
        self->toneFreq = MAX_FREQ;
    } else if (frequency < 1) {
        self->toneFreq = 1;
    } else {
        self->toneFreq = frequency;
    }
    return self->toneFreq;
}

int setVolume(ToneGenerator *self, int volume) {
    if (volume > SAFE_VOLUME) {
        self->volume = SAFE_VOLUME;
    } else if (volume < 1) {
        self->volume = 1;
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

int toggleAudio(ToneGenerator *self, int unused) {
    self ->isMuted = self->isMuted ? 0 : 1;
    return self->volume;
}

int toggleDeadlineTG(ToneGenerator *self, int unused) {
    self->isDeadlineEnabled = !self->isDeadlineEnabled;
    return self->isDeadlineEnabled;
}