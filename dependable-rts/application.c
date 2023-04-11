#include "application.h"
#include "toneGenerator.h"
#include "musicPlayer.h"
#include "userButton.h"
#include "network.h"
#include <stdlib.h>
#include <stdio.h>


App app = initApp();

UserButton userButton = initUserButton();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
SysIO sio0 = initSysIO(SIO_PORT0, &userButton, reactUserButton);


/* CAN MSG */
void constructCanMessage(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg){
    // Construct a CAN message of length 6: ['OP'|'ARG'(4)|'ENDING']
    // msgId, if needed
    msg->nodeId = network.rank;
    msg->length = 7;
    // operation
    msg->buff[0] = opcode;
    // receiver: xxxx for broadcast
    msg->buff[1] = receiver;
    // arg
    msg->buff[2] = (arg >> 24) & 0xFF; 
    msg->buff[3] = (arg >> 16) & 0xFF;
    msg->buff[4] = (arg >>  8) & 0xFF;
    msg->buff[5] = (arg      ) & 0xFF;
    // ending
    msg->buff[6] = 193;
}

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
#ifdef DEBUG
    char debugInfo[64] = {};
    snprintf(debugInfo, 64, "OP: 0x%02X, RE: 0x%02X, ARG: 0x%02X%02X%02X%02X, END: 0x%02X\n",
             msg.buff[0],
             msg.buff[1],
             msg.buff[2], msg.buff[3], msg.buff[4], msg.buff[5],
             msg.buff[6]);
    SCI_WRITE(&sci0, debugInfo);
#endif
    // vintage CAN message, just print it
    if (msg.buff[msg.length - 1] == 0){
        SCI_WRITE(&sci0, "Can msg received: ");
        SCI_WRITE(&sci0, msg.buff);
        SCI_WRITE(&sci0, "\n");
    }
    // INFO from message
    int sender    = msg.nodeId;
    CAN_OPCODE op = msg.buff[0];
    int receiver  = msg.buff[1];
    int arg =      (msg.buff[2] & 0xFF) << 24 | \
                   (msg.buff[3] & 0xFF) << 16 | \
                   (msg.buff[4] & 0xFF) << 8  | \
                   (msg.buff[5] & 0xFF);
    int ending    = msg.buff[6];
    char canInfo[64] = {};
    if (msg.length != 7) return;
    if (ending != 193) return;
    // CORE PARSER
    switch(op)
    {
        case DEBUG_OP:
            SCI_WRITE(&sci0, "Operation: DEBUG\n");
            snprintf(canInfo, 32, "Arg: %d\n", arg);
            SCI_WRITE(&sci0, canInfo);
            int nodeId = msg.nodeId;
            snprintf(canInfo, 64, "msg from nodeId: %d\n", nodeId);
            SCI_WRITE(&sci0, canInfo);
            //break;
        /* NETWORK */
        case SEARCH_NETWORK:
            SYNC(&network, handleJoinRequest, sender);
            break;
        case CLAIM_EXISTENCE:
            // TODO: maybe a new method
            SYNC(&network, handleJoinRequest, sender);
            break;
        case CLAIM_CONDUCTORSHIP:
            SYNC(&network, handleConductorshipRequest, sender);
            break;
        case ACK_CONDUCTORSHIP:
            SYNC(&network, handleConductorshipAck, arg);
            break;
        case OBT_CONDUCTORSHIP:
            SYNC(&network, changeConductor, sender);
            break;
        /* MUSIC */
        case MUSIC_START_ALL:
            SYNC(&musicPlayer, ensembleStart, arg);
            break;
        case MUSIC_PLAY_NOTE_IDX:
            SYNC(&musicPlayer, playIndexTone2, arg);
            break;
        case MUSIC_STOP_ALL:
            SYNC(&musicPlayer, ensembleStop, arg);
            break;
        case _TG_RDY:
            SYNC(&musicPlayer, ensembleReady, arg);
            break;
        default:;
            break;
    }
}


/* user input */
void clearbuffer(App *self, int unused) {
    for (int i = 0; i < MAX_BUFFER_SIZE - 1; i++){
        self->buffer[i] = '\0';
    };
    self->index = 0;
}

int parseValue(App *self, int unused) {
    self->buffer[self->index] = '\0';
    self->index = 0;
    return atoi(self->buffer);
}


