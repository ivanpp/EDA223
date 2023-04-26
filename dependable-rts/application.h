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
    /* SEARCH */
    SEARCH_NETWORK,
    CLAIM_EXISTENCE,
    /* CONDUCTORSHIP */
    CLAIM_CONDUCTORSHIP,
    ANSWER_CLAIM_CONDUCTOR,
    OBTAIN_CONDUCTORSHIP,
    /* DETECTION */
    NODE_REMAIN_ACTIVE,
    DETECT_OFFLINE_NODE,
    ANSWER_DETECT_OFFLINE,
    NOTIFY_NODE_OFFLINE, // TODO: @CON, with arg
    NODE_LOGIN_REQUEST,
    NODE_LOGIN_CONFIRM,
    /* MUSIC PLAYER */
    MUSIC_START_ALL,
    MUSIC_STOP_ALL,
    MUSIC_PLAY_NOTE_IDX, // with arg
    MUSIC_SYNC_LED, // with arg, to CONDUCTOR
    MUSIC_SET_KEY_ALL, // @CON, with arg
    MUSIC_SET_TEMPO_ALL, // @CON, with arg
    MUSIC_SET_VOL_ALL, // TODO: @CON, with arg
    /* TESTING */
    TEST_COMPETE_CONDUCTOR,
    TEST_NOTIFY_FAILURE,
    /* unused */
    MUSIC_START,
    MUSIC_RESTART,
    MUSIC_STOP,
    MUSIC_PAUSE,
    MUSIC_UNPAUSE,
    MUSIC_MUTE,
    MUSIC_VOL_UP,
    MUSIC_VOL_DOWN,
    DEBUG_OP
} CAN_OPCODE;

void reader(App*, int);
void receiver(App*, int);
void to_musician(App*, int);
void to_conductor(App*, int);
void construct_can_message(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg);
void print_app_verbose(App *, int);
void print_verbose(App *, int);
void helper_conductor(App *, int);
void helper_musician(App *, int);

#endif