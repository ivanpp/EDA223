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
} ToneGenerator;

#define initToneGenerator() \
    { initObject(), 0, 1000, 2}


int genTone(ToneGenerator *, int);
int playTone(ToneGenerator *, int);
int setFrequency(ToneGenerator *, int);
int setVolume(ToneGenerator *, int);
int adjustVolume(ToneGenerator *, int);





