#include "musicPlayer.h"
#include "toneGenerator.h"
#include "systemPorts.h"
#include "brotherJohn.h"

MusicPlayer musicPlayer = initMusicPlayer();


/* Single-player */

// set Key
int setKey(MusicPlayer *self, int key){
    key = key < KEY_MIN ? KEY_MIN : key;
    key = key > KEY_MAX ? KEY_MAX : key;
    self->key = key;
    return key;
}


// set tempo (bpm) for 'a'
int setTempo(MusicPlayer *self, int tempo){
    tempo = tempo < TEMPO_MIN ? TEMPO_MIN : tempo;
    tempo = tempo > TEMPO_MAX ? TEMPO_MAX : tempo;
    int beatMult = 30 * 1000 / tempo;
    self->tempo   = tempo;
    self->beatMult = beatMult;
    return tempo;
}


// pause/unpause
int musicPauseUnpause(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, startToneGen, 0);
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    } else {
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
    }
    return self->isStop;
}


int musicUnpause(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;  
        SYNC(&toneGenerator, startToneGen, 0);
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    }
    return self->isStop;
}


int musicPause(MusicPlayer *self, int unused){
    if (!self->hardStopped) {
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
    }
    return self->isStop;
}


// stop/restart
int musicStopStart(MusicPlayer *self, int unused){
    // if music is stop
    if (self->hardStopped) {
         // reset the index, start from the begining
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, startToneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    } else{
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}


int musicStart(MusicPlayer *self, int unused){
    if (self->hardStopped) {
        self->isStop = 0;
        self->hardStopped = 0;
        SYNC(&toneGenerator, startToneGen, 0);
        self->index = 0;
        SYNC(&toneGenerator, playTone, 0);
        playMusic(self, 0);
    }
    return self->isStop;
}


int musicStop(MusicPlayer *self, int unused){
    if (!self->hardStopped){
        self->isStop = 1;
        SYNC(&toneGenerator, stopToneGen, 0);
        self->index = 0;
    }
    return self->isStop;
}


// scheduler
void playMusic(MusicPlayer *self, int unused){
    // get next tone info
    if (self->isStop) {
        self->hardStopped = 1;
        return;
    }
    int period, tempo, beatLen;
    period = pianoPeriods[brotherJohn[self->index] + self->key - PERIODS_IDX_DIFF]; //Get period
    tempo = tempos[self->index]; // val can be 2, 4, 1
    beatLen = self->beatMult * tempo; // Get beatLen
    // LED: lit at the begining, unlit in the middle of a beat
    // since we stores int 2, 4, 1 for 1-beat, 2-beat, half-beat
    // we can use it to schedule LED too
    // first, start with the STUPID way
    switch(tempo){
        case(1):
            AFTER(MSEC(0), &sio0, sio_toggle, 0);
            break;
        case(2):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(beatLen/2), &sio0, sio_toggle, 0);
            break;
        case(4):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(1*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(2*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(3*beatLen/4), &sio0, sio_toggle, 0);
            break;
    }
    // set tone generator
    ASYNC(&toneGenerator, blankTone, 0);
    ASYNC(&toneGenerator, setPeriod, period);
    AFTER(MSEC(50), &toneGenerator, unblankTone, 0);
    // next tone
    self->index++;
    self->index = self->index % MUSIC_LENGTH;
    // call it self?
    AFTER(MSEC(beatLen), self, playMusic, 0);
}


/* Multi-player */

// 1. set period
// 2. unblank
// 3. blank, send can msg to next node
void playIndexTone(MusicPlayer *self, int idx){
    // check index first
    if (idx < 0 || idx > MUSIC_LENGTH - 1) {
        char debugInfo[32] = {};
        snprintf(debugInfo, 32, "WARN: invalid idx for song: %d\n", idx);
        SCI_WRITE(&sci0, debugInfo);
        return;
    }
    int period, tempo, beatLen;
    period = pianoPeriods[brotherJohn[idx] - PERIODS_IDX_DIFF];
    tempo = tempos[idx];
    beatLen = self->beatMult * tempo;
#ifdef DEBUG
    char debug[64] = {};
    snprintf(debug, 64, "[%d]: period: %d, beatLen: %d\n", idx, period, beatLen);
    SCI_WRITE(&sci0, debug);
#endif
    if (app.mode == CONDUCTOR) 
        LEDcontroller(self, tempo);
    else {
        CANMsg msg;
        constructCanMessage(&msg, MUSIC_SYNC_LED, network.conductorRank, tempo);
        CAN_SEND(&can0, &msg); // >> LEDcontroller(tempo)
    }
    ASYNC(&toneGenerator, setPeriod, period);
    AFTER(MSEC(50), &toneGenerator, unblankTone, 0);
    AFTER(MSEC(beatLen), &toneGenerator, blankTone, 0);
    // next tone
    AFTER(MSEC(beatLen), self, playIndexToneNxt, idx);
}


// send idx (which note to play) to next node
void playIndexToneNxt(MusicPlayer *self, int idx){
    if(self->ensembleStop)
        return;
    int nextTone, nextNode;
    nextTone = (idx + 1) % MUSIC_LENGTH;
    nextNode = SYNC(&network, getNextNode, 0);
    CANMsg msg;
    constructCanMessage(&msg, MUSIC_PLAY_NOTE_IDX, nextNode, nextTone);
    CAN_SEND(&can0, &msg); // >> playIndexTone(idx++)
}


void LEDcontroller(MusicPlayer *self, int tempo){
    if (app.mode == CONDUCTOR){
        int beatLen = self->beatMult * tempo;
        switch(tempo){
        case(1):
            AFTER(MSEC(0), &sio0, sio_toggle, 0);
            break;
        case(2):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(beatLen/2), &sio0, sio_toggle, 0);
            break;
        case(4):
            AFTER(MSEC(0), &sio0, sio_write, 0); // lit
            AFTER(MSEC(1*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(2*beatLen/4), &sio0, sio_toggle, 0);
            AFTER(MSEC(3*beatLen/4), &sio0, sio_toggle, 0);
            break;
        default:
            SCI_WRITE(&sci0, "WARN: undefined beat length for LED controller\n");
            break;
        }
    }
}


void ensembleReady(MusicPlayer *self, int unused){
    // setup ToneGenerator, ready to play, then blank it
    SYNC(&toneGenerator, startToneGen, 0);
    SYNC(&toneGenerator, playTone, 0);
    SYNC(&toneGenerator, blankTone, 0);
    // ready to send can msg
    self->ensembleStop = 0;
}


void ensembleStop(MusicPlayer *self, int unused){
    SYNC(&toneGenerator, stopToneGen, 0);
    self->ensembleStop = 1;
}


// CONDUCTOR: start playing music the round-robin way
void ensembleStartAll(MusicPlayer *self, int unused){
    if(app.mode != CONDUCTOR){
        SCI_WRITE(&sci0, "[PLAYER ERR]: Ensemble can be only started by CONDUCTOR");
        return;
    }
    // TODO: maybe check LOOPBACK mode etc.
    // get ready
    ensembleReady(self, 0);
    CANMsg msg;
    constructCanMessage(&msg, MUSIC_START_ALL, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> ensembleReady()
    // sync tempo, key
    setTempoAll(self, self->tempo);
    setKeyAll(self, self->key);
    // start from first node
    int firstNode = network.nodes[0];
    if (firstNode == network.rank)
        playIndexTone(self, 0);
    else{
        CANMsg msg;
        constructCanMessage(&msg, MUSIC_PLAY_NOTE_IDX, firstNode, 0);
        CAN_SEND(&can0, &msg); // >> playIndexTone(0)
    }
}


// CONDUCTOR: stop ensemble
void ensembleStopAll(MusicPlayer *self, int unused){
    CANMsg msg;
    constructCanMessage(&msg, MUSIC_STOP_ALL, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> ensembleStop()
    ensembleStop(self, 0);
}


// CONDUCTOR: restart ensemble
void ensembleRestartAll(MusicPlayer *self, int unused){
    ensembleStopAll(self, 0);
    // FIXME: use elegant way
    AFTER(SEC(1), &musicPlayer, ensembleStartAll, 0);
}


/* TODO the masked way*/
void playMusicMasked(MusicPlayer *self, int unused){
    ;
}


int setKeyAll(MusicPlayer *self, int key){
    key = setKey(self, key);
    CANMsg msg;
    constructCanMessage(&msg, MUSIC_SET_KEY_ALL, BROADCAST, key);
    CAN_SEND(&can0, &msg); // >> setKey(key)
    return key;
}


int setTempoAll(MusicPlayer *self, int tempo){
    tempo = setTempo(self, tempo);
    CANMsg msg;
    constructCanMessage(&msg, MUSIC_SET_TEMPO_ALL, BROADCAST, tempo);
    CAN_SEND(&can0, &msg); // >> setTempo(tempo)
    return tempo;
}


// reset key, tempo for all boards
void resetAll(MusicPlayer *self, int unused){
    if(app.mode == CONDUCTOR){
        setTempoAll(self, TEMPO_DEFAULT);
        setKeyAll(self, KEY_DEFAULT);
        SCI_WRITE(&sci0, "[PLAYER]: reset tempo and key\n");
    }else
        SCI_WRITE(&sci0, "[PLAYER ERR]: resetAll only allowed by CONDUCTOR\n");
}


int toggleMusic(MusicPlayer *self, int unused){
    if(app.mode == MUSICIAN){
        SCI_WRITE(&sci0, "[PLAYER]: toggled\n");
        SYNC(&toneGenerator, toggleAudio, 0);
        int muteStatus = toneGenerator.isMuted;
        if (muteStatus) {
            SIO_WRITE(&sio0, 1); // unlit LED
        } else {
            SIO_WRITE(&sio0, 0); // lit LED
        }
        return muteStatus;
    }else{
        SCI_WRITE(&sci0, "[PLAYER ERR]: toggleMusic only allowed by MUSICIAN\n");
    }
}


/* Information */

void debugStopStatus(MusicPlayer *self, int unused){
    char statsInfo[32] = {};
    snprintf(statsInfo, 32, "isStop: %d, hardStopped: %d\n", self->isStop, self->hardStopped);
    SCI_WRITE(&sci0, statsInfo);
}


void printVolumeInfo(MusicPlayer *self, int unused){
    ;
}


void printMusicPlayerVerbose(MusicPlayer *self, int unused){
    char musicPlayerInfo[256] = {};
    snprintf(musicPlayerInfo, 256,
             "--------------------MUSICPLAYER--------------------\n"
             "key(last): %d,  tempo(last): %d\n",
             self->key, self->tempo);
    SCI_WRITE(&sci0, musicPlayerInfo);
    // print volumn
    // print mute info
    SCI_WRITE(&sci0, "--------------------MUSICPLAYER--------------------\n");
}