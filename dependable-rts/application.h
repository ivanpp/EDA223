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
    int numNode;
    PlayerMode mode; // mode
} App;

// initialize a MusicPlayer with its rank
#define initApp() { \
    initObject(), \
    0, \
    {}, \
    1, \
    CONDUCTOR, \
}


/* CAN OPCODE */
typedef enum {
    /* NETWORK */
    CLAIM_EXISTENCE,
    SEARCH_NETWORK,
    /* MUSIC PLAYER */
    MUSIC_RESTART,
    MUSIC_START,
    MUSIC_STOP,
    MUSIC_PAUSE,
    MUSIC_UNPAUSE,
    MUSIC_PLAY_NOTE, // with arg
    MUSIC_PLAY_NEXT_NOTE,
    MUSIC_MUTE,
    MUSIC_VOL_UP,
    MUSIC_VOL_DOWN,
    MUSIC_SET_VOL, // with arg
    MUSIC_SET_KEY, // with arg
    MUSIC_SET_TEMPO, // with arg

    DEBUG_OP
} CAN_OPCODE;

void reader(App*, int);
void receiver(App*, int);
void constructCanMessage(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg);

#endif