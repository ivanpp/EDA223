#ifndef TONEGEN_H
#define TONEGEN_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 25
#define DEFAULT_VOLUME 25
#define MAX_FREQ 20000
#define MIN_FREQ 1
#define TONE_GEN_DEADLINE USEC(100)

typedef struct 
{
    Object super;
    int isPlaying;
    int toneFreq; //KHz
    int period; // usec
    int volume;
    int isMuted;
    int isBlank;
	int isStop; // be able to detect stop
    bool isDeadlineEnabled;
    int toneGenDeadline;
} ToneGenerator;

#define initToneGenerator() { \
    initObject(), \
    0, \
    /*freq*/ 1000, \
    /*period*/ 500, \
    /*volumn*/ DEFAULT_VOLUME, \
    /*mute*/ 0, \
    /*blank*/ 0, \
    /*stop*/ 1, \
    /*DDL*/ 0\
}

extern ToneGenerator toneGenerator;

void play_tone(ToneGenerator *, int);
int set_frequency(ToneGenerator *, int);
int set_period(ToneGenerator *, int);
int set_volume(ToneGenerator *, int);
int adjust_volume(ToneGenerator *, int);
int toggle_audio(ToneGenerator *, int);
void blank_tone(ToneGenerator *, int);
void unblank_tone(ToneGenerator *, int);
void start_toneGen(ToneGenerator *, int);
void stop_toneGen(ToneGenerator *, int);
int toggle_deadline_toneGen(ToneGenerator *, int);
void print_volume_info(ToneGenerator *, int);

#endif