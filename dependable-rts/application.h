#ifndef APPLICATION_API
#define APPLICATION_API

#include "TinyTimber.h"
#include "canTinyTimber.h"
#include "sciTinyTimber.h"
#include "sioTinyTimber.h"


#define BROADCAST 0x00


/* keyboard */
#define MAX_BUFFER_SIZE 32


/* Application */
// musicplayer mode
typedef enum {
    CONDUCTOR,
    MUSICIAN,
} PlayerMode;

typedef struct {
    Object super;
    int index; // keyboard input
    char buffer[MAX_BUFFER_SIZE];
    PlayerMode mode; // mode
} App;

// initialize a MusicPlayer with its rank
#define initApp() { \
    initObject(), \
    0, \
    {}, \
    MUSICIAN, \
}

extern App app;


/* CAN OPCODE */
typedef enum {
    /* NETWORK */
    CLAIM_EXISTENCE,
    SEARCH_NETWORK,
    CLAIM_CONDUCTORSHIP,
    ACK_CONDUCTORSHIP,
    OBT_CONDUCTORSHIP,
    /* MUSIC PLAYER */
    MUSIC_RESTART,
    MUSIC_START,
    MUSIC_START_ALL,
    MUSIC_STOP,
    MUSIC_STOP_ALL,
    MUSIC_PAUSE,
    MUSIC_UNPAUSE,
    MUSIC_PLAY_NOTE_IDX, // with arg
    MUSIC_PLAY_NEXT_NOTE,
    MUSIC_MUTE,
    MUSIC_VOL_UP,
    MUSIC_VOL_DOWN,
    MUSIC_SET_VOL, // with arg
    MUSIC_SET_KEY_ALL, // with arg
    MUSIC_SET_TEMPO_ALL, // with arg

    DEBUG_OP
} CAN_OPCODE;

void reader(App*, int);
void receiver(App*, int);
void toMusician(App*, int);
void toConductor(App*, int);
void constructCanMessage(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg);
void printAppVerbose(App *self, int unused);

#endif