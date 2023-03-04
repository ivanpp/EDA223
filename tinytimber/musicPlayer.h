#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include "TinyTimber.h"

#define KEY_MAX 5
#define KEY_MIN -5
#define TEMPO_MAX 240
#define TEMPO_MIN 60

#define PERIODS_IDX_DIFF -15

typedef struct{
    Object super;
    int index;
    int key; // offset to the freq index
    int tempo; // bpm: beat per minute
    int beatMult; // ms
	int isStop; // be able to stop
} MusicPlayer;

typedef enum {
    MUSIC_PAUSE,     // play/pause
    MUSIC_STOP,      // play/stop
    MUSIC_MUTE,      // mute/unmute
    MUSIC_SET_KEY,   // set key
    MUSIC_SET_TEMPO, // set tempo
    MUSIC_VOL_UP,    // vol up
    MUSIC_VOL_DOWN,  // vol down
    MUSIC_DEBUG,     // debug
} MUSIC_PLAYER_OP;

#define initMusicPlayer() \
    { initObject(), /*index*/0, /*key*/0, /*tempo*/120, /*beatMult*/250, /*stop*/1} 

extern MusicPlayer musicPlayer;

int setKey(MusicPlayer *, int);
// set tempo (bpm)
int setTempo(MusicPlayer *, int);
// set beat length for 'a', a beat (ms)
void setBeatMult(MusicPlayer *, int);

void playMusic(MusicPlayer *, int);
int pauseMusic(MusicPlayer *, int); // 'p': pause/unpause music
int stopMusic(MusicPlayer *, int);  // 's': stop/restart music

#endif