#include "TinyTimber.h"
#include "application.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include "toneGenerator.h"
#include "backgroundLoad.h"
#include "userButton.h"
#include "musicPlayer.h"
#include "systemPorts.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

App app = initApp();

BackgroundLoad backgroundLoad = initBackgroundLoad();
UserButton userButton = initUserButton();

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
//SysIO sio0 = initSysIO(SIO_PORT0, &app, sioDebug);
SysIO sio0 = initSysIO(SIO_PORT0, &userButton, reactUserButton);

/* SIO DEBUG */
void sioDebug(App *self, int unused){
    SCI_WRITE(&sci0, "SIO invoked\n");
}

/* CAN MSG */
void constructCanMessage(CANMsg *msg, MUSIC_PLAYER_OP op, int arg){
    // Construct a CAN message of length 6: ['OP'|'ARG'(4)|'ENDING']
    msg->length = 6;
    // operation
    msg->buff[0] = op;
    // arg
    msg->buff[1] = (arg >> 24) & 0xFF; 
    msg->buff[2] = (arg >> 16) & 0xFF;
    msg->buff[3] = (arg >>  8) & 0xFF;
    msg->buff[4] = (arg      ) & 0xFF;
    // ending
    msg->buff[5] = 193;
}

void receiver(App *self, int unused) {
    // CAN message: ['OP'|'ARG'(4)|'ENDING']
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    // vintage CAN message, just print it
    if (msg.buff[msg.length - 1] == 0){
        SCI_WRITE(&sci0, "Can msg received: ");
        SCI_WRITE(&sci0, msg.buff);
        SCI_WRITE(&sci0, "\n");
    }
    // check length, and ending
    if (msg.length != 6) return;
    if (msg.buff[5] != 193) return;
    char debugInfo[48] = {};
    // print out the contents of messages received
    snprintf(debugInfo, 48, "OP: 0x%02X, ARG: 0x%02X%02X%02X%02X\n", 
             msg.buff[0], 
             msg.buff[1], msg.buff[2], msg.buff[3], msg.buff[4]);
    SCI_WRITE(&sci0, debugInfo);
    // parse the command from CONDUCTOR if we're MUSICIAN
    if (self->mode == MUSICIAN) {
        MUSIC_PLAYER_OP op = msg.buff[0];
        int arg = (msg.buff[1] << 24) | ((msg.buff[2] & 0xFF) << 16) | ((msg.buff[3] & 0xFF) << 8) | (msg.buff[4] & 0xFF);
        switch (op)
        {
        case MUSIC_PAUSE:
            SCI_WRITE(&sci0, "Operation: Pause/Unpause\n");
            SYNC(&musicPlayer, pauseMusic, /*unused*/0);
            break;
        case MUSIC_STOP:
            SCI_WRITE(&sci0, "Operation: Stop/Start\n");
            SYNC(&musicPlayer, stopMusic, /*unused*/0);
            break;
        case MUSIC_MUTE:
            SCI_WRITE(&sci0, "Operation: mute/unmute\n");
            SYNC(&toneGenerator, toggleAudio, /*unused*/0);
            break;
        case MUSIC_SET_KEY:
            SCI_WRITE(&sci0, "Operation: Set Key\n");
            SYNC(&musicPlayer, setKey, arg);
            break;
        case MUSIC_SET_TEMPO:
            SCI_WRITE(&sci0, "Operation: Set Tempo\n");
            SYNC(&musicPlayer, setTempo, arg);
            break;
        case MUSIC_VOL_UP:
            SCI_WRITE(&sci0, "Operation: Volumn Up\n");
            SYNC(&toneGenerator, adjustVolume, arg);
            break;
        case MUSIC_VOL_DOWN:
            SCI_WRITE(&sci0, "Operation: Volumn Down\n");
            SYNC(&toneGenerator, adjustVolume, arg);
            break;
        case MUSIC_DEBUG:
            SCI_WRITE(&sci0, "Operation: DEBUG\n");
            snprintf(debugInfo, 32, "Arg: %d\n", arg);
            SCI_WRITE(&sci0, debugInfo);
            break;
        default:
            break;
        }
    }
}

