#include "application.h"
#include "toneGenerator.h"
#include "musicPlayer.h"
#include "userButton.h"
#include "network.h"
#include "heartbeat.h"
#include <stdlib.h>
#include <stdio.h>


App app = initApp();
Regulator regulatorSw = initRegulator();

UserButton userButton = initUserButton();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
//Can can0 = initCan(CAN_PORT0, &regulatorSw, regulatorBufferHdlr);

SysIO sio0 = initSysIO(SIO_PORT0, &userButton, reactUserButtonP2);

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

// void regulatorBufferHdlr(Regulator *self, int unused)
// {
//     char printTimingInfo[90]={};
//     uint8_t currentWriteIdx = self->writeIdx;
//     if((self->canMsgBuffer[self->writeIdx].status == INIT) ||
//     (self->canMsgBuffer[self->writeIdx].status == READ_DONE)){
//         // copy CAN msg into RegulatorBuffer at correct index
//         CAN_RECEIVE(&can0, &self->canMsgBuffer[self->writeIdx].msg);

//         // mark buffer as Written 
//         self->canMsgBuffer[self->writeIdx].status = NEW_VALUE_WRITTEN;

//         // update write index
//         ++(self->writeIdx);
//         self->writeIdx = self->writeIdx % MAX_BUFFER_SIZE;

//         // timing calculations
//         int currentMsgArrivalTime, diff;
//         currentMsgArrivalTime = SEC_OF(T_SAMPLE(&self->timer));

//         // calculate time difference to previous msg and print
//         diff = currentMsgArrivalTime - self->prevMsgArrivalTime;
//         snprintf(printTimingInfo,90, "[Regulator]MsgId:%d arrival time:%ds, diff to prev arrival:%ds\n",
//         self->canMsgBuffer[currentWriteIdx].msg.msgId,
//         currentMsgArrivalTime,
//         diff);
//         SCI_WRITE(&sci0, printTimingInfo);

//         // current arrival time becomes previous arrival time for the next msg, so update in Regulator data structure
//         self->prevMsgArrivalTime = currentMsgArrivalTime;

//         // Now, msg is ready for delivery. Print this info
//         int currentMsgDeliveryTime, deliveryDiff;
//         currentMsgDeliveryTime = SEC_OF(T_SAMPLE(&self->timer));
//         deliveryDiff = currentMsgDeliveryTime - self->prevMsgDeliveryTime;
        
//         snprintf(printTimingInfo,90, "[Regulator]MsdId:%d ready for delivery at:%ds, diff to prev delivery:%ds\n", 
//         self->canMsgBuffer[self->writeIdx].msg.msgId,
//         currentMsgDeliveryTime,
//         deliveryDiff);
//         SCI_WRITE(&sci0, printTimingInfo);

//         self->prevMsgDeliveryTime = currentMsgDeliveryTime;

//         // Check if regulation is necessary
//         int arrivalDeliveryDiff;
//         arrivalDeliveryDiff = currentMsgArrivalTime - self->prevMsgDeliveryTime;

//         if(arrivalDeliveryDiff >= self->delta)
//         {
//             // send msg immediately to application or user or receiver
//             SYNC(&app,receiver,unused);
//         }else if(deliveryDiff >= (self->delta)){
//             AFTER(self->writeIdx*SEC(self->delta),&app, receiver, unused);
//         }else{
//             // nothing to do here, explicitly written to avoid confusions
//         }


//     }else{
//         SCI_WRITE(&sci0, "[WARN]Discarding msg as Buffer is full, call readRegulatorBuffer() to discard old data\n");
//     }
// }

// void readRegulatorBuffer(Regulator *self, CANMsg *msgPtr)
// {
// #ifdef DEBUG
//     char debugPrint[64]={};
//     snprintf(debugPrint,64,"readRegulatorBuffer()[%d] status:%d\n",self->readIdx,self->canMsgBuffer[self->readIdx].status);
//     //SCI_WRITE(&sci0, debugPrint);
// #endif
//     // isReadDone is initialized with -1, updated to 1 by readRegulatorBuffer, 
//     if(self->canMsgBuffer[self->readIdx].status == NEW_VALUE_WRITTEN){
//         *msgPtr = self->canMsgBuffer[self->readIdx].msg;
//         self->canMsgBuffer[self->readIdx].status = READ_DONE;//mark corresponding Index as done
//         // @todo does it cause Race condition or invalid values as status is updated by both readRegulatorBuffer()
//         // and regulatorBufferHdlr ??

