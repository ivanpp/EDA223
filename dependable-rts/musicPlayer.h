#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "TinyTimber.h"

#define KEY_MAX 5
#define KEY_MIN -5
#define TEMPO_MAX 300
#define TEMPO_MIN 30
#define TEMPO_DEFAULT 120
#define BEATMULT_DEFAULT 30000/TEMPO_DEFAULT

#define PERIODS_IDX_DIFF -15

typedef struct{
    Object super;
    int index;
    int key; // offset to the freq index
    int tempo; // bpm: beat per minute
    int beatMult; // ms
	int isStop; // be able to stop
    int hardStopped;
} MusicPlayer;

#define initMusicPlayer() \
    { initObject(), /*index*/0, /*key*/0, /*tempo*/TEMPO_DEFAULT, /*beatMult*/BEATMULT_DEFAULT, /*stop*/1, 1} 

extern MusicPlayer musicPlayer;

int setKey(MusicPlayer *, int);
int setTempo(MusicPlayer *, int);
int musicPauseUnpause(MusicPlayer *, int);
int musicPause(MusicPlayer *, int);
int musicUnpause(MusicPlayer *, int);
int musicStopStart(MusicPlayer *, int);
int musicStop(MusicPlayer *, int);
int musicStart(MusicPlayer *, int);
int musicReady(MusicPlayer *, int);
int musicUnready(MusicPlayer *, int);
void playIndexTone(MusicPlayer *, int);
void playMusic(MusicPlayer *, int);
void playMusicMasked(MusicPlayer *, int);
void printMusicStats(MusicPlayer *, int);

#endif