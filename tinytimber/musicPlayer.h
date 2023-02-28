#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "TinyTimber.h"

#define KEY_MAX 5
#define KEY_MIN -5
#define TEMPO_MAX 240
#define TEMPO_MIN 60

typedef struct{
    Object super;
    int index;
    int key; // offset to the freq index
    int tempo; // bpm: beat per minute
    int beatlen;
    int periods[32]; // periods for each notes
    int tempos[32];  // a: 2, b: 4, c: 1
} MusicPlayer;

#define initMusicPlayer() \
    { initObject(), /*index*/0, /*key*/0, /*tempo*/120, /*beatlen*/500, {}}

int setKey(MusicPlayer *, int);
// set tempo (bpm)
int setTempo(MusicPlayer *, int);
// set beat length for 'a', a beat (ms)
void setBeatLength(MusicPlayer *, int);

#endif