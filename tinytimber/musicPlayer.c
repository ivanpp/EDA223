#include "musicPlayer.h"
#include "toneGenerator.h"


// key related
int setKey(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


/* tempo related */
// set tempo (bpm)
int setTempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatlen = 60 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatlen = beatlen;
    return tempo;
}

// set beatlength (ms)
void setBeatLength(MusicPlayer *self, int beatlen){
    int beatlen_max, beatlen_min;
    beatlen_max = 60000 / TEMPO_MIN;
    beatlen_min = 60000 / TEMPO_MAX;
    beatlen = beatlen < beatlen_min ? beatlen_min : beatlen;
    beatlen = beatlen > beatlen_max ? beatlen_max : beatlen;
    self->beatlen = beatlen;
    self->tempo = 60 * 1000 / beatlen;
}


void playMusic(MusicPlayer *self, int unused){
    // get next tone info
    int period, tempo, beatlen;
    period = self->periods[self->index];
    tempo = self->tempos[self->index];
    beatlen = self->beatlen;
    // set tone generator
    SYNC(&toneGenerator, setPeriod, period);
    // use tempo to determin play for how long

    // 50 ms blank

    // next tone
    self->index++;
    // call it self?
}