/* SCI reader */
void reader(App *self, int c) {
    int arg;
    CANMsg msg;
    // For both modes
    switch (c)
    {
    /* buffered keyboard input */
    case '-':
    case '0' ... '9':;
        SCI_WRITE(&sci0, "Input stored in buffer: \'");
        SCI_WRITECHAR(&sci0, c);
        SCI_WRITE(&sci0, "\'\n");
        self->buffer[self->index] = c;
        self->index = (self->index + 1) % MAX_BUFFER_SIZE;
        break;
    /* verbose info */
    case 'v':
    case 'V':
        printAppVerbose(self, 0);
        SCI_WRITE(&sci0, "\n");
        SYNC(&network, printNetworkVerbose, 0);
        SCI_WRITE(&sci0, "\n");
        SYNC(&musicPlayer, printMusicPlayerVerbose, 0);
        SCI_WRITE(&sci0, "\n");
        break;
    /* Get conductorship, brutely, like KIM */
    case 'g':
    case 'G':
        SYNC(&network, obtainConductorship, 0);
        break;
    case 'd': // debug use
    case 'D':
        SYNC(&musicPlayer, ensembleStartAll2, 0);
        break;
    case 's':
    case 'S':
        arg = parseValue(self, /*unused*/0);
        SYNC(&musicPlayer, playIndexTone2, arg);
        break;
    case 'e':
    case 'E':
        SYNC(&musicPlayer, ensembleStartAll2, 0);
        break;
    case 'f':
    case 'F':
        SYNC(&musicPlayer, ensembleStopAll, 0);
        break;
    default:
        break;
    }
    // CONDUCTOR or MUSICIAN
    if (self->mode == CONDUCTOR){
        switch (c)
        {
        /* display helper */
        case '\n':;
            char guide [64] = {};
            snprintf(guide, 64,
                "rank: %d\n", network.rank);
            SCI_WRITE(&sci0, guide);
            break;
        case '\r':
            break;
        /* buffered keyboard input */
        case '-':
        case '0' ... '9':;
            break;
        /* backspace */
        case '\b':
            clearbuffer(self, 0);
            SCI_WRITE(&sci0, "Input buffer cleared, nothing saved\n");
            break;
        /* VOLUME CONTROL: mute/unmute, vol-down, vol-up*/
        //case ''

        /* key & tempo */
        case 't': // tempo
        case 'T':;
            break;
        case 'k':
        case 'K':;


            break;
        case 'r': // reset key and tempo
        case 'R':;
            break;
        /* MUSIC */
        case 's':
        case 'S':;
            // char musicStopInfo [32] = { };
            // int stopStatus = SYNC(&musicPlayer, musicStopStart, 0);
            // if (stopStatus)
            //     snprintf(musicStopInfo, 32, " X  Music Stoped\n");
            // else 
            //     snprintf(musicStopInfo, 32, " >  Music Started\n");
            // SCI_WRITE(&sci0, musicStopInfo);
            break;
        /* DEBUG */
        case 'd':
        case 'D':
            //arg = parseValue(self, /*unused*/0);
            //constructCanMessage(&msg, DEBUG_OP, 0, arg);
            break;
        /* verbose print */
        case 'h':
        case 'H':
            break;
        }
    } 
    else{ // Musician
        switch (c)
        {
        // case PRESS USER BUTTON
        case 't': // toggle mute
        case 'T':
            break;
        /* Claim conductorship, ask others for vote */
        case 'c':
        case 'C':
            SYNC(&network, claimConductorship, 0);
            break;
        default:
            break;
        }
    }
    // if 1
    

}


void toMusician(App *self, int unused){
    if (network.conductorRank == network.rank){
        SCI_WRITE(&sci0, "WARN: illegal musician change\n");
    }
    self->mode = MUSICIAN;
}


void toConductor(App *self, int unused){
    if (network.conductorRank != network.rank){
        SCI_WRITE(&sci0, "WARN: illegal condcutor change\n");
    }
    self->mode = CONDUCTOR;
}

void printAppVerbose(App *self, int unused){
    char appInfo[128] = {};
    snprintf(appInfo, 128,
             "------------------------App------------------------\n"
             "Mode: %d\n"
             "------------------------App------------------------\n",
             self->mode);
    SCI_WRITE(&sci0, appInfo);
}

// initilize the application
void startApp(App *self, int arg) {
    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    // sio
    SCI_WRITE(&sci0, "Hello from DRTS Group 15\n\n");
    BEFORE(toneGenerator.toneGenDeadline,&toneGenerator, playTone, /*unused*/0);
    // init network
    SYNC(&network, printNetwork, 0);
    SYNC(&network, searchNetwork, 0);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
