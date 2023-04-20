#include "application.h"
#include "toneGenerator.h"
#include "musicPlayer.h"
#include "userButton.h"
#include "network.h"
#include "heartbeat.h"
#include <stdlib.h>
#include <stdio.h>


App app = initApp();

UserButton userButton = initUserButton();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
SysIO sio0 = initSysIO(SIO_PORT0, &userButton, reactUserButtonP2);


/* CAN MSG */
void constructCanMessage(CANMsg *msg, CAN_OPCODE opcode, int receiver, int arg){
    // Construct a CAN message of length 7: ['OP'|'RE'|'ARG'(4)|'ENDING']
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
    // INFO from message
    int sender    = msg.nodeId;
    CAN_OPCODE op = msg.buff[0];
    int receiver  = msg.buff[1];
    int arg =      (msg.buff[2] & 0xFF) << 24 | \
                   (msg.buff[3] & 0xFF) << 16 | \
                   (msg.buff[4] & 0xFF) << 8  | \
                   (msg.buff[5] & 0xFF);
    int ending    = msg.buff[6];
#ifdef DEBUG
    char debugInfo[64] = {};
    snprintf(debugInfo, 64, "[0x%02X]: OP: 0x%02X, RE: 0x%02X, ARG: 0x%02X%02X%02X%02X, END: 0x%02X\n",
             sender,
             msg.buff[0],
             msg.buff[1],
             msg.buff[2], msg.buff[3], msg.buff[4], msg.buff[5],
             ending);
    SCI_WRITE(&sci0, debugInfo);
#endif
    // Check
    if (msg.length != 7) return;
    if (receiver != 0 && receiver != network.rank) return;
    //if (ending != 193) return;
    // CORE PARSER
    switch(op)
    {
        /* NETWORK */
        case SEARCH_NETWORK:
            SYNC(&network, handleJoinRequest, sender);
            break;
        case CLAIM_EXISTENCE:
            // TODO: maybe a new method, to reduce communication
            SYNC(&network, handleJoinRequest, sender);
            break;
        case CLAIM_CONDUCTORSHIP:
            SYNC(&network, handleClaimRequest, sender);
            break;
        case ACK_CONDUCTORSHIP:
            SYNC(&network, handleConductorshipAck, arg);
            break;
        case OBT_CONDUCTORSHIP:
            SYNC(&network, changeConductor, sender);
            break;
        /* MUSIC */
        case MUSIC_START_ALL:
            SYNC(&musicPlayer, ensembleReady, arg);
            break;
        case MUSIC_STOP_ALL:
            SYNC(&musicPlayer, ensembleStop, arg);
            break;
        case MUSIC_PLAY_NOTE_IDX:
            SYNC(&musicPlayer, playIndexTone, arg);
            break;
        case MUSIC_SYNC_LED:
            SYNC(&musicPlayer, LEDcontroller, arg);
            break;
        case MUSIC_SET_KEY_ALL:
            SYNC(&musicPlayer, setKey, arg);
            break;
        case MUSIC_SET_TEMPO_ALL:
            SYNC(&musicPlayer, setTempo, arg);
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
    /* backspace */
    case '\b':
        clearbuffer(self, 0);
        SCI_WRITE(&sci0, "Input buffer cleared, nothing saved\n");
        break;
    /* verbose info */
    case 'v':
    case 'V':
        printVerbose(self, 0);
        break;
    /* Get conductorship, brutely, like KIM */
    case 'x':
    case 'X':
        SYNC(&network, obtainConductorship, 0);
        break;
    /* manually trigger searching of network */
    case 'z':
    case 'Z':
        SYNC(&network, searchNetwork, 0);
        break;
    /* arrow-up: volume-up */
    case 0x1e:
        SYNC(&toneGenerator, adjustVolume, 1);
        break;
    /* arrow-down: volume-down */
    case 0x1f:
        SYNC(&toneGenerator, adjustVolume, -1);
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
            helperConductor(self, 0);
            break;
        case '\r':
            break;
        /* MUSIC */
        case 'a': // restart
        case 'A':
            SYNC(&network, ensembleRestartAll, 0);
            break;
        case 's': // start
        case 'S':
            SYNC(&musicPlayer, ensembleStartAll, 0);
            break;
        case 'd': // stop
        case 'D':
            SYNC(&musicPlayer, ensembleStopAll, 0);
            break;
        case 'k':
        case 'K': // key
            arg = parseValue(self, 0);
            SYNC(&musicPlayer, setKeyAll, arg);
            break;
        case 'j':
        case 'J': // tempo
            arg = parseValue(self, 0);
            SYNC(&musicPlayer, setTempoAll, arg);
            break;
        case 'r':
        case 'R': // reset: key, tempo
            SYNC(&musicPlayer, resetAll, 0);
            break;
        case 'h':
        case 'H': // toggle heartbeat
            SYNC(&heartbeatCon, toggleHeartbeat, 0);
            break;
        case 'g':
        case 'G': // set heartbeat period (s)
            arg = parseValue(self, 0);
            SYNC(&heartbeatCon, setHeartbeatPeriod, arg);
            break;
        default:
            break;
        }
    } 
    else{ // Musician
        switch (c)
        {
        /* display helper */
        case '\n':;
            helperMusician(self, 0);
            break;
        case '\r':
            break;
        case 't': // toggle mute
        case 'T':
            SYNC(&musicPlayer, toggleMusic, 0);
            break;
        /* Claim conductorship, ask others for vote */
        case 'c':
        case 'C': // problem 2
            SYNC(&network, claimConductorship, 0);
            break;
        case 'h':
        case 'H':
            SYNC(&heartbeatMus, toggleHeartbeat, 0);
            break;
        case 'g':
        case 'G':
            arg = parseValue(self, 0);
            SYNC(&heartbeatMus, setHeartbeatPeriod, arg);
            break;
        default:
            break;
        }
    }
}


