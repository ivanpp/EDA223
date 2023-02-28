#include "musicPlayer.h"
#include "toneGenerator.h"


// key related
int setKey(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


// tempo related
int setTempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatlen = 60 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatlen = beatlen;
    return tempo;
}


void setBeatLength(MusicPlayer *self, int beatlen){
    int tempo = 60 * 1000 / beatlen;
    self->tempo   = tempo;
    self->beatlen = beatlen;
}


void playMusic(MusicPlayer *self, int unused){
    // to implement
    ;
}