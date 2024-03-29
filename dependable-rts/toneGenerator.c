#include "toneGenerator.h"
#include <stdlib.h>
#include "systemPorts.h"

ToneGenerator toneGenerator = initToneGenerator();

void play_tone(ToneGenerator *self, int unused) { 
    int volatile * const p_reg = (int *) 0x4000741C;
    if (self->isStop)
        return;
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
    SEND(USEC(self->period), USEC(self->toneGenDeadline), self, play_tone, unused);
}


/* freq, period */
// set freq and corresponding period
int set_frequency(ToneGenerator *self, int frequency) {
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

// set period and corresponding freq
int set_period(ToneGenerator *self, int period) {
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


/* volume [0, SAFE_VOLUME] */
int set_volume(ToneGenerator *self, int volume) {
    if (volume > SAFE_VOLUME) {
        self->volume = SAFE_VOLUME;
    } else if (volume < 1) {
        self->volume = 1;
    } else {
        self->volume = volume;
    }
    return self->volume;
}

int adjust_volume(ToneGenerator *self, int volume) {
    int adjusted = self->volume + volume;
    int max = (((adjusted)>(0)) ? (adjusted) : (0));
    int min = (((max)<(SAFE_VOLUME)) ? (max) : (SAFE_VOLUME));
    self->volume = min;
    print_volume_info(self, 0);
    return self->volume;
}

int toggle_audio(ToneGenerator *self, int unused) {
    self ->isMuted = self->isMuted ? 0 : 1;
    print_volume_info(self, 0);
    return self->volume;
}

int muteAudio(ToneGenerator *self, int unused) {
    self->isMuted = 1;
    return self->isMuted;
}

int unmuteAudio(ToneGenerator *self, int unused) {
    self->isMuted = 0;
    return self->isMuted;
}

/* gap of silience */
// small gap of silience between note, for better sound quality
void blank_tone(ToneGenerator *self, int unused) {
    self ->isBlank = 1;
}

void unblank_tone(ToneGenerator *self, int unused) {
    self ->isBlank = 0;
}


/* stop/start ToneGenerator from other object */
void start_toneGen(ToneGenerator *self, int unused) {
    self->isStop = 0;
}

void stop_toneGen(ToneGenerator *self, int unused) {
    self->isStop = 1;
}


/* deadline */
int toggle_deadline_toneGen(ToneGenerator *self, int unused) {
    self->isDeadlineEnabled = !self->isDeadlineEnabled;
    return self->isDeadlineEnabled;
}


/* Information */
void print_volume_info(ToneGenerator *self, int unused){
    int muteStatus, volume, volPercentage;
    muteStatus = self->isMuted;
    volume = self->volume;
    volPercentage = (10 * volume) / SAFE_VOLUME;
    if(muteStatus){
        SCI_WRITE(&sci0, "vol: <x  muted\n");
    }else{
        char volInfo[32] = {};
        snprintf(volInfo, 32, "vol: <)  [-----------]%d/%d\n", volume, SAFE_VOLUME);
        volInfo[volPercentage+10] = '|';
        SCI_WRITE(&sci0, volInfo);
    }
}