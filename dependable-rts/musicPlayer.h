#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "TinyTimber.h"
#include "network.h"

#define KEY_MAX 5
#define KEY_MIN -5
#define KEY_DEFAULT 0
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
    int ensembleStop;
} MusicPlayer;

#define initMusicPlayer() { \
    initObject(), \
    /*index*/ 0, \
    /*key*/ KEY_DEFAULT, \
    /*tempo*/ TEMPO_DEFAULT, \
    /*beatMult*/ BEATMULT_DEFAULT, \
    /*SP: stop*/ 1, \
    /*SP: hardStop*/ 1, \
    /*MP: ensembleStop*/ 1, \
} 

extern App app;
extern MusicPlayer musicPlayer;


/* Single-player */
int setKey(MusicPlayer *, int);
int setTempo(MusicPlayer *, int);
int musicPauseUnpause(MusicPlayer *, int);
int musicPause(MusicPlayer *, int);
int musicUnpause(MusicPlayer *, int);
int musicStopStart(MusicPlayer *, int);
int musicStop(MusicPlayer *, int);
int musicStart(MusicPlayer *, int);
void playMusic(MusicPlayer *, int);
/* Multi-player */
void playIndexTone(MusicPlayer *, int);
void playIndexToneNxt(MusicPlayer *, int);
void LEDcontroller(MusicPlayer *, int);
void ensembleReady(MusicPlayer *, int);
void ensembleStop(MusicPlayer *, int);
void ensembleStartAll(MusicPlayer *, int);
void ensembleStopAll(MusicPlayer *, int);
void ensembleRestartAll(MusicPlayer *, int);
void playMusicMasked(MusicPlayer *, int);
/* MP: key, tempo, mute*/
int setKeyAll(MusicPlayer *, int);
int setTempoAll(MusicPlayer *, int);
void resetAll(MusicPlayer *, int);
int toggleMusic(MusicPlayer *, int);
/* Information */
void printMusicPlayerVerbose(MusicPlayer *, int);
/* deprecated */
void debugStopStatus(MusicPlayer *, int);

#endif