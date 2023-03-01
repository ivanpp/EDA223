#include "toneGenerator.h"
#include <stdlib.h>

ToneGenerator toneGenerator = initToneGenerator();


void playTone(ToneGenerator *self, int unused) { 
    int volatile * const p_reg = (int *) 0x4000741C;
    /* sound */
    if (self->isMuted || self->isBlank) { //Have we muted sound?
        *p_reg = 0;
        self->isPlaying = 0;
    } else if (self->isPlaying) { //Are we playing
        *p_reg = (((self->volume)<(SAFE_VOLUME)) ? (self->volume) : (SAFE_VOLUME));
        self->isPlaying = 0;
    } else { //Are we not playing
        *p_reg = 0x0;
        self->isPlaying = 1;
    }
    /* deadline */
    if(true == self->isDeadlineEnabled) self->toneGenDeadline = TONE_GEN_DEADLINE;
    else self->toneGenDeadline = 0;
    /* periodic call, wi/wo ddl*/
    SEND(USEC(self->period), USEC(self->toneGenDeadline), self, playTone, unused);
}

int setFrequency(ToneGenerator *self, int frequency) {
    // set freq
    if (frequency > MAX_FREQ) {
        self->toneFreq = MAX_FREQ;
    } else if (frequency < MIN_FREQ) {
        self->toneFreq = MIN_FREQ;
    } else {
        self->toneFreq = frequency;
    }
    // set period for that freq
    self->period = 1000000 / (2 * self->toneFreq);
    return self->toneFreq;
}

int setPeriod(ToneGenerator *self, int period) {
    // set period 
    int max_period, min_period;
    max_period = 5000000 / MIN_FREQ;
    min_period = 5000000 / MAX_FREQ;
    if (period > max_period){
        period = max_period;
    } else if (period < min_period){
        period = min_period;
    }
    self->period = period;
    // set freq for that period, return the freq(readable)
    self->toneFreq = 5000000 / period;
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

void mute(ToneGenerator *self, int unused) {
    self ->isBlank = 1;
}

void unmute(ToneGenerator *self, int unused) {
    self ->isBlank = 0;
}

int toggleDeadlineTG(ToneGenerator *self, int unused) {
    self->isDeadlineEnabled = !self->isDeadlineEnabled;
    return self->isDeadlineEnabled;
}