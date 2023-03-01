#ifndef TONEGEN_H
#define TONEGEN_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 25
#define MAX_FREQ 20000
#define MIN_FREQ 1

///@brief   define tone generator deadline 
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
    /// @brief  member variable to check if deadline is enabled or disabled : part1_task3
    bool isDeadlineEnabled;

    ///@brief   tone-generator specific deadline 
    int toneGenDeadline;
} ToneGenerator;

#define initToneGenerator() \
    { initObject(), 0, /*freq*/1000, /*period*/500, /*volumn*/2, 0, false, 0}

extern ToneGenerator toneGenerator;

void playTone(ToneGenerator *, int);
int setFrequency(ToneGenerator *, int);
int setPeriod(ToneGenerator *, int);
int setVolume(ToneGenerator *, int);
int adjustVolume(ToneGenerator *, int);
int toggleAudio(ToneGenerator *, int);
int toggleDeadlineTG(ToneGenerator *, int);
void mute(ToneGenerator *self, int unused);
void unmute(ToneGenerator *self, int unused);

#endif