//         // update read Index
//         self->readIdx++;
//         self->readIdx = self->readIdx % MAX_BUFFER_SIZE;
//     }else{
//         SCI_WRITE(&sci0, "No new data to read\n");
//     }
// }

void regulateMsg(Regulator *self, CANMsg *msgPtr) {
    if (self->ready) 
    {
        processMsg(self, msgPtr);
        self->ready = false;
        //AFTER(self->delta, self, dequeue, 0);
    } 
    else
    {
        enqueue(self, msgPtr);
    }
    // Else enqueue
}

void enqueue(Regulator *self, CANMsg *msgPtr) 
{
    self->writeIdx = (++self->writeIdx)% MAX_BUFFER_SIZE;
    
    // first increment and then write
    if(( self->writeIdx == MAX_BUFFER_SIZE -1 && self->readIdx == -1) ||
    (self->readIdx == self->writeIdx)){ // Queue is Full
    SCI_WRITE(&sci0, "RegulatorBuffer Full\n");
    }else if(){

    }else{


        // add to queue
    }


    // Put in queue
}

// Deque and send, unless empty, in which case we set ready. 
void dequeue(Regulator *self, int unused)
{
    char debugInfo[64]={};

    if(self->writeIdx == -1) //Check if buffer is empty.  
    {
        setReadyFlag(self, 0);
        return;
    }
    else 
    {
        // first increment and then read
        self->readIdx = (++self->readIdx)% MAX_BUFFER_SIZE;

        if(self->readIdx == self->writeIdx) 
        {
            // nothing to read, set indices to -1 to indicate this
            self->readIdx = -1;
            self->writeIdx = -1;
        }
        //Dequeue msg and call processMsg:
        CANMsg msg = self->canMsgBuffer[self->readIdx];
        //Processing:
        snprintf(debugInfo, 64, "[dequeue] MsgId:%d delivered at %d\n",msg.msgId, rxTime);
        SCI_WRITE(&sci0, debugInfo);
        //New periodic call:
        AFTER(self->delta, self, dequeue, 0); 
    }
}

void setReadyFlag(Regulator *self, int unused)
{
    self->ready = 1;
}


void setDelta(Regulator *self, int value)
{
    char printMsg[64]={};
    self->delta = value;
    snprintf(printMsg,64,"Inter-arrival time set to %ds\n",self->delta);
    SCI_WRITE(&sci0,printMsg);
}

void processMsg(Regulator *self, CANMsg *msgPtr)
{
    
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
    //snprintf(debugInfo, 64, "[0x%02X]: OP: 0x%02X, RE: 0x%02X, ARG: 0x%02X%02X%02X%02X, END: 0x%02X\n",
    //         sender,
    //         msg.buff[0],
    //         msg.buff[1],
    //         msg.buff[2], msg.buff[3], msg.buff[4], msg.buff[5],
    //         ending);
    if(canSenderPart5.isPrintEnabled)
    {
        snprintf(debugInfo, 64, "[receiver] Received MsgId:%d at %d\n",msg.msgId, rxTime);
        SCI_WRITE(&sci0, debugInfo);
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
    /* send 1 CAN Msg when 'o' is pressed */
    case 'o':
    case 'O':
        if(canSenderPart5.isBurstMode == false)
            SYNC(&canSenderPart5,canSenderFcnPart5, 0);
        else
            SCI_WRITE(&sci0, "can't send single CAN msg due to BURST Mode, press \'n\' to come out\n");
        break;
    case 'b':
    case 'B':
        /* Check if CAN mode is already in BURST mode */
        if(canSenderPart5.isBurstMode == false){
            canSenderPart5.isBurstMode = true;
            SYNC(&canSenderPart5,canSenderFcnPart5, 0);
        }
        else
            SCI_WRITE(&sci0, "Already in CAN BURST Mode\n");
        break;
    case 'n':
    case 'N':
        /* Come out of CAN Burst mode */
        canSenderPart5.isBurstMode = false;
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
            "press \'n\' to stop Burst CAN Msgs\n"
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
            "press \'n\' to stop Burst CAN Msgs\n"
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
    //BEFORE(toneGenerator.toneGenDeadline,&toneGenerator, playTone, /*unused*/0);
    
    // init network
    //SYNC(&network, printNetwork, 0);
    //SYNC(&network, searchNetwork, 0);
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
