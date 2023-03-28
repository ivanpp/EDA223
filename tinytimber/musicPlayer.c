#include "musicPlayer.h"
#include "toneGenerator.h"
#include "systemPorts.h"
#include "brotherJohn.h"

MusicPlayer musicPlayer = initMusicPlayer();


// set Key
int setKey(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


// set tempo (bpm) for 'a'
int setTempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatMult = 30 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatMult = beatMult;
    return tempo;
}


// pause/unpause
int musicPauseUnpause(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, startToneGen, 0);
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    } else {
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
    }
    return self->isStop;
}

int musicUnpause(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, startToneGen, 0);
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    }
    return self->isStop;
}

int musicPause(MusicPlayer *self, int unused){
    if (!self->hardStopped) {
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
    }
    return self->isStop;
}


// stop/restart
int musicStopStart(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
         // reset the index, start from the begining
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, startToneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    } else{
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}

int musicStart(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, startToneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    }
    return self->isStop;
}

int musicStop(MusicPlayer *self, int unused){
    if (!self->hardStopped){
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}


// scheduler
void playMusic(MusicPlayer *self, int unused){
    // get next tone info
    if (self->isStop) {
        self->hardStopped = 1;
        return;
    }
    int period, tempo, beatLen;
    period = pianoPeriods[brotherJohn[self->index] + self->key - PERIODS_IDX_DIFF]; //Get period
    tempo = tempos[self->index]; // val can be 2, 4, 1
    beatLen = self->beatMult * tempo; // Get beatLen
    // LED: lit at the begining, unlit in the middle of a beat
    // since we stores int 2, 4, 1 for 1-beat, 2-beat, half-beat
    // we can use it to schedule LED too
    // first, start with the STUPID way
    switch(tempo){
        case(1):
            AFTER(MSEC(0), &sio0, sio_toggle, 0);
            break;
        case(2):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(beatLen/2), &sio0, sio_toggle, 0);
            break;
        case(4):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(1*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(2*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(3*beatLen/4), &sio0, sio_toggle, 0);
            break;
    }
    // set tone generator
    ASYNC(&toneGenerator, blankTone, 0);
    ASYNC(&toneGenerator, setPeriod, period);
    AFTER(MSEC(50), &toneGenerator, unblankTone, 0);
    // next tone
    self->index++;
    self->index = self->index%32;
    // call it self?
    AFTER(MSEC(beatLen), self, playMusic, 0);
}