void toMusician(App *self, int unused){
    if (network.conductorRank == network.rank){
        SCI_WRITE(&sci0, "WARN: illegal musician change\n");
    }
    self->mode = MUSICIAN;
    // LED
    int muteStatus = toneGenerator.isMuted;
    int safeTime = 4 * musicPlayer.beatMult;
    if (muteStatus)
        AFTER(MSEC(safeTime), &sio0, sio_write, 1); // unlit LED (safe)
    else
        AFTER(MSEC(safeTime), &sio0, sio_write, 0); // lit LED (safe)
}


void toConductor(App *self, int unused){
    if (network.conductorRank != network.rank){
        SCI_WRITE(&sci0, "WARN: illegal condcutor change\n");
    }
    self->mode = CONDUCTOR;
}


/* Information */
void printAppVerbose(App *self, int unused){
    SCI_WRITE(&sci0, "------------------------App------------------------\n");
    if(self->mode == MUSICIAN)
        SCI_WRITE(&sci0, "Mode: MUSICIAN\n");
    if(self->mode == CONDUCTOR)
        SCI_WRITE(&sci0, "Mode: CONDUCTOR\n");
    //SCI_WRITE(&sci0, "------------------------App------------------------\n");
}


void printVerbose(App *self, int unused){
    printAppVerbose(self, 0);
    //SCI_WRITE(&sci0, "\n");
    SYNC(&network, printNetworkVerbose, 0);
    //SCI_WRITE(&sci0, "\n");
    SYNC(&musicPlayer, printMusicPlayerVerbose, 0);
    //SCI_WRITE(&sci0, "\n");
    SCI_WRITE(&sci0, "---------------------------------------------------\n");
}


void helperConductor(App *self, int unused){
    char helper [768];
    snprintf(helper, 768, 
            "--------------------MUSICPLAYER--------------------\n"
            "press \'a\' ⟳ to restart\n"
            "press \'s\' ▶ to start\n"
            "press \'d\' ▢ to stop\n"
            "press \'j\' ♩ to set the tempo\n"
            "press \'k\' ♯ to set the key\n"
            "press \'r\' (or userButton for 2s) to reset\n"
            "press \'↑\' to volumn-up\n"
            "press \'↓\' to volumn-down\n"
            "----------------------NETWORK----------------------\n"
            "press \'z\' to search network\n"
            "press \'x\' to get conductorship\n"
            "---------------------HEARTBEAT---------------------\n"
            "press \'h\' to toggle heartbeat\n"
            "press \'g\' to set heartbeat period\n"
            "press \'enter\' to display helper again\n\n"
            );
    SCI_WRITE(&sci0, helper);
}


void helperMusician(App *self, int unused){
    char helper [512];
    snprintf(helper, 512, 
            "--------------------MUSICPLAYER--------------------\n"
            "press \'t\' (or userButton) to mute/unmute\n"
            "press \'↑\' to volumn-up\n"
            "press \'↓\' to volumn-down\n"
            "----------------------NETWORK----------------------\n"
            "press \'z\' to search network\n"
            "press \'x\' to get conductorship\n"
            "---------------------HEARTBEAT---------------------\n"
            "press \'h\' to toggle heartbeat\n"
            "press \'g\' to set heartbeat period\n"
            "press \'enter\' to display helper again\n\n"
            );
    SCI_WRITE(&sci0, helper);
}


// initilize the application
void startApp(App *self, int arg) {
    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SIO_INIT(&sio0);
    SCI_WRITE(&sci0, "Hello from DRTS Group 15\n\n");
    BEFORE(toneGenerator.toneGenDeadline,&toneGenerator, playTone, /*unused*/0);
    // init network
    SYNC(&network, printNetwork, 0);
    SYNC(&network, searchNetwork, 0);
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
