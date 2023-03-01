#include "musicPlayer.h"
#include "toneGenerator.h"

const int pianoPeriods[32] = {2702, 2551, 2407, 2272, 2145, 2024, 1911, 1803, /*-8*/
						  1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136 /*0*/, 
						  1072, 1012, 955, 901, 851, 803, 758, 715 /*8*/, 
						  675, 637, 601, 568, 536, 506, 477, 450 /*16*/};


const int brotherJohn[32] = {0, 2, 4, 0,
                        0, 2, 4, 0,
                        4, 5, 7,
                        4, 5, 7,
                        7, 9, 7, 5, 4, 0,
                        7, 9, 7, 5, 4, 0,
                        0, -5, 0,
                        0, -5, 0};

const int tempos[32] = {2, 2, 2, 2, 
                 2, 2, 2, 2,
				 2, 2, 4, 
                 2, 2, 4,
                 1, 1, 1, 1, 2, 2,
                 1, 1, 1, 1, 2, 2, 
                 2, 2, 4, 
                 2, 2, 4}; 

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
int pauseMusic(MusicPlayer *self, int unused){
    // if music is stop
    if (self->isStop) {
        self->isStop = 0;
        SYNC(&toneGenerator, startToneGen, 0);
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    } else {
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
    }
    return self->isStop;
}

// stop/restart
int stopMusic(MusicPlayer *self, int unused){
    // if music is stop
    if (self->isStop) {
         // reset the index, start from the begining
        self->isStop = 0;
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

void playMusic(MusicPlayer *self, int unused){
    // get next tone info
    if (self->isStop)
        return;
    int period, beatLen;
    period = pianoPeriods[brotherJohn[self->index] + self->key - PERIODS_IDX_DIFF]; //Get period
    beatLen = self->beatMult * tempos[self->index]; // Get beatLen
    // set tone generator
    ASYNC(&toneGenerator, mute, 0);
    ASYNC(&toneGenerator, setPeriod, period);
    AFTER(MSEC(50), &toneGenerator, unmute, 0);

    // next tone
    self->index++;
    self->index = self->index%32;
    // call it self?
    AFTER(MSEC(beatLen), self, playMusic, 0);
}