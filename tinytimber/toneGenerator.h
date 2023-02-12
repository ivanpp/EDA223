#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 5

typedef struct 
{
    Object super;
    int isPlaying;
    int toneFreq; //KHz
    int volume;
    int isMuted;
} ToneGenerator;

#define initToneGenerator() \
    { initObject(), 0, 1000, 2, 0}


int genTone(ToneGenerator *, int);
int playTone(ToneGenerator *, int);
int setFrequency(ToneGenerator *, int);
int setVolume(ToneGenerator *, int);
int adjustVolume(ToneGenerator *, int);

int toggleAudio(ToneGenerator *, int);





