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
    Timer timer; // only for Verification
} App;

typedef struct{
    Object super;
    int isPrintEnabled;
    int isBurstMode;
    uint8_t seqCounter; // sequence number counter
    // TODO Buffer Handling
} CanSenderPart5;

typedef enum {
    INIT,
    NEW_VALUE_WRITTEN,
    READ_DONE,
} Status;

typedef struct{
    Object super;

    // single CAN Msg 
    CANMsg msg;

    // regulatorBufferHdlr : write NEW_VALUE_WRITTEN, readRegulatorBuffer: write READ_DONE
    // @todo check if this causes race condition
    Status status;
}CanMsgWithReadFlag;

#define initCanMsgWithReadFlag() {\
    initObject(),\
    {},\
    INIT,\
}

typedef struct{
    Object super;
    CanMsgWithReadFlag canMsgBuffer[MAX_BUFFER_SIZE];// @todo decide buffer size based on some calculations
    uint8_t readIdx;
    uint8_t writeIdx;
    Timer timer;
    uint32_t prevMsgArrivalTime;
    uint32_t prevMsgDeliveryTime;
    uint8_t delta;
} Regulator;

/* initCanMsgWithReadFlag(),*/
// initialize RegulatorBufferHdlr
#define initRegulator() {\
    initObject(),\
    {},\
    0,\
    0,\
    initTimer(),\
    0,\
    0,\
    1,\
}

// init CAN Sender for Part5 
#define initCanSenderPart5() { \
    initObject(),\
    0,\
    0,\
    0,\
}

// initialize a MusicPlayer with its rank
#define initApp() { \
    initObject(), \
    0, \
    {}, \
    MUSICIAN, \
    initTimer(),\
}

extern App app;

/* CAN OPCODE */
typedef enum {
    /* NETWORK */
    SEARCH_NETWORK,
    CLAIM_EXISTENCE,
    CLAIM_CONDUCTORSHIP,
    ACK_CONDUCTORSHIP,
    OBT_CONDUCTORSHIP,
    /* MUSIC PLAYER */
    MUSIC_START_ALL,
    MUSIC_STOP_ALL,
    MUSIC_PLAY_NOTE_IDX, // with arg
    MUSIC_SYNC_LED, // with arg, to CONDUCTOR
    MUSIC_SET_KEY_ALL, // @CON, with arg
    MUSIC_SET_TEMPO_ALL, // @CON, with arg
    MUSIC_SET_VOL_ALL, // TODO: @CON, with arg
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
void toMusician(App*, int);
void toConductor(App*, int);
void constructCanMessage(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg);
void printAppVerbose(App *, int);
void printVerbose(App *, int);
void helperConductor(App *, int);
void helperMusician(App *, int);

void canSenderFcnPart5(CanSenderPart5 *, int);
void regulatorBufferHdlr(Regulator *, int);

void readRegulatorBuffer(Regulator *, CANMsg *msg);
void setDelta(Regulator *, int value);


#endif