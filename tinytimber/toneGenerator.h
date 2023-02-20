#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAFE_VOLUME 5

///@brief   define tone generator deadline 
#define TONE_GEN_DEADLINE USEC(100)

typedef struct 
{
    Object super;
    int isPlaying;
    int toneFreq; //KHz
    int volume;
    int isMuted;
    /// @brief  member variable to check if deadline is enabled or disabled : part1_task3
    bool isDeadlineEnabled;

    ///@brief   tone-generator specific deadline 
    int toneGenDeadline;
} ToneGenerator;

#define initToneGenerator() \
    { initObject(), 0, 1000, 2, 0, false, 0}


int genTone(ToneGenerator *, int);
int playTone(ToneGenerator *, int);
int setFrequency(ToneGenerator *, int);
int setVolume(ToneGenerator *, int);
int adjustVolume(ToneGenerator *, int);

int toggleAudio(ToneGenerator *, int);





