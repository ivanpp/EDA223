#include "application.h"
#include "toneGenerator.h"
#include "musicPlayer.h"
#include "userButton.h"
#include "network.h"
#include "heartbeat.h"
#include <stdlib.h>
#include <stdio.h>
#include<string.h>

App app = initApp();
Regulator regulatorSw = initRegulator();

UserButton userButton = initUserButton();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

SysIO sio0 = initSysIO(SIO_PORT0, &userButton, reactUserButtonP5);

CanSenderPart5 canSenderPart5 = initCanSenderPart5();


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

void canSenderFcnPart5(CanSenderPart5 *self, int unused){
    CANMsg msg;
    int msgStatus = 1;

    // read value from Sequence counter and update CAN MsgID field and Transmit 
    msg.msgId = self->seqCounter;
    msgStatus = CAN_SEND(&can0,&msg);
    if(0 == msgStatus) /* Received Tx ACK, good to goto next steps */
    {
        if(self->isPrintEnabled){
            char seqCntrPrint [32];
            snprintf(seqCntrPrint, 32, "Sequence Num: %d sent\n",self->seqCounter);
            SCI_WRITE(&sci0, seqCntrPrint);
        }
        ++self->seqCounter;
        self->seqCounter = (self->seqCounter % 128);
    }
    if(self->isBurstMode == true) // 'n' is not yet pressed, so call yourself again
    {      
        AFTER(MSEC(500), self, canSenderFcnPart5, 0);
    }

}
void regulateMsg(Regulator *self, CANMsg *msgPtr) {
    char debugInfo[64]={};
    int rxTimeSec;
    if (self->ready) 
    {
        self->ready = false;
        // deliver msg Immediately 
        rxTimeSec = SEC_OF(T_SAMPLE(&self->timer));
        snprintf(debugInfo, 64, "        MsgId:%d delivered at %ds\n",msgPtr->msgId, rxTimeSec);
        SCI_WRITE(&sci0, debugInfo);
        AFTER(SEC(self->delta),&regulatorSw, dequeueCanMsg,0);
    } 
    else
    {
        enqueueCanMsg(self, msgPtr);
    }
    // Else enqueue
}

void trySendSingleCanMessage(App *self, int unused) {
    if(canSenderPart5.isBurstMode == false)
        SYNC(&canSenderPart5, canSenderFcnPart5, 0);
    else
        SCI_WRITE(&sci0, "can't send single CAN msg due to BURST Mode, press \'n\' to come out\n");
}
// must be called only by the dequeueCanMsg()
void resetIndices(Regulator *self, int unused)
{
    self->writeIdx = -1;
    self->readIdx = -1;
}
void setReadIdx(Regulator *self, int f_readIdx)
{
    if((f_readIdx >= 0) && ( f_readIdx <  MAX_BUFFER_SIZE)){
        self->readIdx = f_readIdx;
        #ifdef DEBUG
        char debugInfo[64]={};
        snprintf(debugInfo, 64, "ReadIdx set to: %d\n",self->readIdx);
        SCI_WRITE(&sci0, debugInfo);
        #endif
    }else{
        SCI_WRITE(&sci0, "readIdx not in [-1 to MAX_BUFFER_SIZE] range\n");
    }
}

void tryEnableBurstMode(App *self, int unused) {
    /* Check if CAN mode is already in BURST mode */
        
    if(canSenderPart5.isBurstMode == false)
    {
        canSenderPart5.isBurstMode = true;
        SYNC(&canSenderPart5,canSenderFcnPart5, 0);
    }
    else
    {
        SCI_WRITE(&sci0, "Already in CAN BURST Mode\n");
    }  
}

void disableBurstMode(App *self, int unused) 
{
    /* Come out of CAN Burst mode */
    canSenderPart5.isBurstMode = false;
}

void enqueueCanMsg(Regulator *self, CANMsg *msgPtr) 
{
    if((self->readIdx == 0 && self->writeIdx == MAX_BUFFER_SIZE - 1) || self->readIdx == self->writeIdx +1) 
    {
        //Write that message discareded. 
        #ifdef DEBUG
        SCI_WRITE(&sci0, "discarding msg as buffer is full\n");
        #endif
        return;
    }
    else 
    {
        // this part causes crash. Need to further investigate. But, for now, things seem to work as usual
        if (self->readIdx == -1) 
        {   
            ASYNC(&regulatorSw,setReadIdx,0); // this has to be ASYNC so that it happens concurrently for each queued msg
        }
        self->writeIdx = (self->writeIdx + 1) % MAX_BUFFER_SIZE;
        memcpy(&(self->canMsgBuffer[self->writeIdx]), msgPtr, sizeof(CANMsg));
        #ifdef DEBUG
        char debugInfo[78]={};
        snprintf(debugInfo, 78, "    [enqueue] MsgId: %d queued at WriteIdx: %d, current readIdx:%d\n",self->canMsgBuffer[self->writeIdx].msgId, self->writeIdx, self->readIdx);
        SCI_WRITE(&sci0, debugInfo);
        #endif
    }
}