void clearbuffer(App *self, int unused) {
    for (int i = 0; i < MAX_BUFFER_SIZE - 1; i++){
        self->buffer[i] = '\0';
    };
    self->index = 0;
}

void clearhistory(App *self, int unused) {
    for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++){
        self->history[i] = 0;
    }
    self->count = 0;
    self->sum   = 0;
}

// Get the value from buffer, then reset it
int parseValue(App *self, int unused) {
    self->buffer[self->index] = '\0';
    self->index = 0;
    return atoi(self->buffer);
}

// https://devdocs.io/c/algorithm/qsort
int compare_ints(const void* a, const void* b){
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

void nhistory(App *self, int val) {
    int n = NHISTORY;
    int mleft_idx, mright_idx, median, count, tail, head;
    head   = self->count % MAX_HISTORY_SIZE; // current index (head)
    self->history[head] = val;
    count = ++self->count; // current count
    // min(n, count)-history will be applied
    n = n > count ? count : n; 
    tail = (count - NHISTORY) % MAX_HISTORY_SIZE; // (tail), start from negative, but never negative again
    // memcpy and sort
    if (tail > head){
        self->sum   = self->sum - self->history[tail-1]; // first in, first deducted from sum
        int len_t2e = MAX_HISTORY_SIZE-tail;
        int len_s2h = n - len_t2e;
        memcpy(self->sortedh        , self->history+tail, len_t2e*sizeof(int));
        memcpy(self->sortedh+len_t2e, self->history     , len_s2h*sizeof(int));
    } else if (tail == 0){
        if (count > n) self->sum = self->sum - self->history[MAX_HISTORY_SIZE-1];
        memcpy(self->sortedh, self->history+tail, n*sizeof(int));
    } else if (tail > 0){
        self->sum = self->sum - self->history[tail-1];
        memcpy(self->sortedh, self->history+tail, n*sizeof(int));
    } else{
        memcpy(self->sortedh, self->history, n*sizeof(int));
    }
    qsort(self->sortedh, n, sizeof(int), compare_ints);
    mleft_idx   = (n - 1) - (int) n / 2;
    mright_idx  = (n - 1) - (int) (n - 1) / 2;
    median      = (int) ((self->sortedh[mright_idx] + self->sortedh[mleft_idx]) / 2);
    self->sum += val;
    count = count > MAX_HISTORY_SIZE ? MAX_HISTORY_SIZE : count; // for print
    char temp [128] = {};
    snprintf(temp, 128, \
            "[%d/%d]: \'%d\' is stored, %d-history -> sum: %d, median: %d\n",
            count, MAX_HISTORY_SIZE, val, n, self->sum, median);
    SCI_WRITE(&sci0, temp);
}

void changeMode(App *self, int unused){
    if (self->mode == CONDUCTOR){
        self->mode = MUSICIAN;
        SCI_WRITE(&sci0, "Mode changed to: Musician\n");
    } else{
        self->mode = CONDUCTOR;
        SCI_WRITE(&sci0, "Mode changed to: Conductor\n");
    }
}

void reader(App *self, int c) {
    // create char inside switch case (only claim when needed)
    // think about the execution time
    // naming convention: funcInfo
    //char debugInfo[64] = { }; // for debuging
    int currentLoad;
    int volPercentage; // [0, 10]
    int val, arg; // 
    CANMsg msg;
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    if (c == 0x9){ // 'Tab'
        changeMode(self, /*unused*/0);
        return;
    }
    // for the CONDUCTOR, control yourself
    if (self->mode == CONDUCTOR) {
        switch (c)
        {
        /* display helper */
        case '\n':;
            char guide [1024] = {};
            snprintf(guide, 1024, 
                /* CHANGE TEXT ENCODING OF TERMINAL TO 'utf-8' */
                /* IF YOU WANT TO SHOW THE "ARROWS" */
                "-----------------CONDUCTOR MODE-----------------\n"
                "press \'Tab\' ↹ to change MODE\n"
                "Input number with \'-\' and \'0\'-\'9\'\n"
                "Maximum integer length: %d\n"
                "------------------------------------------------\n"
                "press \'e\' to end input and save\n"
                "press \'backspace\' to discard input\n"
                "press \'f\' to erase the history\n"
                "---------------BACKGROUND AND DDL---------------\n"
                //"press arrow-left to decrease bg load\n"
                //"press arrow-right to increase bg load\n"
                "press \'←\' to decrease bg load\n"
                "press \'→\' to increase bg load\n"
                "press \'d\' to toggle deadline\n"
                "-----------------VOLUMN CONTROL-----------------\n"
                //"press arrow-up to volumn-up\n"
                //"press arrow-down to volumn-down\n"
                "press \'↑\' to volumn-up\n"
                "press \'↓\' to volumn-down\n"
                "press \'m\' to mute/unmute\n"
                "------------------MUSIC PLAYER------------------\n"
                "press \'s\' ▶ to start/stop\n"
                "press \'p\' ⏯ to pause/unpause\n"
                "press \'k\' ♯ to set the key\n"
                "press \'t\' ♩ to set the tempo\n"
                "press \'r\' ♪ to show current tempo\n"
                "--------------------JOYSTICK--------------------\n"
                "hit(4+ times) to set a tempo\n"
                "press(1s) to PRESS_AND_HOLD mode\n"
                "press(2s) to reset tempo\n"
                "------------------------------------------------\n"
                "press \'enter\' to display helper again\n"
                "\n",
                MAX_BUFFER_SIZE - 1);
            SCI_WRITE(&sci0, guide);
            break;
        /* CRLF compatible */
        case '\r':
            break;
        /* valid integer input */
        case '-':
        case '0' ... '9':;
            SCI_WRITE(&sci0, "Input stored in buffer: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            self->buffer[self->index] = c;
            self->index = (self->index + 1) % MAX_BUFFER_SIZE;
            break;
        /* end of integer */
        case 'e':
        case 'E':;
            val = parseValue(self, /*unused*/0);
            nhistory(self, val);
            break;
        /* backspace */
        case '\b':
            clearbuffer(self, 0);
            SCI_WRITE(&sci0, "Input buffer cleared, nothing saved\n");
            break;
        /* set freq */
        case 'q':
        case 'Q':;
            char setFreqInfo [48] = { };
            val = parseValue(self, /*unused*/0);
            if (val < 1 || val > MAX_FREQ) {
                snprintf(setFreqInfo, 48,
                    "Freq must be: [%d, %d] (attempt: %d)\n", 1, MAX_FREQ, val);
                SCI_WRITE(&sci0, setFreqInfo);
            }
            int freq = SYNC(&toneGenerator, setFrequency, val);
            snprintf(setFreqInfo, 48, "Frquency set to: %d Hz\n", freq);
            SCI_WRITE(&sci0, setFreqInfo);
            break;
        /* VOLUME CONTROL: mute/unmute, vol-down, vol-up*/
        /* toggle audio */
        case 'm':
        case 'M':;
            char volMuteInfo [16] = { };
            SYNC(&toneGenerator, toggleAudio, 0);
            if (toneGenerator.isMuted) snprintf(volMuteInfo, 64, "<X  muted\n");
            else snprintf(volMuteInfo, 64, "<)  unmuted\n");
            SCI_WRITE(&sci0, volMuteInfo);
            break;
        /* arrow-down: volumn down */
        case 0x1f:;
            char volDownInfo [32] = { };
            SYNC(&toneGenerator, adjustVolume, -1);
            snprintf(volDownInfo, 64, "<)  [-----------]%d/%d\n", 
                toneGenerator.volume, SAFE_VOLUME);
            volPercentage = (10 * toneGenerator.volume) / SAFE_VOLUME;
            volDownInfo[volPercentage+5] = '|';
            SCI_WRITE(&sci0, volDownInfo);
            break;
        /* arrow-up, volumn up */
        case 0x1e:;
            char volUpInfo [32] = { };
            SYNC(&toneGenerator, adjustVolume, 1);
            snprintf(volUpInfo, 64, "<)) [-----------]%d/%d\n", 
                toneGenerator.volume, SAFE_VOLUME);
            volPercentage = (10 * toneGenerator.volume) / SAFE_VOLUME;
            volUpInfo[volPercentage+5] = '|';
            SCI_WRITE(&sci0, volUpInfo);
            break;
        /* BACKGROUND LOAD CONTROL: load-down, load-up */
        /* arrow-left, bgLoad down */
        case 0x1c:;
            char loadDwonInfo [32] = { };
            currentLoad = SYNC(&backgroundLoad, adjustLoad, -LOAD_STEP);
            snprintf(loadDwonInfo, 32, "Current background load: %d\n", currentLoad);
            SCI_WRITE(&sci0, loadDwonInfo);
            break;
        /* arrow-right, bgLoad up */
        case 0x1d:;
            char loadUpInfo [32] = { };
            currentLoad = SYNC(&backgroundLoad, adjustLoad, +LOAD_STEP);
            snprintf(loadUpInfo, 32, "Current background load: %d\n", currentLoad);
            SCI_WRITE(&sci0, loadUpInfo);
            break;
        /* DEADLINE CONTROL: enable/disable */
        /* Toggle deadline flags for each task */
        case 'd':
        case 'D':;
            char deadlineInfo [32] = {};
            int tgStatus, bgStatus;
            tgStatus = SYNC(&toneGenerator, toggleDeadlineTG, /*unused*/ 0);
            bgStatus = SYNC(&backgroundLoad, toggleDeadlineBG, /*unused*/ 0);
            if (tgStatus && bgStatus) {
                snprintf(deadlineInfo, 32, "Deadline enabled\n");
            } else{
                snprintf(deadlineInfo, 32, "Deadline disabled\n");
            }
            SCI_WRITE(&sci0, deadlineInfo);
            break;
        /* MUSIC PLAYER CONTROL*/
        /* pause/unpause music */
        case 'p':
        case 'P':;
            char musicPauseInfo [32] = { };
            int pauseStatus = SYNC(&musicPlayer, pauseMusic, 0);
            if (pauseStatus)
                snprintf(musicPauseInfo, 32, " || Music Paused\n");
            else 
                snprintf(musicPauseInfo, 32, " |> Music Unpaused\n");
            SCI_WRITE(&sci0, musicPauseInfo);
            break;
        /* stop/start music */
        case 's':
        case 'S':;
            char musicStopInfo [32] = { };
            int stopStatus = SYNC(&musicPlayer, stopMusic, 0);
            if (stopStatus)
                snprintf(musicStopInfo, 32, " X  Music Stoped\n");
            else 
                snprintf(musicStopInfo, 32, " >  Music Started\n");
            SCI_WRITE(&sci0, musicStopInfo);
            break;
        /* change key */
        case 'k':
        case 'K':;
            char musicKeyInfo [48] = { };
            arg = parseValue(self, /*unused*/0);
            if (arg < KEY_MIN || arg > KEY_MAX){
                snprintf(musicKeyInfo, 48, 
                        "Key must be: [%d, %d] (attempt: %d)\n", KEY_MIN, KEY_MAX, arg);
                SCI_WRITE(&sci0, musicKeyInfo);
            }
            arg = SYNC(&musicPlayer, setKey, arg);
            snprintf(musicKeyInfo, 32, "Key set to: %d\n", arg);
            SCI_WRITE(&sci0, musicKeyInfo);
            break;
        /* change tempo */
        case 't':
        case 'T':;
            char musicTempoInfo [48] = { };
            arg = parseValue(self, /*unused*/0);
            if (arg < TEMPO_MIN || arg > TEMPO_MAX){
                snprintf(musicTempoInfo, 48, 
                        "Tempo must be: [%d, %d] (attempt: %d)\n", TEMPO_MIN, TEMPO_MAX, arg);
                SCI_WRITE(&sci0, musicTempoInfo);
            }
            arg = SYNC(&musicPlayer, setTempo, arg);
            snprintf(musicTempoInfo, 32, "Tempo set to %d\n", arg);
            SCI_WRITE(&sci0, musicTempoInfo);
            break;
        /* check tempo */
        case 'r':
        case 'R':;
            char checkTempoInfo[32] = {};
            snprintf(checkTempoInfo, 32, "Tempo: %d\n", musicPlayer.tempo);
            SCI_WRITE(&sci0, checkTempoInfo);
            break;
        /* OTHERS */
        /* erase nhistory */
        case 'f':
        case 'F':;
            clearhistory(self, 0);
            clearbuffer(self, 0);
            char tempf[32] = {};
            snprintf(tempf, 32, "%d-history has been erased\n", NHISTORY);
            SCI_WRITE(&sci0, tempf);
            break;
        /* period lookup */
        case 'l':
        case 'L':
            val = parseValue(self, /*unused*/0);
            char lookupInfo[32] = {};
            snprintf(lookupInfo, 32, "Key: %d\n", val);
            SCI_WRITE(&sci0, lookupInfo);
            if (val < KEY_MIN || val > KEY_MAX){
                snprintf(lookupInfo, 32, "Invalid! Key must be: [%d, %d]\n", KEY_MIN, KEY_MAX);
                SCI_WRITE(&sci0, lookupInfo);
            } else{
                int index, period;
                for (int i = 0; i < 32; i++){
                    index = brotherJohnIndices[i] + val - PERIODS_IDX_DIFF;
                    period = PERIODS[index];
                    snprintf(lookupInfo, 32, "%d ", period);
                    SCI_WRITE(&sci0, lookupInfo);
                }
                SCI_WRITE(&sci0, "\n");
            }
            break;
        case 'c':
        case 'C':;
            constructCanMessage(&msg, MUSIC_DEBUG, -120);
            CAN_SEND(&can0,&msg);
            break;
        default:
            SCI_WRITE(&sci0, "Input received: \'");
            SCI_WRITECHAR(&sci0, c);
            SCI_WRITE(&sci0, "\'\n");
            break;
        }
    }
    // for the MUSICIAN, simulate that someone send the msg to you
    // by send message to yourself
    if (1) {
        switch (c)
        {
        case '-':
        case '0' ... '9':;
            if (self->mode == MUSICIAN){
                SCI_WRITE(&sci0, "Input stored in buffer: \'");
                SCI_WRITECHAR(&sci0, c);
                SCI_WRITE(&sci0, "\'\n");
                self->buffer[self->index] = c;
                self->index = (self->index + 1) % MAX_BUFFER_SIZE;
            }
            break;
        case 'p':
        case 'P':
            constructCanMessage(&msg, MUSIC_PAUSE, /*unused*/0);
            CAN_SEND(&can0, &msg);
            break;
        case 's':
        case 'S':
            constructCanMessage(&msg, MUSIC_STOP, /*unused*/0);
            CAN_SEND(&can0, &msg);
            break;
        case 'm':
        case 'M':
            constructCanMessage(&msg, MUSIC_MUTE, /*unused*/0);
            CAN_SEND(&can0, &msg);
            break;
        case 'k':
        case 'K':
            if(self->mode == MUSICIAN)
                arg = parseValue(self, /*unused*/0);
            constructCanMessage(&msg, MUSIC_SET_KEY, arg);
            CAN_SEND(&can0, &msg);
            break;
        case 't':
        case 'T':
            if(self->mode == MUSICIAN)
                arg = parseValue(self, /*unused*/0);
            constructCanMessage(&msg, MUSIC_SET_TEMPO, arg);
            CAN_SEND(&can0, &msg);
            break;
        case 0x1e:
            constructCanMessage(&msg, MUSIC_VOL_UP, 1);
            CAN_SEND(&can0, &msg);
            break;
        case 0x1f:
            constructCanMessage(&msg, MUSIC_VOL_DOWN, -1);
            CAN_SEND(&can0, &msg);
            break;
        default:
            break;
        }
    }
}

void startApp(App *self, int arg) {
    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SIO_INIT(&sio0);
    SCI_WRITE(&sci0, "Hello from RTS C1...\n");

    BEFORE(toneGenerator.toneGenDeadline,&toneGenerator, playTone, /*unused*/0);
    BEFORE(backgroundLoad.bgLoadDeadline,&backgroundLoad, loadLoop, /*unused*/0);
    ASYNC(&musicPlayer, playMusic, 0);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
