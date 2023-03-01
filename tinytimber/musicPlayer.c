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

// key related
int setKey(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


/* tempo related */
// set tempo (bpm) for 'a'
int setTempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatMult = 60 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatMult = beatMult;
    return tempo;
}

void playMusic(MusicPlayer *self, int unused){
    // get next tone info
    int period, beatLen;
    period = pianoPeriods[brotherJohn[self->index]+15]; //Get period
    beatLen = self->beatMult * tempos[self->index]; // Get beatLen
    // set tone generator
    ASYNC(&toneGenerator, toggleAudio, 0);
    AFTER(MSEC(50), &toneGenerator, toggleAudio, 0);
    ASYNC(&toneGenerator, setPeriod, period);
    // next tone
    self->index++;
    self->index = self->index%32;
    // call it self?
    AFTER(MSEC(beatLen), self, playMusic, 0);
}