// Deque and send, unless empty, in which case we set ready. 
void dequeueCanMsg(Regulator *self, int unused)
{
    char debugInfo[64]={};
    int rxTimeSec;

    if(self->readIdx == -1) 
    {
        #ifdef DEBUG        
        snprintf(debugInfo, 64, "Queue empty, resetting ready flag. \n");
        SCI_WRITE(&sci0, debugInfo);
        #endif
        self->ready = 1;
        return;
    }
    else
    {
        rxTimeSec = SEC_OF(T_SAMPLE(&self->timer));
        snprintf(debugInfo, 64, "        [dequeue] MsgId:%d delivered at %ds\n",self->canMsgBuffer[self->readIdx].msgId, rxTimeSec);
        SCI_WRITE(&sci0, debugInfo);
        if (self->readIdx == self->writeIdx) 
        {
            resetIndices(self, 0);
        }
        else 
        {
            self->readIdx = (self->readIdx + 1) % MAX_BUFFER_SIZE;
        }
        
        AFTER(SEC(self->delta),&regulatorSw, dequeueCanMsg,0);
    }
}

void setDelta(Regulator *self, int value)
{
    char printMsg[64]={};
    self->delta = value;
    snprintf(printMsg,64,"Inter-arrival time set to %ds\n",self->delta);
    SCI_WRITE(&sci0,printMsg);
}

void receiver(App *self, int unused) {
    CANMsg msg;
    int rxTime;
    CAN_RECEIVE(&can0, &msg);
    SYNC(&regulatorSw, regulateMsg, &msg);
    rxTime = SEC_OF(T_SAMPLE(&self->timer));
    
            // INFO from message
    int sender    = msg.nodeId;
    CAN_OPCODE op = msg.buff[0];
    int receiver  = msg.buff[1];
    int arg =      (msg.buff[2] & 0xFF) << 24 | \
                (msg.buff[3] & 0xFF) << 16 | \
                (msg.buff[4] & 0xFF) << 8  | \
                (msg.buff[5] & 0xFF);
    //int ending    = msg.buff[6];
#ifdef DEBUG
    char debugInfo[64] = {};

    if(canSenderPart5.isPrintEnabled)
    {
        snprintf(debugInfo, 64, "    [receiver] Received MsgId:%d at %d\n",msg.msgId, rxTime);
        //SCI_WRITE(&sci0, debugInfo);
    }
    
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
    }// end switch
    
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
    case 'n':
    case 'N':
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
    /* send 1 CAN Msg when 'o' is pressed */
    case 'o':
    case 'O':
        trySendSingleCanMessage(self, 0);
        break;
    case 'b':
    case 'B':
        tryEnableBurstMode(self, 0);
        break;
    case 'x':
    case 'X':
        disableBurstMode(self, 0);
        break;
    case 'm':
    case 'M':
        /* disable prints for CAN Msg Tx */
        canSenderPart5.isPrintEnabled = !(canSenderPart5.isPrintEnabled);
        if(canSenderPart5.isPrintEnabled)
            SCI_WRITE(&sci0, "CAN Msg Print enabled\n");
        else
            SCI_WRITE(&sci0, "CAN Msg Print disabled\n");
        break;
    /* obtain value and set the inter-arrival time (i.e. delta) */
    case 'l':
    case 'L':
        arg = parseValue(self,0);
        SYNC(&regulatorSw,setDelta,arg);
        break;
    default:
        break;
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
    char sizeInfo[64];
    snprintf(sizeInfo, 64, "size of regulator: %d Bytes\n", sizeof(regulatorSw));
    SCI_WRITE(&sci0, sizeInfo);
    CANMsg msg;
    snprintf(sizeInfo, 64, "size of CANMsg stored in buffer: %d Bytes\n", sizeof(msg) * MAX_BUFFER_SIZE);
    SCI_WRITE(&sci0, sizeInfo);
    SCI_WRITE(&sci0, "---------------------------------------------------\n");
}


void helperConductor(App *self, int unused){
    char helper [800];
    snprintf(helper, 800, 
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
            "---------------------Manual CAN Msg send ---------\n"
            "press \'o\' to transmit single CAN Msg\n"
            "press \'b\' to start Burst CAN Msgs\n"
            "press \'x\' to stop Burst CAN Msgs\n"
            "press \'m\' to enable or disable CAN Msg prints\n"
            "press number with \'l\' to set min. inter-arrival time\n"
            "press \'enter\' to display helper again\n\n"
            );
    SCI_WRITE(&sci0, helper);
}


void helperMusician(App *self, int unused){
    char helper [768];
    snprintf(helper, 768, 
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
            "---------------------Manual CAN Msg send ---------\n"
            "press \'o\' to transmit single CAN Msg\n"
            "press \'b\' to start Burst CAN Msgs\n"
            "press \'x\' to stop Burst CAN Msgs\n"
            "press \'m\' to enable or disable CAN Msg prints\n"
            "press number with \'l\' to set min. inter-arrival time\n"
            );
    SCI_WRITE(&sci0, helper);
}


// initilize the application
void startApp(App *self, int arg) {
    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SIO_INIT(&sio0);
    SCI_WRITE(&sci0, "Hello from DRTS Group 15\n\n");

}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}