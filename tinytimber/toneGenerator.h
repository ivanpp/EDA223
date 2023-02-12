#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct 
{
    Object super;
    int isPlaying;
    int toneFreq; //KHz
} ToneGenerator;

#define initToneGenerator() \
    { initObject(), 0}


int genTone(ToneGenerator *, int);
int playTone(ToneGenerator *, int);





